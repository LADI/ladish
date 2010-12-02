/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the code for sending notifications to user
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

#include "notify_proxy.h"

#define NOTIFY_SERVICE "org.freedesktop.Notifications"
#define NOTIFY_OBJECT  "/org/freedesktop/Notifications"
#define NOTIFY_IFACE   "org.freedesktop.Notifications"
#define NOTIFY_METHOD_NOTIFY "Notify"

static char * g_notify_app_name;

bool ladish_notify_init(const char * app_name)
{
  const char * name;
  const char * vendor;
  const char * version;
  const char * spec_version;

  g_notify_app_name = strdup(app_name);
  if (g_notify_app_name == NULL)
  {
    log_error("strdup() failed for app name");
    return false;
  }

  if (dbus_call(0, NOTIFY_SERVICE, NOTIFY_OBJECT, NOTIFY_IFACE, "GetServerInformation", "", "ssss", &name, &vendor, &version, &spec_version))
  {
    log_info("Sending notifications to '%s' '%s' (%s, %s)", vendor, name, version, spec_version);
  }

  return true;
}

void ladish_notify_uninit(void)
{
  free(g_notify_app_name);
  g_notify_app_name = NULL;
}

void ladish_notify_simple(uint8_t urgency, const char * summary, const char * body)
{
  DBusMessage * request_ptr;
  /* DBusMessage * reply_ptr; */
  DBusMessageIter iter;
  DBusMessageIter array_iter;
  DBusMessageIter dict_iter;
  const char * str_value;
  uint32_t uint32_value;
  int32_t int32_value;

  if (g_notify_app_name == NULL)
  {
    /* notifications are disabled */
    return;
  }

  request_ptr = dbus_message_new_method_call(NOTIFY_SERVICE, NOTIFY_OBJECT, NOTIFY_IFACE, NOTIFY_METHOD_NOTIFY);
  if (request_ptr == NULL)
  {
    log_error("dbus_message_new_method_call() failed.");
    goto exit;
  }

  dbus_message_iter_init_append(request_ptr, &iter);

  /* app_name */
  str_value = g_notify_app_name;
  if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &str_value))
  {
    log_error("dbus_message_iter_append_basic() failed.");
    goto free_request;
  }

  /* replaces_id */
  uint32_value = 0;
  if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT32, &uint32_value))
  {
    log_error("dbus_message_iter_append_basic() failed.");
    goto free_request;
  }

  /* app_icon */
  str_value = "";
  if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &str_value))
  {
    log_error("dbus_message_iter_append_basic() failed.");
    goto free_request;
  }

  /* summary */
  str_value = summary;
  if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &str_value))
  {
    log_error("dbus_message_iter_append_basic() failed.");
    goto free_request;
  }

  /* body */
  str_value = body == NULL ? "" : body;
  if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &str_value))
  {
    log_error("dbus_message_iter_append_basic() failed.");
    goto free_request;
  }

  /* actions */
  if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING_AS_STRING, &array_iter))
  {
    log_error("dbus_message_iter_open_container() failed.");
    goto free_request;
  }

  if (!dbus_message_iter_close_container(&iter, &array_iter))
  {
    log_error("dbus_message_iter_close_container() failed.");
    goto free_request;
  }

  /* hints */
  if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter))
  {
    log_error("dbus_message_iter_open_container() failed.");
    goto free_request;
  }

  if (!dbus_iter_append_dict_entry(&dict_iter, DBUS_TYPE_BYTE, "urgency", &urgency, 0))
  {
    log_error("dbus_iter_append_dict_entry() failed.");
    goto free_request;
  }

  if (!dbus_message_iter_close_container(&iter, &dict_iter))
  {
    log_error("dbus_message_iter_close_container() failed.");
    goto free_request;
  }

  /* expire_timeout */
  int32_value = urgency == LADISH_NOTIFY_URGENCY_HIGH ? 0 : -1;
  if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &int32_value))
  {
    log_error("dbus_message_iter_append_basic() failed.");
    goto free_request;
  }

  if (!dbus_call(0, NOTIFY_SERVICE, NOTIFY_OBJECT, NOTIFY_IFACE, NOTIFY_METHOD_NOTIFY, NULL, request_ptr, "u", &uint32_value))
  {
    //log_error("Notify() dbus call failed.");
    goto free_request;
  }

  //log_info("notify ID is %"PRIu32, uint32_value);

free_request:
  dbus_message_unref(request_ptr);
exit:
  return;
}
