/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 **************************************************************************
 * This file contains D-Bus interface dispatcher
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

#include <string.h>

#include "../common/debug.h"

#include "interface.h"
#include "error.h"
#include "object_path.h"

/*
 * Execute a method's function if the method specified in the method call
 * object exists in the method array. Return true if the method was found,
 * false otherwise.
 * TODO: rewrite description ^
 */
bool
interface_default_handler(const interface_t *interface,
                          method_call_t     *call)
{
  const method_t *ptr;

  for (ptr = (const method_t *) interface->methods;
       ptr && ptr->name;
       ++ptr) {
    if (strcmp(call->method_name, ptr->name) == 0) {
      if (ptr->handler) {
        call->interface = interface;
        call->context = ((object_path_t *)call->context)->context;
        ptr->handler(call);

        /* If the method handler didn't construct a return
           message create a void one here */
        // TODO: Also handle cases where the sender doesn't need a reply
        if (!call->reply
            && !(call->reply = dbus_message_new_method_return(call->message))) {
          lash_error("Failed to construct void method return");
        }
      } else {
        lash_dbus_error(call, LASH_DBUS_ERROR_GENERIC,
                        "Handler for method \"%s\" is NULL", ptr->name);
      }

      /* Found method */
      return true;
    }
  }

  /* Didn't find method */
  return false;
}

/* EOF */
