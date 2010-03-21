/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
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

static bool g_clean_exit;

static void on_studio_appeared(void * context, DBusMessage * message_ptr)
{
  log_info("StudioAppeared");
  control_proxy_on_studio_appeared(false);
}

static void on_studio_disappeared(void * context, DBusMessage * message_ptr)
{
  log_info("StudioDisappeared");
  control_proxy_on_studio_disappeared();
}

static void on_clean_exit(void * context, DBusMessage * message_ptr)
{
  g_clean_exit = true;
}

/* this must be static because it is referenced by the
 * dbus helper layer when hooks are active */
static struct dbus_signal_hook g_signal_hooks[] =
{
  {"StudioAppeared", on_studio_appeared},
  {"StudioDisappeared", on_studio_disappeared},
  {"CleanExit", on_clean_exit},
  {NULL, NULL}
};

static bool control_proxy_is_studio_loaded(bool * present_ptr)
{
  dbus_bool_t present;

  if (!dbus_call(SERVICE_NAME, CONTROL_OBJECT_PATH, IFACE_CONTROL, "IsStudioLoaded", "", "b", &present))
  {
    return false;
  }

  *present_ptr = present;

  return true;
}

void on_lifestatus_changed(bool appeared)
{
  if (appeared)
  {
    control_proxy_on_daemon_appeared();
  }
  else
  {
    control_proxy_on_daemon_disappeared(g_clean_exit);
  }

  g_clean_exit = false;
}

bool control_proxy_init(void)
{
  bool studio_present;

  g_clean_exit = false;

  if (!control_proxy_is_studio_loaded(&studio_present))
  {
    return true;
  }

  control_proxy_on_daemon_appeared();

  if (!dbus_register_service_lifetime_hook(g_dbus_connection, SERVICE_NAME, on_lifestatus_changed))
  {
    control_proxy_on_daemon_disappeared(true);
    return false;
  }

  if (studio_present)
  {
    log_info("Initial studio appear");
    control_proxy_on_studio_appeared(true);
  }

  if (!dbus_register_object_signal_hooks(g_dbus_connection, SERVICE_NAME, CONTROL_OBJECT_PATH, IFACE_CONTROL, NULL, g_signal_hooks))
  {
    if (studio_present)
    {
      control_proxy_on_studio_disappeared();
    }

    control_proxy_on_daemon_disappeared(true);

    return false;
  }

  return true;
}

void control_proxy_uninit(void)
{
  dbus_unregister_object_signal_hooks(g_dbus_connection, SERVICE_NAME, CONTROL_OBJECT_PATH, IFACE_CONTROL);
  dbus_unregister_service_lifetime_hook(g_dbus_connection, SERVICE_NAME);
}

void control_proxy_ping(void)
{
  bool studio_present;

  control_proxy_is_studio_loaded(&studio_present);
}

bool control_proxy_get_studio_list(void (* callback)(void * context, const char * studio_name), void * context)
{
  DBusMessage * reply_ptr;
  const char * reply_signature;
  DBusMessageIter top_iter;
  DBusMessageIter struct_iter;
  DBusMessageIter array_iter;
  const char * studio_name;

  if (!dbus_call(SERVICE_NAME, CONTROL_OBJECT_PATH, IFACE_CONTROL, "GetStudioList", "", NULL, &reply_ptr))
  {
    log_error("GetStudioList() failed.");
    return false;
  }

  reply_signature = dbus_message_get_signature(reply_ptr);
  if (strcmp(reply_signature, "a(sa{sv})") != 0)
  {
    log_error("GetStudioList() reply signature mismatch. '%s'", reply_signature);
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

bool control_proxy_new_studio(const char * studio_name)
{
  if (studio_name == NULL)
  {
    studio_name = "";
  }

  if (!dbus_call(SERVICE_NAME, CONTROL_OBJECT_PATH, IFACE_CONTROL, "NewStudio", "s", &studio_name, ""))
  {
    log_error("NewStudio() failed.");
    return false;
  }

  return true;
}

bool control_proxy_load_studio(const char * studio_name)
{
  if (!dbus_call(SERVICE_NAME, CONTROL_OBJECT_PATH, IFACE_CONTROL, "LoadStudio", "s", &studio_name, ""))
  {
    log_error("LoadStudio() failed.");
    return false;
  }

  return true;
}

bool control_proxy_delete_studio(const char * studio_name)
{
  if (!dbus_call(SERVICE_NAME, CONTROL_OBJECT_PATH, IFACE_CONTROL, "DeleteStudio", "s", &studio_name, ""))
  {
    log_error("DeleteStudio() failed.");
    return false;
  }

  return true;
}

bool control_proxy_exit(void)
{
  if (!dbus_call(SERVICE_NAME, CONTROL_OBJECT_PATH, IFACE_CONTROL, "Exit", "", ""))
  {
    log_error("Exit() failed.");
    return false;
  }

  return true;
}

bool control_proxy_get_room_template_list(void (* callback)(void * context, const char * template_name), void * context)
{
  DBusMessage * reply_ptr;
  const char * reply_signature;
  DBusMessageIter top_iter;
  DBusMessageIter struct_iter;
  DBusMessageIter array_iter;
  const char * name;

  if (!dbus_call(SERVICE_NAME, CONTROL_OBJECT_PATH, IFACE_CONTROL, "GetRoomTemplateList", "", NULL, &reply_ptr))
  {
    log_error("GetRoomTemplateList() failed.");
    return false;
  }

  reply_signature = dbus_message_get_signature(reply_ptr);
  if (strcmp(reply_signature, "a(sa{sv})") != 0)
  {
    log_error("GetRoomTemplateList() reply signature mismatch. '%s'", reply_signature);
    dbus_message_unref(reply_ptr);
    return false;
  }

  dbus_message_iter_init(reply_ptr, &top_iter);
  for (dbus_message_iter_recurse(&top_iter, &array_iter);
       dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_INVALID;
       dbus_message_iter_next(&array_iter))
  {
    dbus_message_iter_recurse(&array_iter, &struct_iter);
    dbus_message_iter_get_basic(&struct_iter, &name);
    callback(context, name);
    dbus_message_iter_next(&struct_iter);
    dbus_message_iter_next(&struct_iter);
  }

  dbus_message_unref(reply_ptr);
  return true;
}
