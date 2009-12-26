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

static void (* g_renamed_callback)(const char * new_studio_name) = NULL;
static void (* g_started_callback)(void) = NULL;
static void (* g_stopped_callback)(void) = NULL;
static void (* g_crashed_callback)(void) = NULL;

static void on_studio_renamed(void * context, DBusMessage * message_ptr)
{
  char * name;

  if (!dbus_message_get_args(message_ptr, &g_dbus_error, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID))
  {
    log_error("Invalid parameters of StudioRenamed signal: %s",  g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
  }
  else
  {
    log_info("StudioRenamed");

    if (g_renamed_callback != NULL)
    {
      g_renamed_callback(name);
    }
  }
}

static void on_studio_started(void * context, DBusMessage * message_ptr)
{
  log_info("StudioStarted");

  if (g_started_callback != NULL)
  {
    g_started_callback();
  }
}

static void on_studio_stopped(void * context, DBusMessage * message_ptr)
{
  log_info("StudioStopped");

  if (g_stopped_callback != NULL)
  {
    g_stopped_callback();
  }
}

static void on_studio_crashed(void * context, DBusMessage * message_ptr)
{
  log_info("StudioCrashed");

  if (g_crashed_callback != NULL)
  {
    g_crashed_callback();
  }
}

static void on_room_appeared(void * context, DBusMessage * message_ptr)
{
  log_info("RoomAppeared");
}

static void on_room_disappeared(void * context, DBusMessage * message_ptr)
{
  log_info("RoomDisappeared");
}

/* this must be static because it is referenced by the
 * dbus helper layer when hooks are active */
static struct dbus_signal_hook g_signal_hooks[] =
{
  {"StudioRenamed", on_studio_renamed},
  {"StudioStarted", on_studio_started},
  {"StudioStopped", on_studio_stopped},
  {"StudioCrashed", on_studio_crashed},
  {"RoomAppeared", on_room_appeared},
  {"RoomDisappeared", on_room_disappeared},
  {NULL, NULL}
};

bool studio_proxy_init(void)
{
  if (!dbus_register_object_signal_hooks(
        g_dbus_connection,
        SERVICE_NAME,
        STUDIO_OBJECT_PATH,
        IFACE_STUDIO,
        NULL,
        g_signal_hooks))
  {
    log_error("dbus_register_object_signal_hooks() failed");
    return false;
  }

  return true;
}

void studio_proxy_uninit(void)
{
  dbus_unregister_object_signal_hooks(g_dbus_connection, SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO);
}

bool studio_proxy_get_name(char ** name_ptr)
{
  const char * name;
  if (!dbus_call(SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "GetName", "", "s", &name))
  {
    return false;
  }

  *name_ptr = strdup(name);
  if (*name_ptr == NULL)
  {
    log_error("strdup() failed to duplicate studio name");
    return false;
  }

  return true;
}

bool studio_proxy_rename(const char * name)
{
  return dbus_call(SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "Rename", "s", &name, "");
}

bool studio_proxy_save(void)
{
  return dbus_call(SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "Save", "", "");
}

bool studio_proxy_unload(void)
{
  return dbus_call(SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "Unload", "", "");
}

void studio_proxy_set_renamed_callback(void (* callback)(const char * new_studio_name))
{
  g_renamed_callback = callback;
}

void studio_proxy_set_startstop_callbacks(void (* started_callback)(void), void (* stopped_callback)(void), void (* crashed_callback)(void))
{
  g_started_callback = started_callback;
  g_stopped_callback = stopped_callback;
  g_crashed_callback = crashed_callback;
}

bool studio_proxy_start(void)
{
  return dbus_call(SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "Start", "", "");
}

bool studio_proxy_stop(void)
{
  return dbus_call(SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "Stop", "", "");
}

bool studio_proxy_is_started(bool * is_started_ptr)
{
  dbus_bool_t is_started;

  if (!dbus_call(SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "IsStarted", "", "b", &is_started))
  {
    return false;
  }

  *is_started_ptr = is_started;

  return true;
}
