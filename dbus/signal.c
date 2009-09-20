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

static void dbus_signal_send(DBusConnection * connection_ptr, DBusMessage * message_ptr)
{
  if (!dbus_connection_send(connection_ptr, message_ptr, NULL))
  {
    log_error("Ran out of memory trying to queue signal");
  }

  dbus_connection_flush(connection_ptr);
}

void
dbus_signal_emit(
  DBusConnection * connection_ptr,
  const char * path,
  const char * interface,
  const char * name,
  const char * signature,
  ...)
{
  DBusMessage * message_ptr;
  va_list ap;
  int type;
  DBusSignatureIter sig_iter;
  DBusMessageIter iter;
  void * parameter_ptr;

  log_debug("Sending signal %s.%s from %s", interface, name, path);

  va_start(ap, signature);

  if (signature != NULL)
  {
    if (!dbus_signature_validate(signature, NULL))
    {
      log_error("signature '%s' is invalid", signature);
      goto exit;
    }

    dbus_signature_iter_init(&sig_iter, signature);

    message_ptr = dbus_message_new_signal(path, interface, name);
    if (message_ptr == NULL)
    {
      log_error("dbus_message_new_signal() failed.");
      goto exit;
    }

    dbus_message_iter_init_append(message_ptr, &iter);

    while (*signature != '\0')
    {
      type = dbus_signature_iter_get_current_type(&sig_iter);
      if (!dbus_type_is_basic(type))
      {
        log_error("non-basic input parameter '%c' (%d)", *signature, type);
        goto unref;
      }

      parameter_ptr = va_arg(ap, void *);

      if (!dbus_message_iter_append_basic(&iter, type, parameter_ptr))
      {
        log_error("dbus_message_iter_append_basic() failed.");
        goto unref;
      }

      dbus_signature_iter_next(&sig_iter);
      signature++;
    }

    dbus_signal_send(connection_ptr, message_ptr);

  unref:
    dbus_message_unref(message_ptr);
  }
  else
  {
    message_ptr = va_arg(ap, DBusMessage *);
    dbus_signal_send(connection_ptr, message_ptr);
  }

exit:
  va_end(ap);
}
