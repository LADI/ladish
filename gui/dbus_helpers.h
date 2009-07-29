/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to D-Bus helpers
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

#ifndef DBUS_HELPERS_H__4078F17D_E387_4F96_8CAB_FF0BFF83A295__INCLUDED
#define DBUS_HELPERS_H__4078F17D_E387_4F96_8CAB_FF0BFF83A295__INCLUDED

#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* Adjust editor indent */
#endif

void
patchage_dbus_init();

bool
patchage_dbus_call_valist(
  bool response_expected,
  const char* service,
  const char* object,
  const char* iface,
  const char* method,
  DBusMessage** reply_ptr,
  int in_type,
  va_list ap);

bool
patchage_dbus_call(
  bool response_expected,
  const char* service,
  const char* object,
  const char* iface,
  const char* method,
  DBusMessage** reply_ptr,
  int in_type,
  ...);

void
patchage_dbus_add_match(
  const char * rule);

void
patchage_dbus_add_filter(
  DBusHandleMessageFunction function,
  void * user_data);

void
patchage_dbus_uninit();

extern DBusError g_dbus_error;

#if 0
{ /* Adjust editor indent */
#endif
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // #ifndef DBUS_HELPERS_H__4078F17D_E387_4F96_8CAB_FF0BFF83A295__INCLUDED
