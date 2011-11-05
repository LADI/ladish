/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008,2009,2011 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 **************************************************************************
 * This file contains D-Bus interface dispatcher
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
#include <string.h>

/*
 * Execute a method's function if the method specified in the method call
 * object exists in the method array. Return true if the method was found,
 * false otherwise.
 */
bool cdbus_interface_default_handler(const struct cdbus_interface_descriptor * iface_ptr, struct cdbus_method_call * call_ptr)
{
  const struct cdbus_method_descriptor * method_ptr;

  for (method_ptr = iface_ptr->methods; method_ptr->name != NULL; method_ptr++)
  {
    if (strcmp(call_ptr->method_name, method_ptr->name) == 0)
    {
      call_ptr->iface = iface_ptr;
      method_ptr->handler(call_ptr);
      /* If the method handler didn't construct a return message create a void one here */
      // TODO: Also handle cases where the sender doesn't need a reply
      if (call_ptr->reply == NULL)
      {
        call_ptr->reply = dbus_message_new_method_return(call_ptr->message);
        if (call_ptr->reply == NULL)
        {
          log_error("Failed to construct void method return");
        }
      }

      /* Known method */
      return true;
    }
  }

  /* Unknown method */
  return false;
}
