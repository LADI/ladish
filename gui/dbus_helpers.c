/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains D-Bus helpers
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

#include <stdbool.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "dbus_helpers.h"
#include "../dbus/helpers.h"

void
patchage_dbus_init()
{
  dbus_error_init(&g_dbus_error);

  // Connect to the bus
  g_dbus_connection = dbus_bus_get(DBUS_BUS_SESSION, &g_dbus_error);
  if (dbus_error_is_set(&g_dbus_error))
  {
    //error_msg("dbus_bus_get() failed");
    //error_msg(g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
  }

  dbus_connection_setup_with_g_main(g_dbus_connection, NULL);
}

bool
patchage_dbus_call_valist(
  bool response_expected,
  const char * service,
  const char * object,
  const char * iface,
  const char * method,
  DBusMessage ** reply_ptr_ptr,
  int in_type,
  va_list ap)
{
  DBusMessage * request_ptr;
  DBusMessage * reply_ptr;

  request_ptr = dbus_message_new_method_call(
    service,
    object,
    iface,
    method);
  if (!request_ptr)
  {
    //throw std::runtime_error("dbus_message_new_method_call() returned 0");
  }

  dbus_message_append_args_valist(request_ptr, in_type, ap);

  // send message and get a handle for a reply
  reply_ptr = dbus_connection_send_with_reply_and_block(
    g_dbus_connection,
    request_ptr,
    DBUS_CALL_DEFAULT_TIMEOUT,
    &g_dbus_error);

  dbus_message_unref(request_ptr);

  if (!reply_ptr)
  {
    if (response_expected)
    {
      //error_msg("no reply from server when calling method '%s', error is '%s'", method, _error.message);
    }
    dbus_error_free(&g_dbus_error);
  }
  else
  {
    *reply_ptr_ptr = reply_ptr;
  }

  return reply_ptr;
}

bool
patchage_dbus_call(
  bool response_expected,
  const char * service,
  const char * object,
  const char * iface,
  const char * method,
  DBusMessage ** reply_ptr_ptr,
  int in_type,
  ...)
{
  bool ret;
  va_list ap;

  va_start(ap, in_type);

  ret = patchage_dbus_call_valist(
    response_expected,
    service,
    object,
    iface,
    method,
    reply_ptr_ptr,
    in_type,
    ap);

  va_end(ap);

  return (ap != NULL);
}

void
patchage_dbus_add_match(
  const char * rule)
{
  dbus_bus_add_match(g_dbus_connection, rule, NULL);
}

void
patchage_dbus_add_filter(
  DBusHandleMessageFunction function,
  void * user_data)
{
  dbus_connection_add_filter(g_dbus_connection, function, user_data, NULL);
}

void
patchage_dbus_uninit()
{
  if (g_dbus_connection)
  {
    dbus_connection_flush(g_dbus_connection);
  }

  if (dbus_error_is_set(&g_dbus_error))
  {
    dbus_error_free(&g_dbus_error);
  }
}
