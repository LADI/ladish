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

const char * g_signals[] =
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

  if (!dbus_register_object_signal_handler(g_dbus_connection, SERVICE_NAME, CONTROL_OBJECT_PATH, IFACE_CONTROL, g_signals, message_hook, NULL))
  {
    return false;
  }

  if (!control_proxy_is_studio_loaded(&studio_present))
  {
    dbus_unregister_object_signal_handler(g_dbus_connection, SERVICE_NAME, CONTROL_OBJECT_PATH, IFACE_CONTROL, g_signals, message_hook, NULL);
    return false;
  }

  if (studio_present)
  {
    control_proxy_on_studio_appeared();
  }

  return true;
}

void control_proxy_uninit(void)
{
  dbus_unregister_object_signal_handler(g_dbus_connection, SERVICE_NAME, CONTROL_OBJECT_PATH, IFACE_CONTROL, g_signals, message_hook, NULL);
}

bool control_proxy_get_studio_list(struct list_head * studio_list)
{
  return false;
}

bool control_proxy_load_studio(const char * studio_name)
{
  return false;
}

bool control_proxy_exit(void)
{
  return false;
}
