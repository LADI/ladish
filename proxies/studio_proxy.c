/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009,2010,2011,2012 Nedko Arnaudov <nedko@arnaudov.name>
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

static void (* g_room_appeared_calback)(const char * opath, const char * name, const char * template) = NULL;
static void (* g_room_disappeared_calback)(const char * opath, const char * name, const char * template) = NULL;
static void (* g_room_changed_calback)(const char * opath, const char * name, const char * template) = NULL;

static void on_studio_renamed(void * UNUSED(context), DBusMessage * message_ptr)
{
  char * name;

  if (!dbus_message_get_args(message_ptr, &cdbus_g_dbus_error, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID))
  {
    log_error("Invalid parameters of StudioRenamed signal: %s",  cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
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

static bool extract_room_info(DBusMessageIter * iter_ptr, const char ** opath, const char ** name, const char ** template)
{
  dbus_message_iter_get_basic(iter_ptr, opath);
  //log_info("opath is \"%s\"", *opath);
  dbus_message_iter_next(iter_ptr);

  if (!cdbus_iter_get_dict_entry_string(iter_ptr, "name", name))
  {
    log_error("dbus_iter_get_dict_entry() failed");
    return false;
  }
  //log_info("name is \"%s\"", *name);

  if (!cdbus_iter_get_dict_entry_string(iter_ptr, "template", template))
  {
    *template = NULL;
  }
  //log_info("template is \"%s\"", *template);

  return true;
}

static bool extract_room_info_from_signal(DBusMessage * message_ptr, const char ** opath, const char ** name, const char ** template)
{
  const char * signature;
  DBusMessageIter iter;

  signature = dbus_message_get_signature(message_ptr);
  if (strcmp(signature, "sa{sv}") != 0)
  {
    log_error("Invalid signature of room signal");
    return false;
  }

  dbus_message_iter_init(message_ptr, &iter);

  return extract_room_info(&iter, opath, name, template);
}

static void on_studio_started(void * UNUSED(context), DBusMessage * UNUSED(message_ptr))
{
  log_info("StudioStarted");

  if (g_started_callback != NULL)
  {
    g_started_callback();
  }
}

static void on_studio_stopped(void * UNUSED(context), DBusMessage * UNUSED(message_ptr))
{
  log_info("StudioStopped");

  if (g_stopped_callback != NULL)
  {
    g_stopped_callback();
  }
}

static void on_studio_crashed(void * UNUSED(context), DBusMessage * UNUSED(message_ptr))
{
  log_info("StudioCrashed");

  if (g_crashed_callback != NULL)
  {
    g_crashed_callback();
  }
}

static void on_room_appeared(void * UNUSED(context), DBusMessage * message_ptr)
{
  const char * opath;
  const char * name;
  const char * template;

  log_info("RoomAppeared");

  if (g_room_appeared_calback != NULL && extract_room_info_from_signal(message_ptr, &opath, &name, &template))
  {
    g_room_appeared_calback(opath, name, template);
  }
}

static void on_room_disappeared(void * UNUSED(context), DBusMessage * message_ptr)
{
  const char * opath;
  const char * name;
  const char * template;

  log_info("RoomDisappeared");

  if (g_room_disappeared_calback != NULL && extract_room_info_from_signal(message_ptr, &opath, &name, &template))
  {
    g_room_disappeared_calback(opath, name, template);
  }
}

static void on_room_changed(void * UNUSED(context), DBusMessage * message_ptr)
{
  const char * opath;
  const char * name;
  const char * template;

  log_info("RoomChanged");

  if (g_room_changed_calback != NULL && extract_room_info_from_signal(message_ptr, &opath, &name, &template))
  {
    g_room_changed_calback(opath, name, template);
  }
}

/* this must be static because it is referenced by the
 * dbus helper layer when hooks are active */
static struct cdbus_signal_hook g_signal_hooks[] =
{
  {"StudioRenamed", on_studio_renamed},
  {"StudioStarted", on_studio_started},
  {"StudioStopped", on_studio_stopped},
  {"StudioCrashed", on_studio_crashed},
  {"RoomAppeared", on_room_appeared},
  {"RoomDisappeared", on_room_disappeared},
  {"RoomChanged", on_room_changed},
  {NULL, NULL}
};

bool studio_proxy_init(void)
{
  if (!cdbus_register_object_signal_hooks(
        cdbus_g_dbus_connection,
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
  cdbus_unregister_object_signal_hooks(cdbus_g_dbus_connection, SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO);
}

bool studio_proxy_get_name(char ** name_ptr)
{
  const char * name;
  if (!cdbus_call(0, SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "GetName", "", "s", &name))
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
  return cdbus_call(0, SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "Rename", "s", &name, "");
}

bool studio_proxy_save(void)
{
  return cdbus_call(0, SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "Save", "", "");
}

bool studio_proxy_save_as(const char * name)
{
  return cdbus_call(0, SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "SaveAs", "s", &name, "");
}

bool studio_proxy_unload(void)
{
  return cdbus_call(0, SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "Unload", "", "");
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
  return cdbus_call(0, SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "Start", "", "");
}

bool studio_proxy_stop(void)
{
  return cdbus_call(0, SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "Stop", "", "");
}

bool studio_proxy_is_started(bool * is_started_ptr)
{
  dbus_bool_t is_started;

  if (!cdbus_call(0, SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "IsStarted", "", "b", &is_started))
  {
    return false;
  }

  *is_started_ptr = is_started;

  return true;
}

void
studio_proxy_set_room_callbacks(
  void (* appeared)(const char * opath, const char * name, const char * template),
  void (* disappeared)(const char * opath, const char * name, const char * template),
  void (* changed)(const char * opath, const char * name, const char * template))
{
  DBusMessage * reply_ptr;
  const char * signature;
  DBusMessageIter top_iter;
  DBusMessageIter array_iter;
  DBusMessageIter struct_iter;
  const char * opath;
  const char * name;
  const char * template;

  g_room_appeared_calback = appeared;
  g_room_disappeared_calback = disappeared;
  g_room_changed_calback = changed;

  if (!cdbus_call(0, SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "GetRoomList", "", NULL, &reply_ptr))
  {
    /* Don't log error if there is no studio loaded */
    if (!cdbus_call_last_error_is_name(DBUS_ERROR_UNKNOWN_METHOD))
    {
      log_error("Cannot fetch studio room list: %s", cdbus_call_last_error_get_message());
    }

    return;
  }

  signature = dbus_message_get_signature(reply_ptr);
  if (strcmp(signature, "a(sa{sv})") != 0)
  {
    log_error("Invalid signature of GetRoomList reply");
    goto unref;
  }

  dbus_message_iter_init(reply_ptr, &top_iter);

  for (dbus_message_iter_recurse(&top_iter, &array_iter);
       dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_INVALID;
       dbus_message_iter_next(&array_iter))
  {
    dbus_message_iter_recurse(&array_iter, &struct_iter);

    if (!extract_room_info(&struct_iter, &opath, &name, &template))
    {
      log_error("extract_room_info() failed.");
      goto unref;
    }

    g_room_appeared_calback(opath, name, template);
  }

unref:
  dbus_message_unref(reply_ptr);
}

bool studio_proxy_create_room(const char * name, const char * template)
{
  return cdbus_call(0, SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "CreateRoom", "ss", &name, &template, "");
}

bool studio_proxy_delete_room(const char * name)
{
  return cdbus_call(0, SERVICE_NAME, STUDIO_OBJECT_PATH, IFACE_STUDIO, "DeleteRoom", "s", &name, "");
}
