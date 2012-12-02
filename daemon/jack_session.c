/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2011,2012 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to jack session helper functionality
 **************************************************************************
 *
 * LADI Session Handler is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * LADI Session Handler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LADI Session Handler. If not, see <http://www.gnu.org/licenses/>
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <unistd.h>

#include "jack_session.h"
#include "../common/catdup.h"
#include "studio.h"
#include "../proxies/jack_proxy.h"
#include "../proxies/conf_proxy.h"
#include "conf.h"

struct ladish_js_find_app_client_context
{
  uuid_t app_uuid;
  bool query;
  const char * client_name;
};

#define ctx_ptr ((struct ladish_js_find_app_client_context *)context)

bool
ladish_js_find_app_client_callback(
  void * context,
  ladish_graph_handle UNUSED(graph_handle),
  bool hidden,
  ladish_client_handle client_handle,
  const char * client_name,
  void ** UNUSED(client_iteration_context_ptr_ptr))
{
  bool has_callback;
  const char * jack_name;

  //log_info("checking client '%s'", client_name);

  if (hidden)
  {
    return true;              /* continue iteration */
  }

  jack_name = ladish_client_get_jack_name(client_handle);
  if (jack_name == NULL)
  {
    log_error("visible client '%s' in JACK graph without jack client name", client_name);
    ASSERT_NO_PASS;
    return true;              /* continue iteration */
  }

  if (ctx_ptr->query)
  {
    if (!jack_proxy_session_has_callback(jack_name, &has_callback))
    {
      return true;              /* continue iteration */
    }

    //log_info("client '%s' %s session callback", jack_name, has_callback ? "has" : "hasn't");
    ladish_client_set_js(client_handle, has_callback);

    if (!has_callback)
    {
      return true;              /* continue iteration */
    }
  }
  else
  {
    if (!ladish_client_is_js(client_handle))
    {
      return true;              /* continue iteration */
    }
  }

  log_info("client '%s' has session callback", jack_name);

  if (!ladish_client_is_app(client_handle, ctx_ptr->app_uuid))
  {
    return true;                /* continue iteration */
  }

  ctx_ptr->client_name = jack_name;
  return false;                 /* stop iteration */
}

#undef ctx_ptr

static
const char *
ladish_js_find_app_client(
  uuid_t app_uuid)
{
  struct ladish_js_find_app_client_context ctx;

  ctx.client_name = NULL;
  ctx.query = false;

  uuid_copy(ctx.app_uuid, app_uuid);
  ladish_graph_iterate_nodes(ladish_studio_get_jack_graph(), &ctx, ladish_js_find_app_client_callback, NULL, NULL);

  /* app registered the callback after the client activation, try harder */
  if (ctx.client_name == NULL)
  {
    ctx.query = true;
    ladish_graph_iterate_nodes(ladish_studio_get_jack_graph(), &ctx, ladish_js_find_app_client_callback, NULL, NULL);
  }

  return ctx.client_name;
}

struct ladish_js_save_app_context
{
  void * context;
  void (* callback)(
    void * context,
    const char * commandline);

  char * target_dir;            /* the dir supplied as parameter to ladish_js_save_app() */
  char * temp_dir;              /* temp dir that is passed to jack session notify */
  char * client_dir;            /* client dir within the temp dir */
};

#define ctx_ptr ((struct ladish_js_save_app_context *)context)

void ladish_js_save_app_complete(void * context, const char * commandline)
{
  int iret;
  unsigned int delay;

  if (commandline == NULL)
  {
    goto call;
  }

  log_info("JS app save complete. commandline='%s'", commandline);

  if (!conf_get_uint(LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY, &delay))
  {
    delay = LADISH_CONF_KEY_DAEMON_JS_SAVE_DELAY_DEFAULT;
  }

  if (delay > 0)
  {
    log_info("sleeping for %u seconds...", delay);
    sleep(delay);
  }

  iret = rename(ctx_ptr->client_dir, ctx_ptr->target_dir);
  if (iret != 0)
  {
    log_error("rename('%s' -> '%s') failed. errno = %d (%s)", ctx_ptr->client_dir, ctx_ptr->target_dir, errno, strerror(errno));
    commandline = NULL;
    goto call;
  }

  log_debug("removing temp dir '%s'", ctx_ptr->temp_dir);
  iret = rmdir(ctx_ptr->temp_dir);
  if (iret < 0)
  {
    log_error("rmdir('%s') failed. errno = %d (%s)", ctx_ptr->temp_dir, errno, strerror(errno));
    commandline = NULL;
  }

call:
  ctx_ptr->callback(ctx_ptr->context, commandline);

  free(ctx_ptr->client_dir);
  free(ctx_ptr->temp_dir);
  free(ctx_ptr->target_dir);
  free(ctx_ptr);
}

#undef ctx_ptr

bool
ladish_js_save_app(
  uuid_t app_uuid,
  const char * parent_dir,
  void * completion_context,
  void (* completion_callback)(
    void * completion_context,
    const char * commandline))
{
  struct ladish_js_save_app_context * ctx_ptr;
  char app_uuid_str[37];
  int ret;
  const char * js_client;
  size_t ofs;

  js_client = ladish_js_find_app_client(app_uuid);
  if (js_client == NULL)
  {
    log_error("cannot find js app client");
    goto fail;
  }

  ctx_ptr = malloc(sizeof(struct ladish_js_save_app_context));
  if (ctx_ptr == NULL)
  {
    log_error("malloc() failed to allocate ladish_js_save_app_context struct");
    goto fail;
  }

  uuid_unparse(app_uuid, app_uuid_str);
  ctx_ptr->target_dir = catdup3(parent_dir, "/", app_uuid_str);
  if (ctx_ptr->target_dir == NULL)
  {
    log_error("strdup3(\"%s\", \"/\", \"%s\") failed for compose js target app dir", parent_dir, app_uuid_str);
    goto fail_free_struct;
  }

  log_info("JS target app dir is '%s'", ctx_ptr->target_dir);

  ctx_ptr->temp_dir = catdup(ctx_ptr->target_dir, ".tmpXXXXXX/");
  if (ctx_ptr->temp_dir == NULL)
  {
    log_error("catdup() failed to compose app js temp dir path template");
    goto fail_free_target_dir;
  }

  ofs = strlen(ctx_ptr->temp_dir) - 1;
  ctx_ptr->temp_dir[ofs] = 0;   /* mkdtemp wants last chars six chars to be XXXXXX */

  if (mkdtemp(ctx_ptr->temp_dir) == NULL)
  {
    log_error("mkdtemp('%s') failed. errno = %d (%s)", ctx_ptr->temp_dir, errno, strerror(errno));
    goto fail_free_temp_dir;
  }

  ctx_ptr->temp_dir[ofs] = '/';   /* jack session wants last char to be / */

  log_info("JS temp app dir is '%s'", ctx_ptr->temp_dir);

  ctx_ptr->client_dir = catdup(ctx_ptr->temp_dir, js_client);
  if (ctx_ptr->client_dir == NULL)
  {
    log_error("catdup3() failed to compose js client dir path");
    goto fail_rm_temp_dir;
  }

  ctx_ptr->callback = completion_callback;
  ctx_ptr->context = completion_context;

  if (!jack_proxy_session_save_one(true, js_client, ctx_ptr->temp_dir, ctx_ptr, ladish_js_save_app_complete))
  {
    log_error("jack session failed to initiate save of '%s' app state to '%s'", js_client, ctx_ptr->temp_dir);
    goto fail_rm_temp_dir;
  }

  log_info("JS app save initiated");

  return true;

fail_rm_temp_dir:
  ret = rmdir(ctx_ptr->temp_dir);
  if (ret < 0)
  {
    log_error("rmdir('%s') failed. errno = %d (%s)", ctx_ptr->temp_dir, errno, strerror(errno));
  }
//fail_free_client_dir:
  free(ctx_ptr->client_dir);
fail_free_temp_dir:
  free(ctx_ptr->temp_dir);
fail_free_target_dir:
  free(ctx_ptr->target_dir);
fail_free_struct:
  free(ctx_ptr);
fail:
  return false;
}
