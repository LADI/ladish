/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010,2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file implements the recent project functionality
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

#include "recent_projects.h"
#include "recent_store.h"
#include "../common/catdup.h"
#include "../dbus_constants.h"
#include "../dbus/error.h"
#include "room.h"

#define RECENT_PROJECTS_STORE_FILE "recent_projects"
#define RECENT_PROJECTS_STORE_MAX_ITEMS 50

static ladish_recent_store_handle g_recent_projects_store;

bool ladish_recent_projects_init(void)
{
  char * path;
  bool ret;

  path = catdup(g_base_dir, "/" RECENT_PROJECTS_STORE_FILE);
  if (path == NULL)
  {
    log_error("catdup() failed to compose recent projects store file path");
    return false;
  }

  ret = ladish_recent_store_create(path, RECENT_PROJECTS_STORE_MAX_ITEMS, &g_recent_projects_store);
  if (!ret)
  {
    log_error("creation of recent projects store failed");
  }

  free(path);

  return ret;
}

void ladish_recent_projects_uninit(void)
{
  ladish_recent_store_destroy(g_recent_projects_store);
}

void ladish_recent_project_use(const char * project_path)
{
  ladish_recent_store_use_item(g_recent_projects_store, project_path);
}

/**********************************************************************************/
/*                                D-Bus methods                                   */
/**********************************************************************************/

struct recent_projects_callback_context
{
  DBusMessageIter array_iter;
  uint16_t max_items;
  bool error;
};

#define ctx_ptr ((struct recent_projects_callback_context *)callback_context)

bool recent_projects_callback(void * callback_context, const char * project_path)
{
  DBusMessageIter struct_iter;
  DBusMessageIter dict_iter;
  char * name;

  ASSERT(ctx_ptr->max_items > 0);

  name = ladish_get_project_name(project_path);

  if (!dbus_message_iter_open_container(&ctx_ptr->array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter))
  {
    ctx_ptr->error = true;
    goto exit;
  }

  if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &project_path))
  {
    ctx_ptr->error = true;
    goto close_struct;
  }

  if (!dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter))
  {
    ctx_ptr->error = true;
    goto close_struct;
  }

  if (!dbus_maybe_add_dict_entry_string(&dict_iter, "name", name))
  {
    ctx_ptr->error = true;
    goto close_dict;
  }

close_dict:
  if (!dbus_message_iter_close_container(&struct_iter, &dict_iter))
  {
    ctx_ptr->error = true;
  }

close_struct:
  if (!dbus_message_iter_close_container(&ctx_ptr->array_iter, &struct_iter))
  {
    ctx_ptr->error = true;
  }

exit:
  free(name);                   /* safe if name is NULL */

  if (ctx_ptr->error)
  {
    return false;               /* stop the iteration if error occurs */
  }

  ctx_ptr->max_items--;

  /* stop iteration if the requested max is reached */
  return ctx_ptr->max_items > 0;
}

#undef ctx_ptr

static void get(struct dbus_method_call * call_ptr)
{
  DBusMessageIter iter;
  struct recent_projects_callback_context ctx;

  if (!dbus_message_get_args(call_ptr->message, &cdbus_g_dbus_error, DBUS_TYPE_UINT16, &ctx.max_items, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  if (ctx.max_items == 0)
  {
    lash_dbus_error(
      call_ptr,
      LASH_DBUS_ERROR_INVALID_ARGS,
      "Invalid arguments to method \"%s\": max cannot be 0",
      call_ptr->method_name);
  }

  call_ptr->reply = dbus_message_new_method_return(call_ptr->message);
  if (call_ptr->reply == NULL)
  {
    goto fail;
  }

  dbus_message_iter_init_append(call_ptr->reply, &iter);

  if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(sa{sv})", &ctx.array_iter))
  {
    goto fail_unref;
  }

  ctx.error = false;
  ladish_recent_store_iterate_items(g_recent_projects_store, &ctx,  recent_projects_callback);
  if (ctx.error)
  {
    goto fail_unref;
  }

  if (!dbus_message_iter_close_container(&iter, &ctx.array_iter))
  {
    goto fail_unref;
  }

  return;

fail_unref:
  dbus_message_unref(call_ptr->reply);
  call_ptr->reply = NULL;

fail:
  log_error("Ran out of memory trying to construct method return");
}

METHOD_ARGS_BEGIN(get, "Get list of recent items")
  METHOD_ARG_DESCRIBE_IN("max", "q", "Max number of items caller is interested in")
  METHOD_ARG_DESCRIBE_OUT("items", "a(sa{sv})", "Item list")
METHOD_ARGS_END

METHODS_BEGIN
  METHOD_DESCRIBE(get, get)
METHODS_END

INTERFACE_BEGIN(g_iface_recent_items, IFACE_RECENT_ITEMS)
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
INTERFACE_END
