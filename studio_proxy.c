/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the helper functionality
 * for accessing Studio object through D-Bus
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

#include "common.h"
#include "dbus_constants.h"
#include "dbus/helpers.h"

static void (* g_renamed_callback)(const char * new_studio_name) = NULL;
static void (* g_started_callback)(void) = NULL;
static void (* g_stopped_callback)(void) = NULL;

static const char * g_signals[] =
{
  "StudioRenamed",
  "StudioStarted",
  "StudioStopped",
  "RoomAppeared",
  "RoomDisappeared",
  NULL
};

static DBusHandlerResult message_hook(DBusConnection * connection, DBusMessage * message, void * data)
{
  const char * object_path;
  char * name;

  object_path = dbus_message_get_path(message);
  if (object_path == NULL || strcmp(object_path, STUDIO_OBJECT_PATH) != 0)
  {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  if (dbus_message_is_signal(message, IFACE_STUDIO, "StudioRenamed"))
  {
    if (!dbus_message_get_args(message, &g_dbus_error, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID))
    {
      lash_error("Invalid parameters of StudioRenamed signal: %s",  g_dbus_error.message);
      dbus_error_free(&g_dbus_error);
    }
    else
    {
      lash_info("StudioRenamed");

      if (g_renamed_callback != NULL)
      {
        g_renamed_callback(name);
      }
    }

    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_signal(message, IFACE_STUDIO, "StudioStarted"))
  {
    lash_info("StudioStarted");

    if (g_started_callback != NULL)
    {
      g_started_callback();
    }

    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_signal(message, IFACE_STUDIO, "StudioStopped"))
  {
    lash_info("StudioStopped");

    if (g_stopped_callback != NULL)
    {
      g_stopped_callback();
    }

    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_signal(message, IFACE_STUDIO, "RoomAppeared"))
  {
    lash_info("RoomAppeared");
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_signal(message, IFACE_STUDIO, "RoomDisappeared"))
  {
    lash_info("RoomDisappeared");
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

bool studio_proxy_init(void)
{
  if (!dbus_register_object_signal_handler(
        g_dbus_connection,
        SERVICE_NAME,
        STUDIO_OBJECT_PATH,
        IFACE_STUDIO,
        g_signals,
        message_hook,
        NULL))
  {
    lash_error("studio_object_path() failed");
    return false;
  }

  return true;
}

void studio_proxy_uninit(void)
{
  if (!dbus_unregister_object_signal_handler(
        g_dbus_connection,
        SERVICE_NAME,
        STUDIO_OBJECT_PATH,
        IFACE_STUDIO,
        g_signals,
        message_hook,
        NULL))
  {
    lash_error("studio_object_path() failed");
  }
}

bool studio_proxy_get_name(char ** name_ptr)
{
  const char * name;
  if (!dbus_call_simple(SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "GetName", "", "s", &name))
  {
    return false;
  }

  *name_ptr = strdup(name);
  if (*name_ptr == NULL)
  {
    lash_error("strdup() failed to duplicate studio name");
    return false;
  }

  return true;
}

bool studio_proxy_rename(const char * name)
{
  return dbus_call_simple(SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "Rename", "s", &name, "");
}

bool studio_proxy_save(void)
{
  return dbus_call_simple(SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "Save", "", "");
}

bool studio_proxy_unload(void)
{
  return dbus_call_simple(SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "Unload", "", "");
}

void studio_proxy_set_renamed_callback(void (* callback)(const char * new_studio_name))
{
  g_renamed_callback = callback;
}

void studio_proxy_set_startstop_callbacks(void (* started_callback)(void), void (* stopped_callback)(void))
{
  g_started_callback = started_callback;
  g_stopped_callback = stopped_callback;
}

bool studio_proxy_start(void)
{
  return dbus_call_simple(SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "Start", "", "");
}

bool studio_proxy_stop(void)
{
  return dbus_call_simple(SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "Stop", "", "");
}
