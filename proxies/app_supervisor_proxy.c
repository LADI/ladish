/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009,2010,2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of code that interfaces
 * app supervisor object through D-Bus
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

#include "app_supervisor_proxy.h"

struct ladish_app_supervisor_proxy
{
  char * service;
  char * object;
  uint64_t version;

  void * context;
  void (* app_added)(void * context, uint64_t id, const char * name, bool running, bool terminal, const char * level);
  void (* app_state_changed)(void * context, uint64_t id, const char * name, bool running, bool terminal, const char * level);
  void (* app_removed)(void * context, uint64_t id);
};

#define proxy_ptr ((struct ladish_app_supervisor_proxy *)context)

static void on_app_added(void * context, DBusMessage * message_ptr)
{
  uint64_t new_list_version;
  uint64_t id;
  const char * name;
  dbus_bool_t running;
  dbus_bool_t terminal;
  const char * level;

  if (!dbus_message_get_args(
        message_ptr,
        &cdbus_g_dbus_error,
        DBUS_TYPE_UINT64, &new_list_version,
        DBUS_TYPE_UINT64, &id,
        DBUS_TYPE_STRING, &name,
        DBUS_TYPE_BOOLEAN, &running,
        DBUS_TYPE_BOOLEAN, &terminal,
        DBUS_TYPE_STRING, &level,
        DBUS_TYPE_INVALID))
  {
    log_error("dbus_message_get_args() failed to extract AppAdded2 signal arguments (%s)", cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  //log_info("AppAdded2 signal received. id=%"PRIu64", name='%s', %srunning, %s, level '%s'", id, name, running ? "" : "not ", terminal ? "terminal" : "shell", level);

  if (new_list_version <= proxy_ptr->version)
  {
    log_info("Ignoring signal for older version of the app list");
  }
  else
  {
    //log_info("got new list version %llu", (unsigned long long)version);
    proxy_ptr->version = new_list_version;

    level = ladish_map_app_level_constant(level);
    if (level != NULL)
    {
      proxy_ptr->app_added(proxy_ptr->context, id, name, running, terminal, level);
    }
    else
    {
      log_error("Ignoring app added signal for '%s' because of invalid app level value", name);
    }
  }
}

static void on_app_removed(void * context, DBusMessage * message_ptr)
{
  uint64_t new_list_version;
  uint64_t id;

  if (!dbus_message_get_args(
        message_ptr,
        &cdbus_g_dbus_error,
        DBUS_TYPE_UINT64, &new_list_version,
        DBUS_TYPE_UINT64, &id,
        DBUS_TYPE_INVALID))
  {
    log_error("dbus_message_get_args() failed to extract AppRemoved signal arguments (%s)", cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  //log_info("AppRemoved signal received, id=%"PRIu64, id);
  if (new_list_version <= proxy_ptr->version)
  {
    log_info("Ignoring signal for older version of the app list");
  }
  else
  {
    //log_info("got new list version %llu", (unsigned long long)version);
    proxy_ptr->version = new_list_version;
    proxy_ptr->app_removed(proxy_ptr->context, id);
  }
}

static void on_app_state_changed(void * context, DBusMessage * message_ptr)
{
  uint64_t new_list_version;
  uint64_t id;
  const char * name;
  dbus_bool_t running;
  dbus_bool_t terminal;
  const char * level;

  if (!dbus_message_get_args(
        message_ptr,
        &cdbus_g_dbus_error,
        DBUS_TYPE_UINT64, &new_list_version,
        DBUS_TYPE_UINT64, &id,
        DBUS_TYPE_STRING, &name,
        DBUS_TYPE_BOOLEAN, &running,
        DBUS_TYPE_BOOLEAN, &terminal,
        DBUS_TYPE_STRING, &level,
        DBUS_TYPE_INVALID))
  {
    log_error("dbus_message_get_args() failed to extract AppStateChanged2 signal arguments (%s)", cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  //log_info("AppStateChanged2 signal received");
  //log_info("AppRemoved signal received, id=%"PRIu64, id);
  if (new_list_version <= proxy_ptr->version)
  {
    log_info("Ignoring signal for older version of the app list");
  }
  else
  {
    //log_info("got new list version %llu", (unsigned long long)version);
    proxy_ptr->version = new_list_version;

    level = ladish_map_app_level_constant(level);
    if (level != NULL)
    {
      proxy_ptr->app_state_changed(proxy_ptr->context, id, name, running, terminal, level);
    }
    else
    {
      log_error("Ignoring app state changed signal for '%s' because of invalid app level value", name);
    }
  }
}

#undef proxy_ptr

/* this must be static because it is referenced by the
 * dbus helper layer when hooks are active */
static struct cdbus_signal_hook g_signal_hooks[] =
{
  {"AppAdded2", on_app_added},
  {"AppRemoved", on_app_removed},
  {"AppStateChanged2", on_app_state_changed},
  {NULL, NULL}
};

static void refresh_internal(struct ladish_app_supervisor_proxy * proxy_ptr, bool force)
{
  DBusMessage* reply_ptr;
  DBusMessageIter iter;
  dbus_uint64_t version;
  const char * reply_signature;
  DBusMessageIter array_iter;
  DBusMessageIter struct_iter;
  uint64_t id;
  const char * name;
  dbus_bool_t running;
  dbus_bool_t terminal;
  const char * level;

  log_info("refresh_internal() called");

  version = proxy_ptr->version;

  if (!cdbus_call(0, proxy_ptr->service, proxy_ptr->object, IFACE_APP_SUPERVISOR, "GetAll2", "t", &version, NULL, &reply_ptr))
  {
    log_error("GetAll2() failed.");
    return;
  }

  reply_signature = dbus_message_get_signature(reply_ptr);

  if (strcmp(reply_signature, "ta(tsbbs)") != 0)
  {
    log_error("GetAll2() reply signature mismatch. '%s'", reply_signature);
    goto unref;
  }

  dbus_message_iter_init(reply_ptr, &iter);

  //log_info_msg("version " + (char)dbus_message_iter_get_arg_type(&iter));
  dbus_message_iter_get_basic(&iter, &version);
  dbus_message_iter_next(&iter);

  if (!force && version <= proxy_ptr->version)
  {
    goto unref;
  }

  //log_info("got new list version %llu", (unsigned long long)version);
  proxy_ptr->version = version;

  for (dbus_message_iter_recurse(&iter, &array_iter);
       dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_INVALID;
       dbus_message_iter_next(&array_iter))
  {
    dbus_message_iter_recurse(&array_iter, &struct_iter);

    dbus_message_iter_get_basic(&struct_iter, &id);
    dbus_message_iter_next(&struct_iter);

    dbus_message_iter_get_basic(&struct_iter, &name);
    dbus_message_iter_next(&struct_iter);

    dbus_message_iter_get_basic(&struct_iter, &running);
    dbus_message_iter_next(&struct_iter);

    dbus_message_iter_get_basic(&struct_iter, &terminal);
    dbus_message_iter_next(&struct_iter);

    dbus_message_iter_get_basic(&struct_iter, &level);
    dbus_message_iter_next(&struct_iter);

    level = ladish_map_app_level_constant(level);
    if (level != NULL)
    {
      //log_info("App id=%"PRIu64", name='%s', %srunning, %s, level '%s'", id, name, running ? "" : "not ", terminal ? "terminal" : "shell", level);
      proxy_ptr->app_added(proxy_ptr->context, id, name, running, terminal, level);
    }
    else
    {
      log_error("Ignoring app '%s' because of invalid app level value", name);
    }

    dbus_message_iter_next(&struct_iter);
  }

unref:
  dbus_message_unref(reply_ptr);
}

bool
ladish_app_supervisor_proxy_create(
  const char * service,
  const char * object,
  void * context,
  void (* app_added)(void * context, uint64_t id, const char * name, bool running, bool terminal, const char * level),
  void (* app_state_changed)(void * context, uint64_t id, const char * name, bool running, bool terminal, const char * level),
  void (* app_removed)(void * context, uint64_t id),
  ladish_app_supervisor_proxy_handle * handle_ptr)
{
  struct ladish_app_supervisor_proxy * proxy_ptr;

  proxy_ptr = malloc(sizeof(struct ladish_app_supervisor_proxy));
  if (proxy_ptr == NULL)
  {
    log_error("malloc() failed to allocate struct proxy");
    goto fail;
  }

  proxy_ptr->service = strdup(service);
  if (proxy_ptr->service == NULL)
  {
    log_error("strdup() failed too duplicate service name '%s'", service);
    goto free_proxy;
  }

  proxy_ptr->object = strdup(object);
  if (proxy_ptr->object == NULL)
  {
    log_error("strdup() failed too duplicate object name '%s'", object);
    goto free_service;
  }

  proxy_ptr->version = 0;

  proxy_ptr->context = context;
  proxy_ptr->app_added = app_added;
  proxy_ptr->app_state_changed = app_state_changed;
  proxy_ptr->app_removed = app_removed;

  if (!cdbus_register_object_signal_hooks(
        cdbus_g_dbus_connection,
        proxy_ptr->service,
        proxy_ptr->object,
        IFACE_APP_SUPERVISOR,
        proxy_ptr,
        g_signal_hooks))
  {
    log_error("dbus_register_object_signal_hooks() failed for app supervisor");
    goto free_object;
  }

  refresh_internal(proxy_ptr, true);

  *handle_ptr = (ladish_app_supervisor_proxy_handle)proxy_ptr;

  return true;

free_object:
  free(proxy_ptr->object);
free_service:
  free(proxy_ptr->service);
free_proxy:
  free(proxy_ptr);
fail:
  return false;
}

#define proxy_ptr ((struct ladish_app_supervisor_proxy *)proxy)

void ladish_app_supervisor_proxy_destroy(ladish_app_supervisor_proxy_handle proxy)
{
  cdbus_unregister_object_signal_hooks(cdbus_g_dbus_connection, proxy_ptr->service, proxy_ptr->object, IFACE_APP_SUPERVISOR);

  free(proxy_ptr->object);
  free(proxy_ptr->service);
  free(proxy_ptr);
}

bool
ladish_app_supervisor_proxy_run_custom(
  ladish_app_supervisor_proxy_handle proxy,
  const char * command,
  const char * name,
  bool run_in_terminal,
  const char * level)
{
  dbus_bool_t terminal;

  terminal = run_in_terminal;

  if (!cdbus_call(0, proxy_ptr->service, proxy_ptr->object, IFACE_APP_SUPERVISOR, "RunCustom2", "bsss", &terminal, &command, &name, &level, ""))
  {
    log_error("RunCustom2() failed.");
    return false;
  }

  return true;
}

bool ladish_app_supervisor_proxy_start_app(ladish_app_supervisor_proxy_handle proxy, uint64_t id)
{
  if (!cdbus_call(0, proxy_ptr->service, proxy_ptr->object, IFACE_APP_SUPERVISOR, "StartApp", "t", &id, ""))
  {
    log_error("StartApp() failed.");
    return false;
  }

  return true;
}

bool ladish_app_supervisor_proxy_stop_app(ladish_app_supervisor_proxy_handle proxy, uint64_t id)
{
  if (!cdbus_call(0, proxy_ptr->service, proxy_ptr->object, IFACE_APP_SUPERVISOR, "StopApp", "t", &id, ""))
  {
    log_error("StopApp() failed.");
    return false;
  }

  return true;
}

bool ladish_app_supervisor_proxy_kill_app(ladish_app_supervisor_proxy_handle proxy, uint64_t id)
{
  if (!cdbus_call(0, proxy_ptr->service, proxy_ptr->object, IFACE_APP_SUPERVISOR, "KillApp", "t", &id, ""))
  {
    log_error("KillApp() failed.");
    return false;
  }

  return true;
}

bool ladish_app_supervisor_proxy_remove_app(ladish_app_supervisor_proxy_handle proxy, uint64_t id)
{
  if (!cdbus_call(0, proxy_ptr->service, proxy_ptr->object, IFACE_APP_SUPERVISOR, "RemoveApp", "t", &id, ""))
  {
    log_error("RemoveApp() failed.");
    return false;
  }

  return true;
}

bool
ladish_app_supervisor_get_app_properties(
  ladish_app_supervisor_proxy_handle proxy,
  uint64_t id,
  char ** name_ptr_ptr,
  char ** command_ptr_ptr,
  bool * running_ptr,
  bool * terminal_ptr,
  const char ** level_ptr)
{
  DBusMessage * reply_ptr;
  const char * name;
  const char * commandline;
  dbus_bool_t running;
  dbus_bool_t terminal;
  const char * level;
  char * name_buffer;
  char * commandline_buffer;

  if (!cdbus_call(0, proxy_ptr->service, proxy_ptr->object, IFACE_APP_SUPERVISOR, "GetAppProperties2", "t", &id, NULL, &reply_ptr))
  {
    log_error("GetAppProperties2() failed.");
    return false;
  }

  if (!dbus_message_get_args(
        reply_ptr,
        &cdbus_g_dbus_error,
        DBUS_TYPE_STRING, &name,
        DBUS_TYPE_STRING, &commandline,
        DBUS_TYPE_BOOLEAN, &running,
        DBUS_TYPE_BOOLEAN, &terminal,
        DBUS_TYPE_STRING, &level,
        DBUS_TYPE_INVALID))
  {
    dbus_message_unref(reply_ptr);
    dbus_error_free(&cdbus_g_dbus_error);
    log_error("decoding reply of GetAppProperties failed.");
    return false;
  }

  level = ladish_map_app_level_constant(level);
  if (level == NULL)
  {
    dbus_message_unref(reply_ptr);
    log_error("decoding reply of GetAppProperties failed.");
    return false;
  }

  name_buffer = strdup(name);
  if (name_buffer == NULL)
  {
    log_error("strdup() for app name failed.");
    dbus_message_unref(reply_ptr);
    return false;
  }

  commandline_buffer = strdup(commandline);
  if (commandline_buffer == NULL)
  {
    log_error("strdup() for app commandline failed.");
    free(name_buffer);
    dbus_message_unref(reply_ptr);
    return false;
  }

  *name_ptr_ptr = name_buffer;
  *command_ptr_ptr = commandline_buffer;
  *running_ptr = running;
  *terminal_ptr = terminal;
  *level_ptr = level;

  dbus_message_unref(reply_ptr);

  return true;
}

bool
ladish_app_supervisor_set_app_properties(
  ladish_app_supervisor_proxy_handle proxy,
  uint64_t id,
  const char * name,
  const char * command,
  bool run_in_terminal,
  const char * level)
{
  dbus_bool_t terminal;

  terminal = run_in_terminal;

  if (!cdbus_call(
        0,
        proxy_ptr->service,
        proxy_ptr->object,
        IFACE_APP_SUPERVISOR,
        "SetAppProperties2",
        "tssbs",
        &id,
        &name,
        &command,
        &terminal,
        &level,
        ""))
  {
    log_error("SetAppProperties2() failed.");
    return false;
  }

  return true;
}

#undef proxy_ptr
