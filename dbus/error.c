/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 **************************************************************************
 * This file contains dbus error helpers
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
#include "helpers.h"
#include "error.h"
#include "../common/debug.h"

void lash_dbus_error(struct dbus_method_call * call_ptr, const char * err_name, const char * format, ...)
{
  va_list ap;
  char message[1024];
  const char *interface_name;

  va_start(ap, format);

  vsnprintf(message, sizeof(message), format, ap);
  message[sizeof(message) - 1] = '\0';

  va_end(ap);

  if (call_ptr != NULL)
  {
    interface_name = (call_ptr->iface && call_ptr->iface->name && call_ptr->iface->name[0]) ? call_ptr->iface->name : "<unknown>";

    lash_error("In method %s.%s: %s", interface_name, call_ptr->method_name, message);

    call_ptr->reply = dbus_message_new_error(call_ptr->message, err_name, message);
  }
  else
  {
    lash_error("%s", message);
  }
}
