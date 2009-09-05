/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 **************************************************************************
 * This file contains D-Bus object path helpers
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
#include "object_path.h"
#include "../common/safety.h"
#include "introspection.h"  /* g_dbus_interface_dtor_introspectable */
#include "error.h"  /* lash_dbus_error() */

struct dbus_object_path * dbus_object_path_new(const char *name, const interface_t * iface1_ptr, ...)
{
  struct dbus_object_path * opath_ptr;
  va_list ap;
  const interface_t * iface_src_ptr;
  struct dbus_object_path_interface * iface_dst_ptr;
  void * iface_context;
  size_t len;

  lash_debug("Creating object path");

  opath_ptr = malloc(sizeof(struct dbus_object_path));
  if (opath_ptr == NULL)
  {
    lash_error("malloc() failed to allocate struct dbus_object_path.");
    goto fail;
  }
  
  opath_ptr->name = strdup(name);
  if (opath_ptr->name == NULL)
  {
    lash_error("malloc() failed to allocate struct dbus_object_path.");
    goto free;
  }

  va_start(ap, iface1_ptr);
  iface_src_ptr = iface1_ptr;
  len = 0;
  while (iface_src_ptr != NULL)
  {
    iface_context = va_arg(ap, void *);
    iface_src_ptr = va_arg(ap, const interface_t *);
    len++;
  }
  va_end(ap);

  opath_ptr->ifaces = malloc((len + 2) * sizeof(struct dbus_object_path_interface));
  if (opath_ptr->ifaces == NULL)
  {
    lash_error("malloc failed to allocate interfaces array");
    goto free_name;
  }

  va_start(ap, iface1_ptr);
  iface_src_ptr = iface1_ptr;
  iface_dst_ptr = opath_ptr->ifaces;
  while (iface_src_ptr != NULL)
  {
    iface_dst_ptr->iface = iface_src_ptr;
    iface_dst_ptr->iface_context = va_arg(ap, void *);
    iface_src_ptr = va_arg(ap, const interface_t *);
    iface_dst_ptr++;
    len--;
  }
  va_end(ap);

  assert(len == 0);

  iface_dst_ptr->iface = NULL;
  opath_ptr->introspection = introspection_new(opath_ptr);
  if (opath_ptr->introspection == NULL)
  {
    lash_error("introspection_new() failed.");
    goto free_ifaces;
  }

  iface_dst_ptr->iface = &g_dbus_interface_dtor_introspectable;
  iface_dst_ptr->iface_context = opath_ptr->introspection;
  iface_dst_ptr++;
  iface_dst_ptr->iface = NULL;

  return opath_ptr;

free_ifaces:
  free(opath_ptr->ifaces);
free_name:
  free(opath_ptr->name);
free:
  free(opath_ptr);
fail:
  return NULL;
}

void dbus_object_path_destroy(DBusConnection * connection_ptr, struct dbus_object_path * opath_ptr)
{
  lash_debug("Destroying object path");

  if (connection_ptr != NULL && !dbus_connection_unregister_object_path(connection_ptr, opath_ptr->name))
  {
    lash_error("dbus_connection_unregister_object_path() failed.");
  }

  introspection_destroy(opath_ptr);
  free(opath_ptr->ifaces);
  free(opath_ptr->name);
  free(opath_ptr);
}

#define opath_ptr ((struct dbus_object_path *)data)

static DBusHandlerResult dbus_object_path_handler(DBusConnection * connection, DBusMessage * message, void * data)
{
  const char * iface_name;
  const struct dbus_object_path_interface * iface_ptr;
  method_call_t call;

  /* Check if the message is a method call. If not, ignore it. */
  if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_METHOD_CALL)
  {
    goto handled;
  }

  /* Get the invoked method's name and make sure it's non-NULL. */
  call.method_name = dbus_message_get_member(message);
  if (call.method_name == NULL)
  {
    lash_dbus_error(&call, LASH_DBUS_ERROR_UNKNOWN_METHOD, "Received method call with empty method name");
    goto send_return;
  }

  /* Initialize our data. */
  call.connection = connection;
  call.message = message;
  call.interface = NULL; /* To be set by the default interface handler */
  call.reply = NULL;

  /* Check if there's an interface specified for this method call. */
  iface_name = dbus_message_get_interface(message);
  if (iface_name != NULL)
  {
    for (iface_ptr = opath_ptr->ifaces; iface_ptr->iface != NULL; iface_ptr++)
    {
      if (strcmp(iface_name, iface_ptr->iface->name) == 0)
      {
        call.context = iface_ptr->iface_context;
        if (!iface_ptr->iface->handler(iface_ptr->iface, &call))
        {
          /* unknown method */
          break;
        }

        goto send_return;
      }
    }
  }
  else
  {
    /* No interface was specified so we have to try them all. D-Bus spec states:
     *
     * Optionally, the message has an INTERFACE field giving the interface the method is a part of.
     * In the absence of an INTERFACE field, if two interfaces on the same object have a method with
     * the same name, it is undefined which of the two methods will be invoked.
     * Implementations may also choose to return an error in this ambiguous case.
     * However, if a method name is unique implementations must not require an interface field.
     */
    for (iface_ptr = opath_ptr->ifaces; iface_ptr->iface != NULL; iface_ptr++)
    {
      call.context = iface_ptr->iface_context;
      if (!iface_ptr->iface->handler(iface_ptr->iface, &call))
      {
        /* known method */
        goto send_return;
      }
    }
  }

  lash_dbus_error(&call, LASH_DBUS_ERROR_UNKNOWN_METHOD, "Method \"%s\" with signature \"%s\" on interface \"%s\" doesn't exist", call.method_name, dbus_message_get_signature(message), iface_name);

send_return:
  method_return_send(&call);

handled:
  return DBUS_HANDLER_RESULT_HANDLED;
}

static void dbus_object_path_handler_unregister(DBusConnection * connection_ptr, void * data)
{
#ifdef LASH_DEBUG
  lash_debug("Message handler of object path %s was unregistered", (opath_ptr && path->name) ? opath_ptr->name : "<unknown>");
#endif /* LASH_DEBUG */
}

#undef opath_ptr

bool dbus_object_path_register(DBusConnection * connection_ptr, struct dbus_object_path * opath_ptr)
{
  lash_debug("Registering object path");

  DBusObjectPathVTable vtable =
  {
    dbus_object_path_handler_unregister,
    dbus_object_path_handler,
    NULL, NULL, NULL, NULL
  };

  dbus_connection_register_object_path(connection_ptr, opath_ptr->name, &vtable, opath_ptr);

  return true;
}
