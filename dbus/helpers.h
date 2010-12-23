/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 **************************************************************************
 * This file contains interface to the D-Bus helpers code
 **************************************************************************
 *
 * Licensed under the Academic Free License version 2.1
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

#ifndef HELPERS_H__6C2107A6_A5E3_4806_869B_4BE609535BA2__INCLUDED
#define HELPERS_H__6C2107A6_A5E3_4806_869B_4BE609535BA2__INCLUDED

#include <dbus/dbus.h>

extern DBusConnection * g_dbus_connection;
extern DBusError g_dbus_error;

bool dbus_iter_get_dict_entry(DBusMessageIter * iter_ptr, const char * key, void * value, int * type, int * size);
bool dbus_iter_get_dict_entry_string(DBusMessageIter * iter_ptr, const char * key, const char ** value);
bool dbus_iter_append_variant(DBusMessageIter * iter, int type, const void * arg);
bool dbus_iter_append_dict_entry(DBusMessageIter *iter, int type, const char * key, const void * value, int length);
bool dbus_maybe_add_dict_entry_string(DBusMessageIter *dict_iter_ptr, const char * key, const char * value);
bool dbus_add_dict_entry_uint32(DBusMessageIter * dict_iter_ptr, const char * key, dbus_uint32_t value);
bool dbus_add_dict_entry_bool(DBusMessageIter * dict_iter_ptr, const char * key, dbus_bool_t value);

DBusMessage *
dbus_call_raw(
  unsigned int timeout,         /* in milliseconds */
  DBusMessage * request_ptr);

bool
dbus_call(
  unsigned int timeout,         /* in milliseconds */
  const char * service,
  const char * object,
  const char * iface,
  const char * method,
  const char * input_signature,
  ...);

bool
dbus_register_object_signal_handler(
  DBusConnection * connection,
  const char * service,
  const char * object,
  const char * iface,
  const char * const * signals,
  DBusHandleMessageFunction handler,
  void * handler_data);

bool
dbus_unregister_object_signal_handler(
  DBusConnection * connection,
  const char * service,
  const char * object,
  const char * iface,
  const char * const * signals,
  DBusHandleMessageFunction handler,
  void * handler_data);

struct dbus_signal_hook
{
  const char * signal_name;
  void (* hook_function)(void * context, DBusMessage * message_ptr);
};

bool
dbus_register_object_signal_hooks(
  DBusConnection * connection,
  const char * service,
  const char * object,
  const char * iface,
  void * hook_context,
  const struct dbus_signal_hook * signal_hooks);

void
dbus_unregister_object_signal_hooks(
  DBusConnection * connection,
  const char * service,
  const char * object,
  const char * iface);

bool
dbus_register_service_lifetime_hook(
  DBusConnection * connection,
  const char * service,
  void (* hook_function)(bool appeared));

void
dbus_unregister_service_lifetime_hook(
  DBusConnection * connection,
  const char * service);

void dbus_call_last_error_cleanup(void);
bool dbus_call_last_error_is_name(const char * name);
const char * dbus_call_last_error_get_message(void);

#include "method.h"
#include "signal.h"
#include "interface.h"
#include "object_path.h"

#endif /* #ifndef HELPERS_H__6C2107A6_A5E3_4806_869B_4BE609535BA2__INCLUDED */
