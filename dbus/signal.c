/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 **************************************************************************
 * This file contains D-Bus signal helpers
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

#include "../common.h"
#include <stdarg.h>
#include "helpers.h"

struct dbus_signal_msg
{
  DBusConnection * connection;
  DBusMessage * message;
};

static void signal_send(struct dbus_signal_msg * signal)
{
  if (!dbus_connection_send(signal->connection, signal->message, NULL))
  {
    lash_error("Ran out of memory trying to queue signal");
  }

  dbus_connection_flush(signal->connection);
}

void
signal_new_single(
  DBusConnection * connection_ptr,
  const char * path,
  const char * interface,
  const char * name,
  int type,
  const void * arg)
{
  struct dbus_signal_msg signal;
  DBusMessageIter iter;

  lash_debug("Sending signal %s.%s from %s", interface, name, path);

  if ((signal.message = dbus_message_new_signal(path, interface, name))) {
    dbus_message_iter_init_append(signal.message, &iter);
    if (dbus_message_iter_append_basic(&iter, type, arg)) {
      signal.connection = connection_ptr;
      signal_send(&signal);
    } else {
      lash_error("Ran out of memory trying to append signal argument");
    }

    dbus_message_unref(signal.message);
    signal.message = NULL;

    return;
  }

  lash_error("Ran out of memory trying to create new signal");
}

void
signal_new_valist(
  DBusConnection * connection_ptr,
  const char * path,
  const char * interface,
  const char * name,
  int type,
  ...)
{
  struct dbus_signal_msg signal;
  va_list argp;

  lash_debug("Sending signal %s.%s from %s", interface, name, path);

  if ((signal.message = dbus_message_new_signal (path, interface, name))) {
    va_start(argp, type);
    if (dbus_message_append_args_valist(signal.message, type, argp)) {
      signal.connection = connection_ptr;
      signal_send(&signal);
    } else {
      lash_error("Ran out of memory trying to append signal argument(s)");
    }
    va_end(argp);

    dbus_message_unref(signal.message);
    signal.message = NULL;

    return;
  }

  lash_error("Ran out of memory trying to create new signal");
}
