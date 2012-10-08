/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010, 2011, 2012 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of app supervisor object
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

#include "config.h"  /* Get _GNU_SOURCE defenition first to have some GNU extension available */

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>

#include "app_supervisor.h"
#include "../dbus_constants.h"
#include "loader.h"
#include "studio_internal.h"
#include "../proxies/notify_proxy.h"
#include "../proxies/lash_client_proxy.h"
#include "../common/catdup.h"
#include "../common/dirhelpers.h"
#include "jack_session.h"

struct ladish_app
{
  struct list_head siblings;
  uint64_t id;
  uuid_t uuid;
  char * name;
  char * commandline;
  char * js_commandline;
  bool terminal;
  char level[MAX_LEVEL_CHARCOUNT];
  pid_t pid;
  pid_t pgrp;
  pid_t firstborn_pid;
  pid_t firstborn_pgrp;
  int firstborn_refcount;
  bool zombie;                  /* if true, remove when stopped */
  bool autorun;
  unsigned int state;
  char * dbus_name;
  struct ladish_app_supervisor * supervisor;
};

struct ladish_app_supervisor
{
  char * name;
  char * opath;
  char * dir;

  char * js_dir;
  char * js_temp_dir;
  unsigned int pending_js_saves;
  void * save_callback_context;
  ladish_save_complete_callback save_callback;

  char * project_name;
  uint64_t version;
  uint64_t next_id;
  struct list_head applist;
  void * on_app_renamed_context;
  ladish_app_supervisor_on_app_renamed_callback on_app_renamed;
};

bool ladish_check_app_level_validity(const char * level, size_t * len_ptr)
{
  size_t len;
  len = strlen(level);
  if (len >= MAX_LEVEL_CHARCOUNT)
  {
    return false;
  }

  if (strcmp(level, LADISH_APP_LEVEL_0) != 0 &&
      strcmp(level, LADISH_APP_LEVEL_1) != 0 &&
      strcmp(level, LADISH_APP_LEVEL_LASH) != 0 &&
      strcmp(level, LADISH_APP_LEVEL_JACKSESSION) != 0)
  {
    return false;
  }

  if (len_ptr != NULL)
  {
    *len_ptr = len;
  }

  return true;
}

static int ladish_level_string_to_integer(const char * level)
{
  if (strcmp(level, LADISH_APP_LEVEL_0) == 0)
  {
    return 0;
  }
  else if (strcmp(level, LADISH_APP_LEVEL_1) == 0)
  {
    return 1;
  }
  else if (strcmp(level, LADISH_APP_LEVEL_LASH) == 0 ||
           strcmp(level, LADISH_APP_LEVEL_JACKSESSION) == 0)
  {
    return 2;
  }

  ASSERT_NO_PASS;
  return 255;
}

bool
ladish_app_supervisor_create(
  ladish_app_supervisor_handle * supervisor_handle_ptr,
  const char * opath,
  const char * name,
  void * context,
  ladish_app_supervisor_on_app_renamed_callback on_app_renamed)
{
  struct ladish_app_supervisor * supervisor_ptr;

  supervisor_ptr = malloc(sizeof(struct ladish_app_supervisor));
  if (supervisor_ptr == NULL)
  {
    log_error("malloc() failed to allocate struct ladish_app_supervisor");
    return false;
  }

  supervisor_ptr->opath = strdup(opath);
  if (supervisor_ptr->opath == NULL)
  {
    log_error("strdup() failed for app supervisor opath");
    free(supervisor_ptr);
    return false;
  }

  supervisor_ptr->name = strdup(name);
  if (supervisor_ptr->name == NULL)
  {
    log_error("strdup() failed for app supervisor name");
    free(supervisor_ptr->opath);
    free(supervisor_ptr);
    return false;
  }

  supervisor_ptr->dir = NULL;

  supervisor_ptr->js_temp_dir = NULL;
  supervisor_ptr->js_dir = NULL;
  supervisor_ptr->pending_js_saves = 0;
  supervisor_ptr->save_callback_context = NULL;
  supervisor_ptr->save_callback = NULL;

  supervisor_ptr->project_name = NULL;

  supervisor_ptr->version = 0;
  supervisor_ptr->next_id = 1;

  INIT_LIST_HEAD(&supervisor_ptr->applist);

  supervisor_ptr->on_app_renamed_context = context;
  supervisor_ptr->on_app_renamed = on_app_renamed;

  *supervisor_handle_ptr = (ladish_app_supervisor_handle)supervisor_ptr;

  return true;
}

struct ladish_app * ladish_app_supervisor_find_app_by_id_internal(struct ladish_app_supervisor * supervisor_ptr, uint64_t id)
{
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;

  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);
    if (app_ptr->id == id)
    {
      return app_ptr;
    }
  }

  return NULL;
}

void remove_app_internal(struct ladish_app_supervisor * supervisor_ptr, struct ladish_app * app_ptr)
{
  ASSERT(app_ptr->pid == 0);    /* Removing not-stoped app? Zombies will make a rebellion! */

  list_del(&app_ptr->siblings);

  supervisor_ptr->version++;

  cdbus_signal_emit(
    cdbus_g_dbus_connection,
    supervisor_ptr->opath,
    IFACE_APP_SUPERVISOR,
    "AppRemoved",
    "tt",
    &supervisor_ptr->version,
    &app_ptr->id);

  free(app_ptr->dbus_name);
  free(app_ptr->name);
  free(app_ptr->commandline);
  free(app_ptr->js_commandline);
  free(app_ptr);
}

void emit_app_state_changed(struct ladish_app_supervisor * supervisor_ptr, struct ladish_app * app_ptr)
{
  dbus_bool_t running;
  dbus_bool_t terminal;
  const char * level_str;
  uint8_t level_byte;

  running = app_ptr->pid != 0;
  terminal = app_ptr->terminal;
  level_str = app_ptr->level;
  level_byte = ladish_level_string_to_integer(app_ptr->level);

  supervisor_ptr->version++;

  cdbus_signal_emit(
    cdbus_g_dbus_connection,
    supervisor_ptr->opath,
    IFACE_APP_SUPERVISOR,
    "AppStateChanged",
    "ttsbby",
    &supervisor_ptr->version,
    &app_ptr->id,
    &app_ptr->name,
    &running,
    &terminal,
    &level_byte);

  cdbus_signal_emit(
    cdbus_g_dbus_connection,
    supervisor_ptr->opath,
    IFACE_APP_SUPERVISOR,
    "AppStateChanged2",
    "ttsbbs",
    &supervisor_ptr->version,
    &app_ptr->id,
    &app_ptr->name,
    &running,
    &terminal,
    &level_str);
}

static void ladish_js_save_complete(struct ladish_app_supervisor * supervisor_ptr)
{
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;
  bool success;

  ASSERT(supervisor_ptr->js_temp_dir != NULL);
  ASSERT(supervisor_ptr->js_dir != NULL);
  ASSERT(supervisor_ptr->pending_js_saves == 0);

  /* find whether all strdup() calls for new commandlines succeeded */
  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);
    if (app_ptr->state == LADISH_APP_STATE_STARTED &&
        strcmp(app_ptr->level, LADISH_APP_LEVEL_JACKSESSION) == 0 &&
        app_ptr->js_commandline == NULL)
    { /* a strdup() call has failed, or js save failed for some other reason,
         free js commandline buffers allocated by succeeded strdup() calls */
      list_for_each(node_ptr, &supervisor_ptr->applist)
      {
        app_ptr = list_entry(node_ptr, struct ladish_app, siblings);
        free(app_ptr->js_commandline);
        app_ptr->js_commandline = NULL;
      }
      success = false;
      goto fail_rm_temp_dir;
    }
  }

  /* move js_dir to js_dir.1; move js_temp_dir to js_dir  */
  success = ladish_rotate(supervisor_ptr->js_temp_dir, supervisor_ptr->js_dir, 10);
  if (!success)
  {
    goto fail_rm_temp_dir;
  }

  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);
    if (app_ptr->state == LADISH_APP_STATE_STARTED &&
        strcmp(app_ptr->level, LADISH_APP_LEVEL_JACKSESSION) == 0)
    {
      ASSERT(app_ptr->commandline != NULL);
      ASSERT(app_ptr->js_commandline != NULL);
      free(app_ptr->commandline);
      app_ptr->commandline = app_ptr->js_commandline;
      app_ptr->js_commandline = NULL;
    }
  }

  goto done;

fail_rm_temp_dir:
  if (!ladish_rmdir_recursive(supervisor_ptr->js_temp_dir))
  {
    log_error("Cannot remove JS temp dir '%s'", supervisor_ptr->js_temp_dir);
  }

done:
  free(supervisor_ptr->js_temp_dir);
  supervisor_ptr->js_temp_dir = NULL;

  supervisor_ptr->save_callback(supervisor_ptr->save_callback_context, success);
  supervisor_ptr->save_callback = NULL;
  supervisor_ptr->save_callback_context = NULL;
}

#define supervisor_ptr ((struct ladish_app_supervisor *)supervisor_handle)

const char * ladish_app_supervisor_get_opath(ladish_app_supervisor_handle supervisor_handle)
{
  return supervisor_ptr->opath;
}

ladish_app_handle ladish_app_supervisor_find_app_by_name(ladish_app_supervisor_handle supervisor_handle, const char * name)
{
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;

  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);
    if (strcmp(app_ptr->name, name) == 0)
    {
      return (ladish_app_handle)app_ptr;
    }
  }

  return NULL;
}

ladish_app_handle ladish_app_supervisor_find_app_by_id(ladish_app_supervisor_handle supervisor_handle, uint64_t id)
{
  return (ladish_app_handle)ladish_app_supervisor_find_app_by_id_internal(supervisor_ptr, id);
}

ladish_app_handle ladish_app_supervisor_find_app_by_pid(ladish_app_supervisor_handle supervisor_handle, pid_t pid)
{
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;

  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);
    if (app_ptr->pid == pid)
    {
      //log_info("app \"%s\" found by pid %llu", app_ptr->name, (unsigned long long)pid);
      return (ladish_app_handle)app_ptr;
    }
  }

  return NULL;
}

ladish_app_handle ladish_app_supervisor_find_app_by_uuid(ladish_app_supervisor_handle supervisor_handle, const uuid_t uuid)
{
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;

  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);
    if (uuid_compare(app_ptr->uuid, uuid) == 0)
    {
      return (ladish_app_handle)app_ptr;
    }
  }

  return NULL;
}

ladish_app_handle
ladish_app_supervisor_add(
  ladish_app_supervisor_handle supervisor_handle,
  const char * name,
  uuid_t uuid,
  bool autorun,
  const char * command,
  bool terminal,
  const char * level)
{
  struct ladish_app * app_ptr;
  dbus_bool_t running;
  dbus_bool_t dbus_terminal;
  size_t len;
  uint8_t level_byte;

  if (!ladish_check_app_level_validity(level, &len))
  {
    log_error("invalid level '%s'", level);
    goto fail;
  }

  if (strcmp(level, LADISH_APP_LEVEL_JACKSESSION) == 0 &&
      supervisor_ptr->dir == NULL)
  {
    log_error("jack session apps need directory");
    goto fail;
  }

  app_ptr = malloc(sizeof(struct ladish_app));
  if (app_ptr == NULL)
  {
    log_error("malloc of struct ladish_app failed");
    goto fail;
  }

  app_ptr->name = strdup(name);
  if (app_ptr->name == NULL)
  {
    log_error("strdup() failed for app name");
    goto free_app;
  }

  app_ptr->commandline = strdup(command);
  if (app_ptr->commandline == NULL)
  {
    log_error("strdup() failed for app commandline");
    goto free_name;
  }

  app_ptr->js_commandline = NULL;

  app_ptr->dbus_name = NULL;

  app_ptr->terminal = terminal;
  memcpy(app_ptr->level, level, len + 1);
  app_ptr->pid = 0;
  app_ptr->pgrp = 0;
  app_ptr->firstborn_pid = 0;
  app_ptr->firstborn_pgrp = 0;
  app_ptr->firstborn_refcount = 0;

  app_ptr->id = supervisor_ptr->next_id++;
  if (uuid == NULL || uuid_is_null(uuid))
  {
    uuid_generate(app_ptr->uuid);
  }
  else
  {
    uuid_copy(app_ptr->uuid, uuid);
  }

  app_ptr->zombie = false;
  app_ptr->state = LADISH_APP_STATE_STOPPED;
  app_ptr->autorun = autorun;
  app_ptr->supervisor = supervisor_ptr;
  list_add_tail(&app_ptr->siblings, &supervisor_ptr->applist);

  supervisor_ptr->version++;

  running = false;
  dbus_terminal = terminal;
  level = app_ptr->level;
  level_byte = ladish_level_string_to_integer(app_ptr->level);
  cdbus_signal_emit(
    cdbus_g_dbus_connection,
    supervisor_ptr->opath,
    IFACE_APP_SUPERVISOR,
    "AppAdded",
    "ttsbby",
    &supervisor_ptr->version,
    &app_ptr->id,
    &app_ptr->name,
    &running,
    &dbus_terminal,
    &level_byte);
  cdbus_signal_emit(
    cdbus_g_dbus_connection,
    supervisor_ptr->opath,
    IFACE_APP_SUPERVISOR,
    "AppAdded2",
    "ttsbbs",
    &supervisor_ptr->version,
    &app_ptr->id,
    &app_ptr->name,
    &running,
    &dbus_terminal,
    &level);

  return (ladish_app_handle)app_ptr;

free_name:
  free(app_ptr->name);
free_app:
  free(app_ptr);
fail:
  return NULL;
}

static void ladish_app_send_signal(struct ladish_app * app_ptr, int sig, bool prefer_firstborn)
{
  pid_t pid;
  const char * signal_name;

  ASSERT(app_ptr->state = LADISH_APP_STATE_STARTED);

  switch (sig)
  {
  case SIGTERM:
    signal_name = "SIGTERM";
    break;
  case SIGKILL:
    signal_name = "SIGKILL";
    break;
  case SIGUSR1:
    signal_name = "SIGUSR1";
    break;
  default:
    signal_name = strsignal(sig);
    if (signal_name == NULL)
    {
      signal_name = "unknown";
    }
  }

  if (app_ptr->pid == 0)
  {
    log_error("not sending signal %d (%s) to app '%s' because its pid is %d", sig, signal_name, app_ptr->name, (int)app_ptr->pid);
    ASSERT_NO_PASS;
    return;
  }

  switch (sig)
  {
  case SIGKILL:
  case SIGTERM:
    if (app_ptr->pgrp == 0)
    {
      app_ptr->pgrp = getpgid(app_ptr->pid);
      if (app_ptr->pgrp == -1)
      {
        app_ptr->pgrp = 0;
        if (errno != ESRCH)
        {
          log_error("getpgid(%u) failed. %s (%d)", (unsigned int)app_ptr->pid, strerror(errno), errno);
        }
      }
    }

    if (app_ptr->firstborn_pid != 0)
    {
      app_ptr->firstborn_pgrp = getpgid(app_ptr->firstborn_pid);
      if (app_ptr->firstborn_pgrp == -1)
      {
        app_ptr->firstborn_pgrp = 0;
        if (errno != ESRCH)
        {
          log_error("getpgid(%u) failed (firstborn). %s (%d)", (unsigned int)app_ptr->firstborn_pid, strerror(errno), errno);
        }
      }
    }

    if (app_ptr->pgrp != 0)
    {
      log_info("sending signal %d (%s) to pgrp %u ('%s')", sig, signal_name, (unsigned int)app_ptr->pgrp, app_ptr->name);

      if (app_ptr->pgrp <= 1)
      {
        ASSERT_NO_PASS;
        return;
      }

      killpg(app_ptr->pgrp, sig);

      if (app_ptr->firstborn_pid != 0)
      {
        if (app_ptr->firstborn_pgrp != 0)
        {
          if (app_ptr->firstborn_pgrp <= 1)
          {
            ASSERT_NO_PASS;
            return;
          }

          if (app_ptr->firstborn_pgrp != app_ptr->pgrp)
          {
            log_info("sending signal %d (%s) to firstborn pgrp %u ('%s')", sig, signal_name, (unsigned int)app_ptr->firstborn_pgrp, app_ptr->name);

            killpg(app_ptr->firstborn_pgrp, sig);
          }
          return;
        }
        /* fall through to sending signal to pid */
      }
      else
      {
        return;
      }
    }
    /* fall through to sending signal to pid */
  default:
    if (app_ptr->pid <= 1)
    {
      ASSERT_NO_PASS;
      return;
    }

    if (prefer_firstborn && app_ptr->firstborn_pid != 0)
    {
      pid = app_ptr->firstborn_pid;
    }
    else
    {
      pid = app_ptr->pid;
    }

    if (pid <= 1)
    {
      ASSERT_NO_PASS;
      return;
    }

    log_info("sending signal %d (%s) to '%s' with pid %u", sig, signal_name, app_ptr->name, (unsigned int)pid);
    kill(pid, sig);
  }
}

static inline void ladish_app_initiate_lash_save(struct ladish_app * app_ptr, const char * base_dir)
{
  char * app_dir;
  char uuid_str[37];

  if (base_dir == NULL)
  {
    log_error("Cannot initiate LASH save because base dir is unknown");
    ASSERT_NO_PASS;
    goto exit;
  }

  uuid_unparse(app_ptr->uuid, uuid_str);
  app_dir = catdup3(base_dir, "/lash_apps/", uuid_str);
  if (app_dir == NULL)
  {
    log_error("Cannot initiate LASH save because of memory allocation failure.");
    goto exit;
  }

  if (!ensure_dir_exist(app_dir, S_IRWXU | S_IRWXG | S_IRWXO))
  {
    goto free;
  }

  if (lash_client_proxy_save(app_ptr->dbus_name, app_dir))
  {
    log_info("LASH Save into '%s' initiated for '%s' with D-Bus name '%s'", app_dir, app_ptr->name, app_ptr->dbus_name);
  }

free:
  free(app_dir);
exit:
  return;
}

static inline void ladish_app_initiate_lash_restore(struct ladish_app * app_ptr, const char * base_dir)
{
  char * app_dir;
  char uuid_str[37];
  struct st;

  if (base_dir == NULL)
  {
    log_error("Cannot initiate LASH restore because base dir is unknown");
    ASSERT_NO_PASS;
    goto exit;
  }

  uuid_unparse(app_ptr->uuid, uuid_str);
  app_dir = catdup3(base_dir, "/lash_apps/", uuid_str);
  if (app_dir == NULL)
  {
    log_error("Cannot initiate LASH restore because of memory allocation failure.");
    goto exit;
  }

  if (!check_dir_exists(app_dir))
  {
    log_info("Not initiating LASH restore because of app directory '%s' does not exist.", app_dir);
    goto free;
  }

  if (lash_client_proxy_restore(app_ptr->dbus_name, app_dir))
  {
    log_info("LASH Save from '%s' initiated for '%s' with D-Bus name '%s'", app_dir, app_ptr->name, app_ptr->dbus_name);
  }

free:
  free(app_dir);
exit:
  return;
}

#define app_ptr ((struct ladish_app *)context)

static void ladish_js_app_save_complete(void * context, const char * commandline)
{
  if (commandline != NULL)
  {
    log_info("JS app saved, commandline '%s'", commandline);
    ASSERT(app_ptr->supervisor->js_temp_dir != NULL);

    ASSERT(app_ptr->js_commandline == NULL);
    app_ptr->js_commandline = strdup(commandline);
    if (app_ptr->js_commandline == NULL)
    {
      log_error("strdup() failed for JS app '%s' commandline '%s'", app_ptr->name, commandline);
    }
  }
  else
  {
    ASSERT(app_ptr->js_commandline == NULL);
    log_error("JACK session save failed for JS app '%s'", app_ptr->name);
  }

  if (app_ptr->supervisor->pending_js_saves != 1)
  {
    ASSERT(app_ptr->supervisor->pending_js_saves > 1);
    app_ptr->supervisor->pending_js_saves--;
    log_info("%u more JS apps are still saving", app_ptr->supervisor->pending_js_saves);
    return;
  }

  log_info("No more pending JS app saves");
  app_ptr->supervisor->pending_js_saves = 0;

  ladish_js_save_complete(app_ptr->supervisor);
}

#undef app_ptr

static inline void ladish_app_initiate_save(struct ladish_app * app_ptr)
{
  if (strcmp(app_ptr->level, LADISH_APP_LEVEL_LASH) == 0 &&
      app_ptr->dbus_name != NULL)
  {
    ladish_app_initiate_lash_save(app_ptr, app_ptr->supervisor->dir != NULL ? app_ptr->supervisor->dir : g_base_dir);
  }
  else if (strcmp(app_ptr->level, LADISH_APP_LEVEL_JACKSESSION) == 0)
  {
    log_info("Initiating JACK session save for '%s'", app_ptr->name);
    if (!ladish_js_save_app(app_ptr->uuid, app_ptr->supervisor->js_temp_dir, app_ptr, ladish_js_app_save_complete))
    {
      ladish_js_app_save_complete(app_ptr, NULL);
    }
  }
  else if (strcmp(app_ptr->level, LADISH_APP_LEVEL_1) == 0)
  {
    ladish_app_send_signal(app_ptr, SIGUSR1, true);
  }
}

static inline void ladish_app_initiate_stop(struct ladish_app * app_ptr)
{
  if (strcmp(app_ptr->level, LADISH_APP_LEVEL_LASH) == 0 &&
      app_ptr->dbus_name != NULL &&
      lash_client_proxy_quit(app_ptr->dbus_name))
  {
    log_info("LASH Quit initiated for '%s' with D-Bus name '%s'", app_ptr->name, app_ptr->dbus_name);
  }
  else
  {
    ladish_app_send_signal(app_ptr, SIGTERM, false);
  }

  app_ptr->state = LADISH_APP_STATE_STOPPING;
}

bool ladish_app_supervisor_clear(ladish_app_supervisor_handle supervisor_handle)
{
  struct list_head * node_ptr;
  struct list_head * safe_node_ptr;
  struct ladish_app * app_ptr;
  bool lifeless;

  free(supervisor_ptr->js_temp_dir);
  supervisor_ptr->js_temp_dir = NULL;
  free(supervisor_ptr->js_dir);
  supervisor_ptr->js_dir = NULL;
  free(supervisor_ptr->dir);
  supervisor_ptr->dir = NULL;

  free(supervisor_ptr->project_name);
  supervisor_ptr->project_name = NULL;

  lifeless = true;

  list_for_each_safe(node_ptr, safe_node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);
    if (app_ptr->pid != 0)
    {
      log_info("terminating '%s'...", app_ptr->name);
      ladish_app_initiate_stop(app_ptr);
      app_ptr->zombie = true;
      lifeless = false;
    }
    else
    {
      log_info("removing '%s'", app_ptr->name);
      remove_app_internal(supervisor_ptr, app_ptr);
    }
  }

  return lifeless;
}

void ladish_app_supervisor_destroy(ladish_app_supervisor_handle supervisor_handle)
{
  ladish_app_supervisor_clear(supervisor_handle);
  free(supervisor_ptr->name);
  free(supervisor_ptr->opath);
  free(supervisor_ptr);
}

bool
ladish_app_supervisor_set_directory(
  ladish_app_supervisor_handle supervisor_handle,
  const char * dir)
{
  char * dup;
  char * js_dir;

  ASSERT(supervisor_ptr->pending_js_saves == 0);

  dup = strdup(dir);
  if (dup == NULL)
  {
    log_error("strdup(\"%s\") failed", dir);
    return false;
  }

  js_dir = catdup(dir, "/js_apps");
  if (js_dir == NULL)
  {
    log_error("catdup() failed to compose supervisor js dir path");
    free(dup);
    return false;
  }

  free(supervisor_ptr->dir);
  supervisor_ptr->dir = dup;

  free(supervisor_ptr->js_dir);
  supervisor_ptr->js_dir = js_dir;

  return true;
}

bool
ladish_app_supervisor_set_project_name(
  ladish_app_supervisor_handle supervisor_handle,
  const char * project_name)
{
  char * dup;

  if (project_name != NULL)
  {
    dup = strdup(project_name);
    if (dup == NULL)
    {
      log_error("strdup(\"%s\") failed", project_name);
      return false;
    }
  }
  else
  {
    dup = NULL;
  }

  free(supervisor_ptr->project_name);
  supervisor_ptr->project_name = dup;

  return true;
}

bool ladish_app_supervisor_child_exit(ladish_app_supervisor_handle supervisor_handle, pid_t pid, int exit_status)
{
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;
  bool clean;

  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);
    if (app_ptr->pid == pid)
    {
      clean = WIFEXITED(exit_status) && WEXITSTATUS(exit_status) == 0;

      log_info("%s exit of child '%s' detected.", clean ? "clean" : "dirty", app_ptr->name);

      app_ptr->pid = 0;
      app_ptr->pgrp = 0;
      /* firstborn pid and pgrp is not reset here because it is refcounted
         and managed independently through the add/del_pid() methods */

      if (app_ptr->zombie)
      {
        remove_app_internal(supervisor_ptr, app_ptr);
      }
      else
      {
        if (app_ptr->state == LADISH_APP_STATE_STARTED && !clean)
        {
          ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "App terminated unexpectedly", app_ptr->name);
        }

        app_ptr->state = LADISH_APP_STATE_STOPPED;

        emit_app_state_changed(supervisor_ptr, app_ptr);
      }

      return true;
    }
  }

  return false;
}

bool
ladish_app_supervisor_enum(
  ladish_app_supervisor_handle supervisor_handle,
  void * context,
  ladish_app_supervisor_enum_callback callback)
{
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;

  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);

    if (!callback(context, app_ptr->name, app_ptr->pid != 0, app_ptr->commandline, app_ptr->terminal, app_ptr->level, app_ptr->pid, app_ptr->uuid))
    {
      return false;
    }
  }

  return true;
}

#define app_ptr ((struct ladish_app *)app_handle)

bool ladish_app_supervisor_start_app(ladish_app_supervisor_handle supervisor_handle, ladish_app_handle app_handle)
{
  char uuid_str[37];
  char * js_dir;
  bool ret;

  app_ptr->zombie = false;

  ASSERT(app_ptr->pid == 0);

  if (strcmp(app_ptr->level, LADISH_APP_LEVEL_JACKSESSION) == 0)
  {
    uuid_unparse(app_ptr->uuid, uuid_str);
    js_dir = catdup4(supervisor_ptr->js_dir, "/", uuid_str, "/");
    if (js_dir == NULL)
    {
      log_error("catdup4() failed to compose app jack session dir");
      return false;
    }
  }
  else
  {
    js_dir = NULL;
  }

  ret = loader_execute(
    supervisor_ptr->name,
    supervisor_ptr->project_name,
    app_ptr->name,
    supervisor_ptr->dir != NULL ? supervisor_ptr->dir : "/",
    js_dir,
    app_ptr->terminal,
    app_ptr->commandline,
    &app_ptr->pid);

  free(js_dir);

  if (!ret)
  {
    return false;
  }

  ASSERT(app_ptr->pid != 0);
  app_ptr->state = LADISH_APP_STATE_STARTED;

  emit_app_state_changed(supervisor_ptr, app_ptr);
  return true;
}

void ladish_app_supervisor_remove_app(ladish_app_supervisor_handle supervisor_handle, ladish_app_handle app_handle)
{
  remove_app_internal(supervisor_ptr, app_ptr);
}

unsigned int ladish_app_get_state(ladish_app_handle app_handle)
{
  return app_ptr->state;
}

bool ladish_app_is_running(ladish_app_handle app_handle)
{
  return app_ptr->pid != 0;
}

const char * ladish_app_get_name(ladish_app_handle app_handle)
{
  return app_ptr->name;
}

void ladish_app_get_uuid(ladish_app_handle app_handle, uuid_t uuid)
{
  uuid_copy(uuid, app_ptr->uuid);
}

void ladish_app_stop(ladish_app_handle app_handle)
{
  ladish_app_initiate_stop(app_ptr);
}

void ladish_app_kill(ladish_app_handle app_handle)
{
  ladish_app_send_signal(app_ptr, SIGKILL, false);
  app_ptr->state = LADISH_APP_STATE_KILL;
}

void ladish_app_restore(ladish_app_handle app_handle)
{
  if (strcmp(app_ptr->level, LADISH_APP_LEVEL_LASH) == 0 &&
      app_ptr->dbus_name != NULL)
  {
    ladish_app_initiate_lash_restore(app_ptr, app_ptr->supervisor->dir != NULL ? app_ptr->supervisor->dir : g_base_dir);
  }
}

void ladish_app_add_pid(ladish_app_handle app_handle, pid_t pid)
{
  if (app_ptr->pid == 0)
  {
    log_error("Associating pid with stopped app does not make sense");
    ASSERT_NO_PASS;
    return;
  }

  if (pid <= 1)                 /* catch -1, 0 and 1 */
  {
    log_error("Refusing domination by ignoring pid %d", (int)pid);
    ASSERT_NO_PASS;
    return;
  }

  if (app_ptr->pid == pid)
  { /* The top level process that is already known */
    return;
  }

  if (app_ptr->firstborn_pid != 0)
  { /* Ignore non-first children */
    if (app_ptr->firstborn_pid == pid)
    {
      app_ptr->firstborn_refcount++;
    }
    return;
  }

  log_info("First grandchild with pid %u", (unsigned int)pid);
  app_ptr->firstborn_pid = pid;
  ASSERT(app_ptr->firstborn_refcount == 0);
  app_ptr->firstborn_refcount = 1;
}

void ladish_app_del_pid(ladish_app_handle app_handle, pid_t pid)
{
  if (app_ptr->firstborn_pid != 0 && app_ptr->firstborn_pid == pid)
  {
    ASSERT(app_ptr->firstborn_refcount > 0);
    app_ptr->firstborn_refcount--;
    if (app_ptr->firstborn_refcount > 0)
    {
      return;
    }
    log_info("First grandchild with pid %u has gone", (unsigned int)pid);
    app_ptr->firstborn_pid = 0;
    app_ptr->firstborn_pgrp = 0;
    app_ptr->firstborn_refcount = 0;
  }
}

bool ladish_app_set_dbus_name(ladish_app_handle app_handle, const char * name)
{
  char * dup;

  dup = strdup(name);
  if (dup == NULL)
  {
    log_error("strdup() failed for app dbus name");
    return false;
  }

  free(app_ptr->dbus_name);
  app_ptr->dbus_name = dup;
  return true;
}

#undef app_ptr

void ladish_app_supervisor_autorun(ladish_app_supervisor_handle supervisor_handle)
{
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;

  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);

    if (!app_ptr->autorun)
    {
      continue;
    }

    app_ptr->autorun = false;

    log_info("autorun('%s', %s, '%s') called", app_ptr->name, app_ptr->terminal ? "terminal" : "shell", app_ptr->commandline);

    if (!ladish_app_supervisor_start_app((ladish_app_supervisor_handle)supervisor_ptr, (ladish_app_handle)app_ptr))
    {
      log_error("Execution of '%s' failed",  app_ptr->commandline);
      return;
    }
  }
}

void ladish_app_supervisor_stop(ladish_app_supervisor_handle supervisor_handle)
{
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;

  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);
    if (app_ptr->pid != 0)
    {
      log_info("terminating '%s'...", app_ptr->name);
      app_ptr->autorun = true;
      ladish_app_initiate_stop(app_ptr);
    }
  }
}

void
ladish_app_supervisor_save(
  ladish_app_supervisor_handle supervisor_handle,
  void * context,
  ladish_save_complete_callback callback)
{
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;
  bool success;

  ASSERT(callback != NULL);

  ASSERT(supervisor_ptr->js_temp_dir == NULL);
  ASSERT(supervisor_ptr->pending_js_saves == 0);
  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);
    if (app_ptr->state == LADISH_APP_STATE_STARTED &&
        strcmp(app_ptr->level, LADISH_APP_LEVEL_JACKSESSION) == 0)
    {
      ASSERT(app_ptr->js_commandline == NULL);
      supervisor_ptr->pending_js_saves++;
    }
  }

  supervisor_ptr->save_callback_context = context;
  supervisor_ptr->save_callback = callback;

  if (supervisor_ptr->pending_js_saves > 0)
  {
    ASSERT(supervisor_ptr->dir != NULL);
    supervisor_ptr->js_temp_dir = catdup(supervisor_ptr->dir, "/js_apps.tmpXXXXXX");
    if (supervisor_ptr->js_temp_dir == NULL)
    {
      log_error("catdup() failed to compose supervisor js temp dir path template");
      goto reset_js_pending_saves;
    }

    if (mkdtemp(supervisor_ptr->js_temp_dir) == NULL)
    {
      log_error("mkdtemp('%s') failed. errno = %d (%s)", supervisor_ptr->js_temp_dir, errno, strerror(errno));
      goto free_js_temp_dir;
    }

    log_info("saving %u JACK session apps to '%s'", supervisor_ptr->pending_js_saves, supervisor_ptr->js_temp_dir);
  }

  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);
    if (app_ptr->state != LADISH_APP_STATE_STARTED)
    {
      continue;
    }

    if (app_ptr->pid == 0)
    {
      ASSERT_NO_PASS;
      continue;
    }

    ladish_app_initiate_save(app_ptr);
  }

  success = true;
  goto exit;

free_js_temp_dir:
  free(supervisor_ptr->js_temp_dir);
  supervisor_ptr->js_temp_dir = NULL;
reset_js_pending_saves:
  supervisor_ptr->pending_js_saves = 0;
  success = false;
exit:
  if (supervisor_ptr->pending_js_saves == 0 && supervisor_ptr->save_callback != NULL)
  { /* Room/studio without js apps.
       In case of ladish_js_save_app() failure,
       the callback will either be called already or
       will be called later when all pending js app saves are done.
       In former case callback will be NULL.
       In latter case pending_js_saves will be greater than zero. */
    ASSERT(supervisor_ptr->save_callback == callback);
    ASSERT(supervisor_ptr->save_callback_context = context);
    callback(context, success);
    supervisor_ptr->save_callback = NULL;
    supervisor_ptr->save_callback_context = NULL;
  }
}

const char * ladish_app_supervisor_get_name(ladish_app_supervisor_handle supervisor_handle)
{
  return supervisor_ptr->name;
}

unsigned int ladish_app_supervisor_get_running_app_count(ladish_app_supervisor_handle supervisor_handle)
{
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;
  unsigned int counter;

  counter = 0;
  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);
    if (app_ptr->pid != 0)
    {
      counter++;
    }
  }

  return counter;
}

bool ladish_app_supervisor_has_apps(ladish_app_supervisor_handle supervisor_handle)
{
  return !list_empty(&supervisor_ptr->applist);
}

void ladish_app_supervisor_dump(ladish_app_supervisor_handle supervisor_handle)
{
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;
  char uuid_str[37];

  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);
    uuid_unparse(app_ptr->uuid, uuid_str);
    log_info("app '%s' with commandline '%s'", app_ptr->name, app_ptr->commandline);
    log_info("  %s", uuid_str);
    log_info("  %s, %s, level '%s'", app_ptr->terminal ? "terminal" : "shell", app_ptr->autorun ? "autorun" : "stopped", app_ptr->level, app_ptr->commandline);
  }
}

#undef supervisor_ptr

/**********************************************************************************/
/*                                D-Bus methods                                   */
/**********************************************************************************/

#define supervisor_ptr ((struct ladish_app_supervisor *)call_ptr->iface_context)

static void get_version(struct cdbus_method_call * call_ptr)
{
  uint32_t version;

  version = 1;

  cdbus_method_return_new_single(call_ptr, DBUS_TYPE_UINT32, &version);
}

static void get_all_multiversion(struct cdbus_method_call * call_ptr, int version)
{
  DBusMessageIter iter, array_iter, struct_iter;
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;
  dbus_bool_t running;
  dbus_bool_t terminal;
  const char * level_str;
  uint8_t level_byte;

  //log_info("get_all called");

  call_ptr->reply = dbus_message_new_method_return(call_ptr->message);
  if (call_ptr->reply == NULL)
  {
    goto fail;
  }

  dbus_message_iter_init_append(call_ptr->reply, &iter);

  if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT64, &supervisor_ptr->version))
  {
    goto fail_unref;
  }

  if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, version == 1 ? "(tsbby)" : "(tsbbs)", &array_iter))
  {
    goto fail_unref;
  }

  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);

    log_info("app '%s' (%llu)", app_ptr->name, (unsigned long long)app_ptr->id);

    if (!dbus_message_iter_open_container (&array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter))
    {
      goto fail_unref;
    }

    if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_UINT64, &app_ptr->id))
    {
      goto fail_unref;
    }

    if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &app_ptr->name))
    {
      goto fail_unref;
    }

    running = app_ptr->pid != 0;
    if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_BOOLEAN, &running))
    {
      goto fail_unref;
    }

    terminal = app_ptr->terminal;
    if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_BOOLEAN, &terminal))
    {
      goto fail_unref;
    }

    if (version == 1)
    {
      level_byte = ladish_level_string_to_integer(app_ptr->level);
      if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_BYTE, &level_byte))
      {
        goto fail_unref;
      }
    }
    else
    {
      ASSERT(version == 2);

      level_str = app_ptr->level;
      if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &level_str))
      {
        goto fail_unref;
      }
    }

    if (!dbus_message_iter_close_container(&array_iter, &struct_iter))
    {
      goto fail_unref;
    }
  }

  if (!dbus_message_iter_close_container(&iter, &array_iter))
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

static void get_all1(struct cdbus_method_call * call_ptr)
{
  get_all_multiversion(call_ptr, 1);
}

static void get_all2(struct cdbus_method_call * call_ptr)
{
  get_all_multiversion(call_ptr, 2);
}

static void run_custom1(struct cdbus_method_call * call_ptr)
{
  dbus_bool_t terminal;
  const char * commandline;
  const char * name;
  uint8_t level;

  if (!dbus_message_get_args(
        call_ptr->message,
        &cdbus_g_dbus_error,
        DBUS_TYPE_BOOLEAN, &terminal,
        DBUS_TYPE_STRING, &commandline,
        DBUS_TYPE_STRING, &name,
        DBUS_TYPE_BYTE, &level,
        DBUS_TYPE_INVALID))
  {
    cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  log_info("%s('%s', %s, '%s', %"PRIu8") called", call_ptr->method_name, name, terminal ? "terminal" : "shell", commandline, level);

  if (level > 1)
  {
    cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "invalid integer level %"PRIu8, level);
    return;
  }

  if (ladish_command_new_app(
        call_ptr,
        ladish_studio_get_cmd_queue(),
        supervisor_ptr->opath,
        terminal,
        commandline,
        name,
        level == 0 ? LADISH_APP_LEVEL_0 : LADISH_APP_LEVEL_1))
  {
    cdbus_method_return_new_void(call_ptr);
  }
}

static void run_custom2(struct cdbus_method_call * call_ptr)
{
  dbus_bool_t terminal;
  const char * commandline;
  const char * name;
  const char * level;

  if (!dbus_message_get_args(
        call_ptr->message,
        &cdbus_g_dbus_error,
        DBUS_TYPE_BOOLEAN, &terminal,
        DBUS_TYPE_STRING, &commandline,
        DBUS_TYPE_STRING, &name,
        DBUS_TYPE_STRING, &level,
        DBUS_TYPE_INVALID))
  {
    cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  log_info("%s('%s', %s, '%s', '%s') called", call_ptr->method_name, name, terminal ? "terminal" : "shell", commandline, level);

  if (!ladish_check_app_level_validity(level, NULL))
  {
    cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "invalid level '%s'", level);
    return;
  }

  if (ladish_command_new_app(
        call_ptr,
        ladish_studio_get_cmd_queue(),
        supervisor_ptr->opath,
        terminal,
        commandline,
        name,
        level))
  {
    cdbus_method_return_new_void(call_ptr);
  }
}

static void start_app(struct cdbus_method_call * call_ptr)
{
  uint64_t id;

  if (!dbus_message_get_args(
        call_ptr->message,
        &cdbus_g_dbus_error,
        DBUS_TYPE_UINT64, &id,
        DBUS_TYPE_INVALID))
  {
    cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  if (ladish_command_change_app_state(call_ptr, ladish_studio_get_cmd_queue(), supervisor_ptr->opath, id, LADISH_APP_STATE_STARTED))
  {
    cdbus_method_return_new_void(call_ptr);
  }
}

static void stop_app(struct cdbus_method_call * call_ptr)
{
  uint64_t id;

  if (!dbus_message_get_args(
        call_ptr->message,
        &cdbus_g_dbus_error,
        DBUS_TYPE_UINT64, &id,
        DBUS_TYPE_INVALID))
  {
    cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  if (ladish_command_change_app_state(call_ptr, ladish_studio_get_cmd_queue(), supervisor_ptr->opath, id, LADISH_APP_STATE_STOPPED))
  {
    cdbus_method_return_new_void(call_ptr);
  }
}

static void kill_app(struct cdbus_method_call * call_ptr)
{
  uint64_t id;

  if (!dbus_message_get_args(
        call_ptr->message,
        &cdbus_g_dbus_error,
        DBUS_TYPE_UINT64, &id,
        DBUS_TYPE_INVALID))
  {
    cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  if (ladish_command_change_app_state(call_ptr, ladish_studio_get_cmd_queue(), supervisor_ptr->opath, id, LADISH_APP_STATE_KILL))
  {
    cdbus_method_return_new_void(call_ptr);
  }
}

static void get_app_properties_multiversion(struct cdbus_method_call * call_ptr, int version)
{
  uint64_t id;
  struct ladish_app * app_ptr;
  dbus_bool_t running;
  dbus_bool_t terminal;
  const char * level_str;
  uint8_t level_byte;
  int level_type;
  void * level_ptr;

  if (!dbus_message_get_args(
        call_ptr->message,
        &cdbus_g_dbus_error,
        DBUS_TYPE_UINT64, &id,
        DBUS_TYPE_INVALID))
  {
    cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  app_ptr = ladish_app_supervisor_find_app_by_id_internal(supervisor_ptr, id);
  if (app_ptr == NULL)
  {
    cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "App with ID %"PRIu64" not found", id);
    return;
  }

  running = app_ptr->pid != 0;
  terminal = app_ptr->terminal;
  if (version == 1)
  {
    level_byte = ladish_level_string_to_integer(app_ptr->level);
    level_type = DBUS_TYPE_BYTE;
    level_ptr = &level_byte;
  }
  else
  {
    ASSERT(version == 2);
    level_str = app_ptr->level;
    level_type = DBUS_TYPE_STRING;
    level_ptr = &level_str;
  }

  call_ptr->reply = dbus_message_new_method_return(call_ptr->message);
  if (call_ptr->reply == NULL)
  {
    goto fail;
  }

  if (!dbus_message_append_args(
        call_ptr->reply,
        DBUS_TYPE_STRING, &app_ptr->name,
        DBUS_TYPE_STRING, &app_ptr->commandline,
        DBUS_TYPE_BOOLEAN, &running,
        DBUS_TYPE_BOOLEAN, &terminal,
        level_type, level_ptr,
        DBUS_TYPE_INVALID))
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

static void get_app_properties1(struct cdbus_method_call * call_ptr)
{
  get_app_properties_multiversion(call_ptr, 1);
}

static void get_app_properties2(struct cdbus_method_call * call_ptr)
{
  get_app_properties_multiversion(call_ptr, 2);
}

static void set_app_properties_multiversion(struct cdbus_method_call * call_ptr, int version)
{
  uint64_t id;
  dbus_bool_t terminal;
  const char * name;
  const char * commandline;
  const char * level_str;
  uint8_t level_byte;
  int level_type;
  void * level_ptr;
  struct ladish_app * app_ptr;
  char * name_buffer;
  char * commandline_buffer;
  size_t len;

  if (version == 1)
  {
    level_type = DBUS_TYPE_BYTE;
    level_ptr = &level_byte;
  }
  else
  {
    ASSERT(version == 2);
    level_type = DBUS_TYPE_STRING;
    level_ptr = &level_str;
  }

  if (!dbus_message_get_args(
        call_ptr->message,
        &cdbus_g_dbus_error,
        DBUS_TYPE_UINT64, &id,
        DBUS_TYPE_STRING, &name,
        DBUS_TYPE_STRING, &commandline,
        DBUS_TYPE_BOOLEAN, &terminal,
        level_type, level_ptr,
        DBUS_TYPE_INVALID))
  {
    cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  if (version == 1)
  {
    if (level_byte > 1)
    {
      cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "invalid integer level %"PRIu8, level_byte);
      return;
    }

    level_str = level_byte == 0 ? LADISH_APP_LEVEL_0 : LADISH_APP_LEVEL_1;
    len = strlen(level_str);
  }
  else
  {
    ASSERT(version == 2);
    if (!ladish_check_app_level_validity(level_str, &len))
    {
      cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "invalid level '%s'", level_str);
      return;
    }
  }

  app_ptr = ladish_app_supervisor_find_app_by_id_internal(supervisor_ptr, id);
  if (app_ptr == NULL)
  {
    cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "App with ID %"PRIu64" not found", id);
    return;
  }

  if (app_ptr->pid != 0 && strcmp(commandline, app_ptr->commandline) != 0)
  {
    cdbus_error(call_ptr, DBUS_ERROR_FAILED, "Cannot change commandline when app is running. '%s' -> '%s'", app_ptr->commandline, commandline);
    return;
  }

  if (app_ptr->pid != 0 && ((app_ptr->terminal && !terminal) || (!app_ptr->terminal && terminal)))
  {
    cdbus_error(call_ptr, DBUS_ERROR_FAILED, "Cannot change whether to run in terminal when app is running");
    return;
  }

  if (app_ptr->pid != 0 && strcmp(app_ptr->level, level_str) != 0)
  {
    cdbus_error(call_ptr, DBUS_ERROR_FAILED, "Cannot change app level when app is running");
    return;
  }

  if (strcmp(commandline, app_ptr->commandline) != 0)
  {
    commandline_buffer = strdup(commandline);
    if (commandline_buffer == NULL)
    {
      cdbus_error(call_ptr, DBUS_ERROR_FAILED, "strdup() failed for app commandline");
      return;
    }
  }
  else
  {
    commandline_buffer = NULL;
  }

  if (strcmp(name, app_ptr->name) != 0)
  {
    name_buffer = strdup(name);
    if (name_buffer == NULL)
    {
      cdbus_error(call_ptr, DBUS_ERROR_FAILED, "strdup() failed for app nam");
      if (commandline_buffer != NULL)
      {
        free(commandline_buffer);
      }
      return;
    }
  }
  else
  {
    name_buffer = NULL;
  }

  if (name_buffer != NULL)
  {
    supervisor_ptr->on_app_renamed(supervisor_ptr->on_app_renamed_context, app_ptr->uuid, app_ptr->name, name_buffer);
    free(app_ptr->name);
    app_ptr->name = name_buffer;
  }

  if (commandline_buffer != NULL)
  {
    free(app_ptr->commandline);
    app_ptr->commandline = commandline_buffer;
  }

  memcpy(app_ptr->level, level_str, len + 1);
  app_ptr->terminal = terminal;

  emit_app_state_changed(supervisor_ptr, app_ptr);

  cdbus_method_return_new_void(call_ptr);
}

static void set_app_properties1(struct cdbus_method_call * call_ptr)
{
  set_app_properties_multiversion(call_ptr, 1);
}

static void set_app_properties2(struct cdbus_method_call * call_ptr)
{
  set_app_properties_multiversion(call_ptr, 2);
}


static void remove_app(struct cdbus_method_call * call_ptr)
{
  uint64_t id;

  if (!dbus_message_get_args(
        call_ptr->message,
        &cdbus_g_dbus_error,
        DBUS_TYPE_UINT64, &id,
        DBUS_TYPE_INVALID))
  {
    cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  if (ladish_command_remove_app(call_ptr, ladish_studio_get_cmd_queue(), supervisor_ptr->opath, id))
  {
    cdbus_method_return_new_void(call_ptr);
  }
}

static void is_app_running(struct cdbus_method_call * call_ptr)
{
  uint64_t id;
  struct ladish_app * app_ptr;
  dbus_bool_t running;

  if (!dbus_message_get_args(
        call_ptr->message,
        &cdbus_g_dbus_error,
        DBUS_TYPE_UINT64, &id,
        DBUS_TYPE_INVALID))
  {
    cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  app_ptr = ladish_app_supervisor_find_app_by_id_internal(supervisor_ptr, id);
  if (app_ptr == NULL)
  {
    cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "App with ID %"PRIu64" not found", id);
    return;
  }

  running = app_ptr->pid != 0;

  cdbus_method_return_new_single(call_ptr, DBUS_TYPE_BOOLEAN, &running);
}

#undef supervisor_ptr

CDBUS_METHOD_ARGS_BEGIN(GetInterfaceVersion, "Get version of this D-Bus interface")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("running", DBUS_TYPE_UINT32_AS_STRING, "Interface version")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(GetAll, "Get list of apps")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("list_version", DBUS_TYPE_UINT64_AS_STRING, "Version of the list")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("apps_list", "a(tsbby)", "List of apps")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(GetAll2, "Get list of apps")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("list_version", DBUS_TYPE_UINT64_AS_STRING, "Version of the list")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("apps_list", "a(tsbbs)", "List of apps")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(RunCustom, "Start application by supplying commandline")
  CDBUS_METHOD_ARG_DESCRIBE_IN("terminal", DBUS_TYPE_BOOLEAN_AS_STRING, "Whether to run in terminal")
  CDBUS_METHOD_ARG_DESCRIBE_IN("commandline", DBUS_TYPE_STRING_AS_STRING, "Commandline")
  CDBUS_METHOD_ARG_DESCRIBE_IN("name", DBUS_TYPE_STRING_AS_STRING, "Name")
  CDBUS_METHOD_ARG_DESCRIBE_IN("level", DBUS_TYPE_BYTE_AS_STRING, "Level")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(RunCustom2, "Start application by supplying commandline")
  CDBUS_METHOD_ARG_DESCRIBE_IN("terminal", DBUS_TYPE_BOOLEAN_AS_STRING, "Whether to run in terminal")
  CDBUS_METHOD_ARG_DESCRIBE_IN("commandline", DBUS_TYPE_STRING_AS_STRING, "Commandline")
  CDBUS_METHOD_ARG_DESCRIBE_IN("name", DBUS_TYPE_STRING_AS_STRING, "Name")
  CDBUS_METHOD_ARG_DESCRIBE_IN("level", DBUS_TYPE_STRING_AS_STRING, "Level")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(StartApp, "Start application")
  CDBUS_METHOD_ARG_DESCRIBE_IN("id", DBUS_TYPE_UINT64_AS_STRING, "id of app")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(StopApp, "Stop application")
  CDBUS_METHOD_ARG_DESCRIBE_IN("id", DBUS_TYPE_UINT64_AS_STRING, "id of app")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(KillApp, "Kill application")
  CDBUS_METHOD_ARG_DESCRIBE_IN("id", DBUS_TYPE_UINT64_AS_STRING, "id of app")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(RemoveApp, "Remove application")
  CDBUS_METHOD_ARG_DESCRIBE_IN("id", DBUS_TYPE_UINT64_AS_STRING, "id of app")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(GetAppProperties, "Get properties of an application")
  CDBUS_METHOD_ARG_DESCRIBE_IN("id", DBUS_TYPE_UINT64_AS_STRING, "id of app")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("name", DBUS_TYPE_STRING_AS_STRING, "")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("commandline", DBUS_TYPE_STRING_AS_STRING, "Commandline")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("running", DBUS_TYPE_BOOLEAN_AS_STRING, "")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("terminal", DBUS_TYPE_BOOLEAN_AS_STRING, "Whether to run in terminal")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("level", DBUS_TYPE_BYTE_AS_STRING, "Level")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(GetAppProperties2, "Get properties of an application")
  CDBUS_METHOD_ARG_DESCRIBE_IN("id", DBUS_TYPE_UINT64_AS_STRING, "id of app")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("name", DBUS_TYPE_STRING_AS_STRING, "")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("commandline", DBUS_TYPE_STRING_AS_STRING, "Commandline")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("running", DBUS_TYPE_BOOLEAN_AS_STRING, "")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("terminal", DBUS_TYPE_BOOLEAN_AS_STRING, "Whether to run in terminal")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("level", DBUS_TYPE_STRING_AS_STRING, "Level")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(SetAppProperties, "Set properties of an application")
  CDBUS_METHOD_ARG_DESCRIBE_IN("id", DBUS_TYPE_UINT64_AS_STRING, "id of app")
  CDBUS_METHOD_ARG_DESCRIBE_IN("name", DBUS_TYPE_STRING_AS_STRING, "")
  CDBUS_METHOD_ARG_DESCRIBE_IN("commandline", DBUS_TYPE_STRING_AS_STRING, "Commandline")
  CDBUS_METHOD_ARG_DESCRIBE_IN("terminal", DBUS_TYPE_BOOLEAN_AS_STRING, "Whether to run in terminal")
  CDBUS_METHOD_ARG_DESCRIBE_IN("level", DBUS_TYPE_BYTE_AS_STRING, "Level")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(SetAppProperties2, "Set properties of an application")
  CDBUS_METHOD_ARG_DESCRIBE_IN("id", DBUS_TYPE_UINT64_AS_STRING, "id of app")
  CDBUS_METHOD_ARG_DESCRIBE_IN("name", DBUS_TYPE_STRING_AS_STRING, "")
  CDBUS_METHOD_ARG_DESCRIBE_IN("commandline", DBUS_TYPE_STRING_AS_STRING, "Commandline")
  CDBUS_METHOD_ARG_DESCRIBE_IN("terminal", DBUS_TYPE_BOOLEAN_AS_STRING, "Whether to run in terminal")
  CDBUS_METHOD_ARG_DESCRIBE_IN("level", DBUS_TYPE_STRING_AS_STRING, "Level")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(IsAppRunning, "Check whether application is running")
  CDBUS_METHOD_ARG_DESCRIBE_IN("id", DBUS_TYPE_UINT64_AS_STRING, "id of app")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("running", DBUS_TYPE_BOOLEAN_AS_STRING, "Whether app is running")
CDBUS_METHOD_ARGS_END


CDBUS_METHODS_BEGIN
  CDBUS_METHOD_DESCRIBE(GetInterfaceVersion, get_version)     /* sync */
  CDBUS_METHOD_DESCRIBE(GetAll, get_all1)                     /* sync */
  CDBUS_METHOD_DESCRIBE(GetAll2, get_all2)                    /* sync */
  CDBUS_METHOD_DESCRIBE(RunCustom, run_custom1)               /* async */
  CDBUS_METHOD_DESCRIBE(RunCustom2, run_custom2)              /* async */
  CDBUS_METHOD_DESCRIBE(StartApp, start_app)                  /* async */
  CDBUS_METHOD_DESCRIBE(StopApp, stop_app)                    /* async */
  CDBUS_METHOD_DESCRIBE(KillApp, kill_app)                    /* async */
  CDBUS_METHOD_DESCRIBE(GetAppProperties, get_app_properties1)  /* sync */
  CDBUS_METHOD_DESCRIBE(GetAppProperties2, get_app_properties2) /* sync */
  CDBUS_METHOD_DESCRIBE(SetAppProperties, set_app_properties1)  /* sync */
  CDBUS_METHOD_DESCRIBE(SetAppProperties2, set_app_properties2) /* sync */
  CDBUS_METHOD_DESCRIBE(RemoveApp, remove_app)                /* sync */
  CDBUS_METHOD_DESCRIBE(IsAppRunning, is_app_running)         /* sync */
CDBUS_METHODS_END

CDBUS_SIGNAL_ARGS_BEGIN(AppAdded, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("new_list_version", DBUS_TYPE_UINT64_AS_STRING, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("id", DBUS_TYPE_UINT64_AS_STRING, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("name", DBUS_TYPE_STRING_AS_STRING, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("running", DBUS_TYPE_BOOLEAN_AS_STRING, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("terminal", DBUS_TYPE_BOOLEAN_AS_STRING, "Whether to run in terminal")
  CDBUS_SIGNAL_ARG_DESCRIBE("level", DBUS_TYPE_BYTE_AS_STRING, "Level")
CDBUS_SIGNAL_ARGS_END

CDBUS_SIGNAL_ARGS_BEGIN(AppAdded2, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("new_list_version", DBUS_TYPE_UINT64_AS_STRING, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("id", DBUS_TYPE_UINT64_AS_STRING, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("name", DBUS_TYPE_STRING_AS_STRING, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("running", DBUS_TYPE_BOOLEAN_AS_STRING, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("terminal", DBUS_TYPE_BOOLEAN_AS_STRING, "Whether to run in terminal")
  CDBUS_SIGNAL_ARG_DESCRIBE("level", DBUS_TYPE_STRING_AS_STRING, "Level")
CDBUS_SIGNAL_ARGS_END

CDBUS_SIGNAL_ARGS_BEGIN(AppRemoved, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("new_list_version", DBUS_TYPE_UINT64_AS_STRING, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("id", DBUS_TYPE_UINT64_AS_STRING, "")
CDBUS_SIGNAL_ARGS_END

CDBUS_SIGNAL_ARGS_BEGIN(AppStateChanged, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("new_list_version", DBUS_TYPE_UINT64_AS_STRING, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("id", DBUS_TYPE_UINT64_AS_STRING, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("name", DBUS_TYPE_STRING_AS_STRING, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("running", DBUS_TYPE_BOOLEAN_AS_STRING, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("terminal", DBUS_TYPE_BOOLEAN_AS_STRING, "Whether to run in terminal")
  CDBUS_SIGNAL_ARG_DESCRIBE("level", DBUS_TYPE_BYTE_AS_STRING, "Level")
CDBUS_SIGNAL_ARGS_END

CDBUS_SIGNAL_ARGS_BEGIN(AppStateChanged2, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("new_list_version", DBUS_TYPE_UINT64_AS_STRING, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("id", DBUS_TYPE_UINT64_AS_STRING, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("name", DBUS_TYPE_STRING_AS_STRING, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("running", DBUS_TYPE_BOOLEAN_AS_STRING, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("terminal", DBUS_TYPE_BOOLEAN_AS_STRING, "Whether to run in terminal")
  CDBUS_SIGNAL_ARG_DESCRIBE("level", DBUS_TYPE_STRING_AS_STRING, "Level")
CDBUS_SIGNAL_ARGS_END

CDBUS_SIGNALS_BEGIN
  CDBUS_SIGNAL_DESCRIBE(AppAdded)
  CDBUS_SIGNAL_DESCRIBE(AppAdded2)
  CDBUS_SIGNAL_DESCRIBE(AppRemoved)
  CDBUS_SIGNAL_DESCRIBE(AppStateChanged)
  CDBUS_SIGNAL_DESCRIBE(AppStateChanged2)
CDBUS_SIGNALS_END

CDBUS_INTERFACE_DEFAULT_HANDLER_METHODS_AND_SIGNALS(g_iface_app_supervisor, IFACE_APP_SUPERVISOR)
