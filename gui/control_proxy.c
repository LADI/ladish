/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of code that interfaces
 * the ladishd Control object through D-Bus
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

#include "control_proxy.h"
#include "../dbus/helpers.h"
#include "../dbus_constants.h"

static const char * g_signals[] =
{
  "StudioAppeared",
  "StudioDisappeared",
  NULL
};

static DBusHandlerResult message_hook(DBusConnection * connection, DBusMessage * message, void * data)
{
  const char * object_path;

  object_path = dbus_message_get_path(message);
  if (object_path == NULL || strcmp(object_path, CONTROL_OBJECT_PATH) != 0)
  {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  if (dbus_message_is_signal(message, IFACE_CONTROL, "StudioAppeared"))
  {
    lash_info("StudioAppeared");
    control_proxy_on_studio_appeared();
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_signal(message, IFACE_CONTROL, "StudioDisappeared"))
  {
    lash_info("StudioDisappeared");
    control_proxy_on_studio_disappeared();
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static bool control_proxy_is_studio_loaded(bool * present_ptr)
{
  dbus_bool_t present;

  if (!dbus_call_simple(SERVICE_NAME, CONTROL_OBJECT_PATH, IFACE_CONTROL, "IsStudioLoaded", "", "b", &present))
  {
    return false;
  }

  *present_ptr = present;

  return true;
}

bool control_proxy_init(void)
{
  bool studio_present;

  if (!control_proxy_is_studio_loaded(&studio_present))
  {
    return false;
  }

  if (studio_present)
  {
    lash_info("Initial studio appear");
    control_proxy_on_studio_appeared();
  }

  if (!dbus_register_object_signal_handler(g_dbus_connection, SERVICE_NAME, CONTROL_OBJECT_PATH, IFACE_CONTROL, g_signals, message_hook, NULL))
  {
    if (studio_present)
    {
      control_proxy_on_studio_disappeared();
    }

    return false;
  }

  return true;
}

void control_proxy_uninit(void)
{
  dbus_unregister_object_signal_handler(g_dbus_connection, SERVICE_NAME, CONTROL_OBJECT_PATH, IFACE_CONTROL, g_signals, message_hook, NULL);
}

bool control_proxy_get_studio_list(void (* callback)(void * context, const char * studio_name), void * context)
{
  DBusMessage * reply_ptr;
  const char * reply_signature;
  DBusMessageIter top_iter;
  DBusMessageIter struct_iter;
  DBusMessageIter array_iter;
  const char * studio_name;

  if (!dbus_call_simple(SERVICE_NAME, CONTROL_OBJECT_PATH, IFACE_CONTROL, "GetStudioList", "", NULL, &reply_ptr))
  {
    lash_error("GetStudioList() failed.");
    return false;
  }

  reply_signature = dbus_message_get_signature(reply_ptr);
  if (strcmp(reply_signature, "a(sa{sv})") != 0)
  {
    lash_error("GetStudioList() reply signature mismatch. '%s'", reply_signature);
    dbus_message_unref(reply_ptr);
    return false;
  }

  dbus_message_iter_init(reply_ptr, &top_iter);
  for (dbus_message_iter_recurse(&top_iter, &array_iter);
       dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_INVALID;
       dbus_message_iter_next(&array_iter))
  {
    dbus_message_iter_recurse(&array_iter, &struct_iter);
    dbus_message_iter_get_basic(&struct_iter, &studio_name);
    callback(context, studio_name);
    dbus_message_iter_next(&struct_iter);
    dbus_message_iter_next(&struct_iter);
  }

  dbus_message_unref(reply_ptr);
  return true;
}

bool control_proxy_load_studio(const char * studio_name)
{
  if (!dbus_call_simple(SERVICE_NAME, CONTROL_OBJECT_PATH, IFACE_CONTROL, "LoadStudio", "s", &studio_name, ""))
  {
    lash_error("LoadStudio() failed.");
    return false;
  }

  return true;
}

bool control_proxy_exit(void)
{
  if (!dbus_call_simple(SERVICE_NAME, CONTROL_OBJECT_PATH, IFACE_CONTROL, "Exit", "", ""))
  {
    lash_error("Exit() failed.");
    return false;
  }

  return true;
}
