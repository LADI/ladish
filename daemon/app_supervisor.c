/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
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

#include <sys/types.h>
#include <signal.h>

#include "app_supervisor.h"
#include "../dbus/error.h"
#include "../dbus_constants.h"
#include "loader.h"
#include "studio_internal.h"
#include "../proxies/notify_proxy.h"

struct ladish_app
{
  struct list_head siblings;
  uint64_t id;
  char * name;
  char * commandline;
  bool terminal;
  uint8_t level;
  pid_t pid;
  bool zombie;                  /* if true, remove when stopped */
  bool autorun;
  unsigned int state;
};

struct ladish_app_supervisor
{
  char * name;
  char * opath;
  char * dir;
  uint64_t version;
  uint64_t next_id;
  struct list_head applist;
  void * on_app_renamed_context;
  void (* on_app_renamed)(void * context, const char * old_name, const char * new_app_name);
};

bool
ladish_app_supervisor_create(
  ladish_app_supervisor_handle * supervisor_handle_ptr,
  const char * opath,
  const char * name,
  void * context,
  void (* on_app_renamed)(void * context, const char * old_name, const char * new_app_name))
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

  dbus_signal_emit(
    g_dbus_connection,
    supervisor_ptr->opath,
    IFACE_APP_SUPERVISOR,
    "AppRemoved",
    "tt",
    &supervisor_ptr->version,
    &app_ptr->id);

  free(app_ptr->name);
  free(app_ptr->commandline);
  free(app_ptr);
}

void emit_app_state_changed(struct ladish_app_supervisor * supervisor_ptr, struct ladish_app * app_ptr)
{
  dbus_bool_t running;
  dbus_bool_t terminal;

  running = app_ptr->pid != 0;
  terminal = app_ptr->terminal;

  dbus_signal_emit(
    g_dbus_connection,
    supervisor_ptr->opath,
    IFACE_APP_SUPERVISOR,
    "AppStateChanged",
    "tsbby",
    &app_ptr->id,
    &app_ptr->name,
    &running,
    &terminal,
    &app_ptr->level);
}

#define supervisor_ptr ((struct ladish_app_supervisor *)supervisor_handle)

const char * ladish_app_supervisor_get_opath(ladish_app_supervisor_handle supervisor_handle)
{
  return supervisor_ptr->opath;
}

bool ladish_app_supervisor_check_app_name(ladish_app_supervisor_handle supervisor_handle, const char * name)
{
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;

  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);
    if (strcmp(app_ptr->name, name) == 0)
    {
      return true;
    }
  }

  return false;
}

ladish_app_handle ladish_app_supervisor_find_app_by_id(ladish_app_supervisor_handle supervisor_handle, uint64_t id)
{
  return (ladish_app_handle)ladish_app_supervisor_find_app_by_id_internal(supervisor_ptr, id);
}

ladish_app_handle
ladish_app_supervisor_add(
  ladish_app_supervisor_handle supervisor_handle,
  const char * name,
  bool autorun,
  const char * command,
  bool terminal,
  uint8_t level)
{
  struct ladish_app * app_ptr;
  dbus_bool_t running;

  app_ptr = malloc(sizeof(struct ladish_app));
  if (app_ptr == NULL)
  {
    log_error("malloc of struct ladish_app failed");
    return NULL;
  }

  app_ptr->name = strdup(name);
  if (app_ptr->name == NULL)
  {
    log_error("strdup() failed for app name");
    free(app_ptr);
    return NULL;
  }

  app_ptr->commandline = strdup(command);
  if (app_ptr->commandline == NULL)
  {
    log_error("strdup() failed for app commandline");
    free(app_ptr->name);
    free(app_ptr);
    return NULL;
  }

  app_ptr->terminal = terminal;
  app_ptr->level = level;
  app_ptr->pid = 0;

  app_ptr->id = supervisor_ptr->next_id++;
  app_ptr->zombie = false;
  app_ptr->state = LADISH_APP_STATE_STOPPED;
  app_ptr->autorun = autorun;
  list_add_tail(&app_ptr->siblings, &supervisor_ptr->applist);

  running = false;
  dbus_signal_emit(
    g_dbus_connection,
    supervisor_ptr->opath,
    IFACE_APP_SUPERVISOR,
    "AppAdded",
    "ttsbby",
    &supervisor_ptr->version,
    &app_ptr->id,
    &app_ptr->name,
    &running,
    &terminal,
    &app_ptr->level);

  return (ladish_app_handle)app_ptr;
}

static void ladish_app_send_signal(struct ladish_app * app_ptr, int sig)
{
  ASSERT(app_ptr->state = LADISH_APP_STATE_STARTED);
  if (app_ptr->pid <= 0)
  {
    ASSERT_NO_PASS;
    return;
  }
  kill(app_ptr->pid, sig);
}

void ladish_app_supervisor_clear(ladish_app_supervisor_handle supervisor_handle)
{
  struct list_head * node_ptr;
  struct list_head * safe_node_ptr;
  struct ladish_app * app_ptr;

  if (supervisor_ptr->dir != NULL)
  {
    free(supervisor_ptr->dir);
    supervisor_ptr->dir = NULL;
  }

  list_for_each_safe(node_ptr, safe_node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);
    if (app_ptr->pid != 0)
    {
      log_info("terminating '%s'...", app_ptr->name);
      ladish_app_send_signal(app_ptr, SIGTERM);
      app_ptr->zombie = true;
      app_ptr->state = LADISH_APP_STATE_STOPPING;
    }
    else
    {
      log_info("removing '%s'", app_ptr->name);
      remove_app_internal(supervisor_ptr, app_ptr);
    }
  }
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

  dup = strdup(dir);
  if (dup == NULL)
  {
    log_error("strdup(\"%s\") failed", dir);
    return false;
  }

  if (supervisor_ptr->dir != NULL)
  {
    free(supervisor_ptr->dir);
  }

  supervisor_ptr->dir = dup;

  return true;
}

bool ladish_app_supervisor_child_exit(ladish_app_supervisor_handle supervisor_handle, pid_t pid)
{
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;

  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);
    if (app_ptr->pid == pid)
    {
      log_info("exit of child '%s' detected.", app_ptr->name);

      app_ptr->pid = 0;
      if (app_ptr->zombie)
      {
        remove_app_internal(supervisor_ptr, app_ptr);
      }
      else
      {
        if (app_ptr->state == LADISH_APP_STATE_STARTED)
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
  bool (* callback)(void * context, const char * name, bool running, const char * command, bool terminal, uint8_t level, pid_t pid))
{
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;

  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);

    if (!callback(context, app_ptr->name, app_ptr->pid != 0, app_ptr->commandline, app_ptr->terminal, app_ptr->level, app_ptr->pid))
    {
      return false;
    }
  }

  return true;
}

#define app_ptr ((struct ladish_app *)app_handle)

bool ladish_app_supervisor_start_app(ladish_app_supervisor_handle supervisor_handle, ladish_app_handle app_handle)
{
  app_ptr->zombie = false;

  ASSERT(app_ptr->pid == 0);

  if (!loader_execute(
        supervisor_ptr->name,
        app_ptr->name,
        supervisor_ptr->dir != NULL ? supervisor_ptr->dir : "/",
        app_ptr->terminal,
        app_ptr->commandline,
        &app_ptr->pid))
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

void ladish_app_stop(ladish_app_handle app_handle)
{
  ladish_app_send_signal(app_ptr, SIGTERM);
  app_ptr->state = LADISH_APP_STATE_STOPPING;
}

void ladish_app_kill(ladish_app_handle app_handle)
{
  ladish_app_send_signal(app_ptr, SIGKILL);
  app_ptr->state = LADISH_APP_STATE_KILL;
}

void ladish_app_save_L1(ladish_app_handle app_handle)
{
  if (app_ptr->level == 1)
  {
    log_info("sending SIGUSR1 to '%s' with pid %u", app_ptr->name, (unsigned int)app_ptr->pid);
    ladish_app_send_signal(app_ptr, SIGUSR1);
  }
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
      ladish_app_send_signal(app_ptr, SIGTERM);
      app_ptr->autorun = true;
      app_ptr->state = LADISH_APP_STATE_STOPPING;
    }
  }
}

void ladish_app_supervisor_save_L1(ladish_app_supervisor_handle supervisor_handle)
{
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;

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

    if (app_ptr->level == 1)
    {
      log_info("sending SIGUSR1 to '%s' with pid %u", app_ptr->name, (unsigned int)app_ptr->pid);
      ladish_app_send_signal(app_ptr, SIGUSR1);
    }
  }
}

char * ladish_app_supervisor_search_app(ladish_app_supervisor_handle supervisor_handle, pid_t pid)
{
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;
  char * name;

  list_for_each(node_ptr, &supervisor_ptr->applist)
  {
    app_ptr = list_entry(node_ptr, struct ladish_app, siblings);
    if (app_ptr->pid == pid)
    {
      //log_info("app \"%s\" found by pid %llu", app_ptr->name, (unsigned long long)pid);
      name = strdup(app_ptr->name);
      if (name == NULL)
      {
        log_error("strdup() failed for '%s'", app_ptr->name);
      }

      return name;
    }
  }

  return NULL;
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

#undef supervisor_ptr

/**********************************************************************************/
/*                                D-Bus methods                                   */
/**********************************************************************************/

#define supervisor_ptr ((struct ladish_app_supervisor *)call_ptr->iface_context)

static void get_all(struct dbus_method_call * call_ptr)
{
  DBusMessageIter iter, array_iter, struct_iter;
  struct list_head * node_ptr;
  struct ladish_app * app_ptr;
  dbus_bool_t running;
  dbus_bool_t terminal;

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

  if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(tsbby)", &array_iter))
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

    if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_BYTE, &app_ptr->level))
    {
      goto fail_unref;
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

static void run_custom(struct dbus_method_call * call_ptr)
{
  dbus_bool_t terminal;
  const char * commandline;
  const char * name;
  uint8_t level;

  if (!dbus_message_get_args(
        call_ptr->message,
        &g_dbus_error,
        DBUS_TYPE_BOOLEAN, &terminal,
        DBUS_TYPE_STRING, &commandline,
        DBUS_TYPE_STRING, &name,
        DBUS_TYPE_BYTE, &level,
        DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  log_info("run_custom('%s', %s, '%s', %"PRIu8") called", name, terminal ? "terminal" : "shell", commandline, level);

  if (level != 0 && level != 1)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "invalid level %"PRIu8, level);
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
    method_return_new_void(call_ptr);
  }
}

static void start_app(struct dbus_method_call * call_ptr)
{
  uint64_t id;

  if (!dbus_message_get_args(
        call_ptr->message,
        &g_dbus_error,
        DBUS_TYPE_UINT64, &id,
        DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  if (ladish_command_change_app_state(call_ptr, ladish_studio_get_cmd_queue(), supervisor_ptr->opath, id, LADISH_APP_STATE_STARTED))
  {
    method_return_new_void(call_ptr);
  }
}

static void stop_app(struct dbus_method_call * call_ptr)
{
  uint64_t id;

  if (!dbus_message_get_args(
        call_ptr->message,
        &g_dbus_error,
        DBUS_TYPE_UINT64, &id,
        DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  if (ladish_command_change_app_state(call_ptr, ladish_studio_get_cmd_queue(), supervisor_ptr->opath, id, LADISH_APP_STATE_STOPPED))
  {
    method_return_new_void(call_ptr);
  }
}

static void kill_app(struct dbus_method_call * call_ptr)
{
  uint64_t id;

  if (!dbus_message_get_args(
        call_ptr->message,
        &g_dbus_error,
        DBUS_TYPE_UINT64, &id,
        DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  if (ladish_command_change_app_state(call_ptr, ladish_studio_get_cmd_queue(), supervisor_ptr->opath, id, LADISH_APP_STATE_KILL))
  {
    method_return_new_void(call_ptr);
  }
}

static void get_app_properties(struct dbus_method_call * call_ptr)
{
  uint64_t id;
  struct ladish_app * app_ptr;
  dbus_bool_t running;
  dbus_bool_t terminal;

  if (!dbus_message_get_args(
        call_ptr->message,
        &g_dbus_error,
        DBUS_TYPE_UINT64, &id,
        DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  app_ptr = ladish_app_supervisor_find_app_by_id_internal(supervisor_ptr, id);
  if (app_ptr == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "App with ID %"PRIu64" not found", id);
    return;
  }

  running = app_ptr->pid != 0;
  terminal = app_ptr->terminal;

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
        DBUS_TYPE_BYTE, &app_ptr->level,
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

static void set_app_properties(struct dbus_method_call * call_ptr)
{
  uint64_t id;
  dbus_bool_t terminal;
  const char * name;
  const char * commandline;
  uint8_t level;
  struct ladish_app * app_ptr;
  char * name_buffer;
  char * commandline_buffer;

  if (!dbus_message_get_args(
        call_ptr->message,
        &g_dbus_error,
        DBUS_TYPE_UINT64, &id,
        DBUS_TYPE_STRING, &name,
        DBUS_TYPE_STRING, &commandline,
        DBUS_TYPE_BOOLEAN, &terminal,
        DBUS_TYPE_BYTE, &level,
        DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  app_ptr = ladish_app_supervisor_find_app_by_id_internal(supervisor_ptr, id);
  if (app_ptr == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "App with ID %"PRIu64" not found", id);
    return;
  }

  if (app_ptr->pid != 0 && strcmp(commandline, app_ptr->commandline) != 0)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "Cannot change commandline when app is running. '%s' -> '%s'", app_ptr->commandline, commandline);
    return;
  }

  if (app_ptr->pid != 0 && ((app_ptr->terminal && !terminal) || (!app_ptr->terminal && terminal)))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "Cannot change whether to run in terminal when app is running");
    return;
  }

  if (app_ptr->pid != 0 && app_ptr->level != level)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "Cannot change app level when app is running");
    return;
  }

  if (strcmp(commandline, app_ptr->commandline) != 0)
  {
    commandline_buffer = strdup(commandline);
    if (commandline_buffer == NULL)
    {
      lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup() failed for app commandline");
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
      lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup() failed for app nam");
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
    supervisor_ptr->on_app_renamed(supervisor_ptr->on_app_renamed_context, app_ptr->name, name_buffer);
    free(app_ptr->name);
    app_ptr->name = name_buffer;
  }

  if (commandline_buffer != NULL)
  {
    free(app_ptr->commandline);
    app_ptr->commandline = commandline_buffer;
  }

  app_ptr->level = level;
  app_ptr->terminal = terminal;

  emit_app_state_changed(supervisor_ptr, app_ptr);

  method_return_new_void(call_ptr);
}

static void remove_app(struct dbus_method_call * call_ptr)
{
  uint64_t id;

  if (!dbus_message_get_args(
        call_ptr->message,
        &g_dbus_error,
        DBUS_TYPE_UINT64, &id,
        DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  if (ladish_command_remove_app(call_ptr, ladish_studio_get_cmd_queue(), supervisor_ptr->opath, id))
  {
    method_return_new_void(call_ptr);
  }
}

static void is_app_running(struct dbus_method_call * call_ptr)
{
  uint64_t id;
  struct ladish_app * app_ptr;
  dbus_bool_t running;

  if (!dbus_message_get_args(
        call_ptr->message,
        &g_dbus_error,
        DBUS_TYPE_UINT64, &id,
        DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  app_ptr = ladish_app_supervisor_find_app_by_id_internal(supervisor_ptr, id);
  if (app_ptr == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "App with ID %"PRIu64" not found", id);
    return;
  }

  running = app_ptr->pid != 0;

  method_return_new_single(call_ptr, DBUS_TYPE_BOOLEAN, &running);
}

#undef supervisor_ptr

METHOD_ARGS_BEGIN(GetAll, "Get list of apps")
  METHOD_ARG_DESCRIBE_OUT("list_version", DBUS_TYPE_UINT64_AS_STRING, "Version of the list")
  METHOD_ARG_DESCRIBE_OUT("apps_list", "a(tsbby)", "List of apps")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(RunCustom, "Start application by supplying commandline")
  METHOD_ARG_DESCRIBE_IN("terminal", DBUS_TYPE_BOOLEAN_AS_STRING, "Whether to run in terminal")
  METHOD_ARG_DESCRIBE_IN("commandline", DBUS_TYPE_STRING_AS_STRING, "Commandline")
  METHOD_ARG_DESCRIBE_IN("name", DBUS_TYPE_STRING_AS_STRING, "Name")
  METHOD_ARG_DESCRIBE_IN("level", DBUS_TYPE_BYTE_AS_STRING, "Level")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(StartApp, "Start application")
  METHOD_ARG_DESCRIBE_IN("id", DBUS_TYPE_UINT64_AS_STRING, "id of app")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(StopApp, "Stop application")
  METHOD_ARG_DESCRIBE_IN("id", DBUS_TYPE_UINT64_AS_STRING, "id of app")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(KillApp, "Kill application")
  METHOD_ARG_DESCRIBE_IN("id", DBUS_TYPE_UINT64_AS_STRING, "id of app")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(RemoveApp, "Remove application")
  METHOD_ARG_DESCRIBE_IN("id", DBUS_TYPE_UINT64_AS_STRING, "id of app")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(GetAppProperties, "Get properties of an application")
  METHOD_ARG_DESCRIBE_IN("id", DBUS_TYPE_UINT64_AS_STRING, "id of app")
  METHOD_ARG_DESCRIBE_OUT("name", DBUS_TYPE_STRING_AS_STRING, "")
  METHOD_ARG_DESCRIBE_OUT("commandline", DBUS_TYPE_STRING_AS_STRING, "Commandline")
  METHOD_ARG_DESCRIBE_OUT("running", DBUS_TYPE_BOOLEAN_AS_STRING, "")
  METHOD_ARG_DESCRIBE_OUT("terminal", DBUS_TYPE_BOOLEAN_AS_STRING, "Whether to run in terminal")
  METHOD_ARG_DESCRIBE_OUT("level", DBUS_TYPE_BYTE_AS_STRING, "Level")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(SetAppProperties, "Set properties of an application")
  METHOD_ARG_DESCRIBE_IN("id", DBUS_TYPE_UINT64_AS_STRING, "id of app")
  METHOD_ARG_DESCRIBE_IN("name", DBUS_TYPE_STRING_AS_STRING, "")
  METHOD_ARG_DESCRIBE_IN("commandline", DBUS_TYPE_STRING_AS_STRING, "Commandline")
  METHOD_ARG_DESCRIBE_IN("terminal", DBUS_TYPE_BOOLEAN_AS_STRING, "Whether to run in terminal")
  METHOD_ARG_DESCRIBE_IN("level", DBUS_TYPE_BYTE_AS_STRING, "Level")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(IsAppRunning, "Check whether application is running")
  METHOD_ARG_DESCRIBE_IN("id", DBUS_TYPE_UINT64_AS_STRING, "id of app")
  METHOD_ARG_DESCRIBE_IN("terminal", DBUS_TYPE_BOOLEAN_AS_STRING, "Whether app is running")
METHOD_ARGS_END


METHODS_BEGIN
  METHOD_DESCRIBE(GetAll, get_all)                      /* sync */
  METHOD_DESCRIBE(RunCustom, run_custom)                /* async */
  METHOD_DESCRIBE(StartApp, start_app)                  /* async */
  METHOD_DESCRIBE(StopApp, stop_app)                    /* async */
  METHOD_DESCRIBE(KillApp, kill_app)                    /* async */
  METHOD_DESCRIBE(GetAppProperties, get_app_properties) /* sync */
  METHOD_DESCRIBE(SetAppProperties, set_app_properties) /* sync */
  METHOD_DESCRIBE(RemoveApp, remove_app)                /* sync */
  METHOD_DESCRIBE(IsAppRunning, is_app_running)         /* sync */
METHODS_END

SIGNAL_ARGS_BEGIN(AppAdded, "")
  SIGNAL_ARG_DESCRIBE("new_list_version", DBUS_TYPE_UINT64_AS_STRING, "")
  SIGNAL_ARG_DESCRIBE("id", DBUS_TYPE_UINT64_AS_STRING, "")
  SIGNAL_ARG_DESCRIBE("name", DBUS_TYPE_STRING_AS_STRING, "")
  SIGNAL_ARG_DESCRIBE("running", DBUS_TYPE_BOOLEAN_AS_STRING, "")
  SIGNAL_ARG_DESCRIBE("terminal", DBUS_TYPE_BOOLEAN_AS_STRING, "Whether to run in terminal")
  SIGNAL_ARG_DESCRIBE("level", DBUS_TYPE_BYTE_AS_STRING, "Level")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(AppRemoved, "")
  SIGNAL_ARG_DESCRIBE("new_list_version", DBUS_TYPE_UINT64_AS_STRING, "")
  SIGNAL_ARG_DESCRIBE("id", DBUS_TYPE_UINT64_AS_STRING, "")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(AppStateChanged, "")
  SIGNAL_ARG_DESCRIBE("id", DBUS_TYPE_UINT64_AS_STRING, "")
  SIGNAL_ARG_DESCRIBE("name", DBUS_TYPE_STRING_AS_STRING, "")
  SIGNAL_ARG_DESCRIBE("running", DBUS_TYPE_BOOLEAN_AS_STRING, "")
  SIGNAL_ARG_DESCRIBE("terminal", DBUS_TYPE_BOOLEAN_AS_STRING, "Whether to run in terminal")
  SIGNAL_ARG_DESCRIBE("level", DBUS_TYPE_BYTE_AS_STRING, "Level")
SIGNAL_ARGS_END

SIGNALS_BEGIN
  SIGNAL_DESCRIBE(AppAdded)
  SIGNAL_DESCRIBE(AppRemoved)
  SIGNAL_DESCRIBE(AppStateChanged)
SIGNALS_END

INTERFACE_BEGIN(g_iface_app_supervisor, IFACE_APP_SUPERVISOR)
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
  INTERFACE_EXPOSE_SIGNALS
INTERFACE_END
