/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
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
#include "../dbus/helpers.h"
#include "../dbus_constants.h"

struct ladish_app_supervisor_proxy
{
  char * service;
  char * object;
  uint64_t version;

  void * context;
  void (* app_added)(void * context, uint64_t id, const char * name, bool running, bool terminal, uint8_t level);
  void (* app_state_changed)(void * context, uint64_t id, const char * name, bool running, bool terminal, uint8_t level);
  void (* app_removed)(void * context, uint64_t id);
};

static const char * g_signals[] =
{
  "AppAdded",
  "AppRemoved",
  "AppStateChanged",
  NULL
};

#define proxy_ptr ((struct ladish_app_supervisor_proxy *)proxy)

static
DBusHandlerResult
message_hook(
  DBusConnection * connection,
  DBusMessage * message,
  void * proxy)
{
  const char * object_path;
  uint64_t new_list_version;
  uint64_t id;
  const char * name;
  dbus_bool_t running;
  dbus_bool_t terminal;
  uint8_t level;

  object_path = dbus_message_get_path(message);
  if (object_path == NULL || strcmp(object_path, proxy_ptr->object) != 0)
  {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  if (dbus_message_is_signal(message, IFACE_APP_SUPERVISOR, "AppAdded"))
  {
    if (!dbus_message_get_args(
          message,
          &g_dbus_error,
          DBUS_TYPE_UINT64, &new_list_version,
          DBUS_TYPE_UINT64, &id,
          DBUS_TYPE_STRING, &name,
          DBUS_TYPE_BOOLEAN, &running,
          DBUS_TYPE_BOOLEAN, &terminal,
          DBUS_TYPE_BYTE, &level,
          DBUS_TYPE_INVALID))
    {
      log_error("dbus_message_get_args() failed to extract AppAdded signal arguments (%s)", g_dbus_error.message);
      dbus_error_free(&g_dbus_error);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    //log_info("AppAdded signal received. id=%"PRIu64", name='%s', %srunning, %s, level %u", id, name, running ? "" : "not ", terminal ? "terminal" : "shell", (unsigned int)level);
    proxy_ptr->app_added(proxy_ptr->context, id, name, running, terminal, level);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_signal(message, IFACE_APP_SUPERVISOR, "AppRemoved"))
  {
    if (!dbus_message_get_args(
          message,
          &g_dbus_error,
          DBUS_TYPE_UINT64, &new_list_version,
          DBUS_TYPE_UINT64, &id,
          DBUS_TYPE_INVALID))
    {
      log_error("dbus_message_get_args() failed to extract AppRemoved signal arguments (%s)", g_dbus_error.message);
      dbus_error_free(&g_dbus_error);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    //log_info("AppRemoved signal received, id=%"PRIu64, id);
    proxy_ptr->app_removed(proxy_ptr->context, id);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_signal(message, IFACE_APP_SUPERVISOR, "AppStateChanged"))
  {
    if (!dbus_message_get_args(
          message,
          &g_dbus_error,
          DBUS_TYPE_UINT64, &id,
          DBUS_TYPE_STRING, &name,
          DBUS_TYPE_BOOLEAN, &running,
          DBUS_TYPE_BOOLEAN, &terminal,
          DBUS_TYPE_BYTE, &level,
          DBUS_TYPE_INVALID))
    {
      log_error("dbus_message_get_args() failed to extract AppStateChanged signal arguments (%s)", g_dbus_error.message);
      dbus_error_free(&g_dbus_error);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    //log_info("AppStateChanged signal received");
    proxy_ptr->app_state_changed(proxy_ptr->context, id, name, running, terminal, level);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

#undef proxy_ptr

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
  uint8_t level;

  log_info("refresh_internal() called");

  version = proxy_ptr->version;

  if (!dbus_call(proxy_ptr->service, proxy_ptr->object, IFACE_APP_SUPERVISOR, "GetAll", "t", &version, NULL, &reply_ptr))
  {
    log_error("GetAll() failed.");
    return;
  }

  reply_signature = dbus_message_get_signature(reply_ptr);

  if (strcmp(reply_signature, "ta(tsbby)") != 0)
  {
    log_error("GetAll() reply signature mismatch. '%s'", reply_signature);
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

    //log_info("App id=%"PRIu64", name='%s', %srunning, %s, level %u", id, name, running ? "" : "not ", terminal ? "terminal" : "shell", (unsigned int)level);
    proxy_ptr->app_added(proxy_ptr->context, id, name, running, terminal, level);

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
  void (* app_added)(void * context, uint64_t id, const char * name, bool running, bool terminal, uint8_t level),
  void (* app_state_changed)(void * context, uint64_t id, const char * name, bool running, bool terminal, uint8_t level),
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

  if (!dbus_register_object_signal_handler(
        g_dbus_connection,
        proxy_ptr->service,
        proxy_ptr->object,
        IFACE_APP_SUPERVISOR,
        g_signals,
        message_hook,
        proxy_ptr))
  {
    return false;
  }

  refresh_internal(proxy_ptr, true);

  *handle_ptr = (ladish_app_supervisor_proxy_handle)proxy_ptr;

  return true;

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
  dbus_unregister_object_signal_handler(
    g_dbus_connection,
    proxy_ptr->service,
    proxy_ptr->object,
    IFACE_APP_SUPERVISOR,
    g_signals,
    message_hook,
    proxy_ptr);

  free(proxy_ptr->object);
  free(proxy_ptr->service);
  free(proxy_ptr);
}

bool ladish_app_supervisor_proxy_run_custom(ladish_app_supervisor_proxy_handle proxy, const char * command, const char * name, bool run_in_terminal)
{
  dbus_bool_t terminal;
  if (!dbus_call(proxy_ptr->service, proxy_ptr->object, IFACE_APP_SUPERVISOR, "RunCustom", "bss", &terminal, &command, &name, ""))
  {
    log_error("RunCustom() failed.");
    return false;
  }

  return true;
}

bool ladish_app_supervisor_proxy_start_app(ladish_app_supervisor_proxy_handle proxy, uint64_t id)
{
  if (!dbus_call(proxy_ptr->service, proxy_ptr->object, IFACE_APP_SUPERVISOR, "StartApp", "t", &id, ""))
  {
    log_error("StartApp() failed.");
    return false;
  }

  return true;
}

bool ladish_app_supervisor_proxy_stop_app(ladish_app_supervisor_proxy_handle proxy, uint64_t id)
{
  if (!dbus_call(proxy_ptr->service, proxy_ptr->object, IFACE_APP_SUPERVISOR, "StopApp", "t", &id, ""))
  {
    log_error("StopApp() failed.");
    return false;
  }

  return true;
}

bool ladish_app_supervisor_proxy_kill_app(ladish_app_supervisor_proxy_handle proxy, uint64_t id)
{
  if (!dbus_call(proxy_ptr->service, proxy_ptr->object, IFACE_APP_SUPERVISOR, "KillApp", "t", &id, ""))
  {
    log_error("KillApp() failed.");
    return false;
  }

  return true;
}

bool ladish_app_supervisor_proxy_remove_app(ladish_app_supervisor_proxy_handle proxy, uint64_t id)
{
  if (!dbus_call(proxy_ptr->service, proxy_ptr->object, IFACE_APP_SUPERVISOR, "RemoveApp", "t", &id, ""))
  {
    log_error("RemoveApp() failed.");
    return false;
  }

  return true;
}

#undef proxy_ptr
