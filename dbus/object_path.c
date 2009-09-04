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

static DBusHandlerResult
object_path_handler(DBusConnection *connection,
                    DBusMessage    *message,
                    void           *data);

static void
object_path_handler_unregister(DBusConnection *conn,
                               void           *data);

struct dbus_object_path *
dbus_object_path_new(const char *name,
                void       *context,
                int         num_ifaces,
                            ...)
{
  if (!name || !name[0] || num_ifaces < 0)
  {
    lash_error("Invalid arguments");
    return NULL;
  }

  lash_debug("Creating object path");

  struct dbus_object_path *path;
  va_list argp;
  const interface_t **iface_pptr;

  path = lash_malloc(1, sizeof(struct dbus_object_path));
  path->name = lash_strdup(name);
  path->interfaces = lash_malloc(num_ifaces + 2, sizeof(interface_t *));

  va_start(argp, num_ifaces);

  iface_pptr = path->interfaces;
  *iface_pptr++ = &g_dbus_interface_dtor_introspectable;
  while (num_ifaces > 0)
  {
    *iface_pptr++ = va_arg(argp, const interface_t *);
    num_ifaces--;
  }
  *iface_pptr = NULL;

  va_end(argp);

  if ((path->introspection = introspection_new(path))) {
    path->context = context;
    return path;
  }

  lash_error("Failed to create object path");
  dbus_object_path_destroy(NULL, path);

  return NULL;
}

bool
dbus_object_path_register(DBusConnection *conn,
                     struct dbus_object_path  *path)
{
  if (!conn || !path || !path->name || !path->interfaces) {
    lash_debug("Invalid arguments");
    return false;
  }

  lash_debug("Registering object path");

  DBusObjectPathVTable vtable =
  {
    object_path_handler_unregister,
    object_path_handler,
    NULL, NULL, NULL, NULL
  };

  dbus_connection_register_object_path(conn, path->name,
                                       &vtable, (void *) path);

  return true;
}

void dbus_object_path_destroy(DBusConnection * connection_ptr, struct dbus_object_path * path_ptr)
{
  lash_debug("Destroying object path");

  if (path_ptr)
  {
    if (connection_ptr != NULL && !dbus_connection_unregister_object_path(connection_ptr, path_ptr->name))
    {
      lash_error("dbus_connection_unregister_object_path() failed.");
    }

    if (path_ptr->name)
    {
      free(path_ptr->name);
      path_ptr->name = NULL;
    }

    if (path_ptr->interfaces)
    {
      free(path_ptr->interfaces);
      path_ptr->interfaces = NULL;
    }

    introspection_destroy(path_ptr);
    free(path_ptr);
  }
#ifdef LASH_DEBUG
  else
  {
    lash_debug("Nothing to destroy");
  }
#endif
}


static DBusHandlerResult
object_path_handler(DBusConnection *connection,
                    DBusMessage    *message,
                    void           *data)
{
  const char *interface_name;
  const interface_t **iface_pptr;
  method_call_t call;

  /* Check if the message is a method call. If not, ignore it. */
  if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_METHOD_CALL) {
    goto handled;
  }

  /* Get the invoked method's name and make sure it's non-NULL. */
  if (!(call.method_name = dbus_message_get_member(message))) {
    lash_dbus_error(&call, LASH_DBUS_ERROR_UNKNOWN_METHOD,
                    "Received method call with empty method name");
    goto send_return;
  }

  /* Initialize our data. */
  call.connection = connection;
  call.message = message;
  call.interface = NULL; /* To be set by the default interface handler */
  call.context = data;
  call.reply = NULL;

  /* Check if there's an interface specified for this method call. */
  if ((interface_name = dbus_message_get_interface(message)))
  {
    for (iface_pptr = (const interface_t **) ((struct dbus_object_path *) data)->interfaces;
         iface_pptr && *iface_pptr;
         ++iface_pptr)
    {
      if (strcmp(interface_name, (*iface_pptr)->name) == 0)
      {
        if ((*iface_pptr)->handler(*iface_pptr, &call))
        {
          goto send_return;
        }

        break;
      }
    }
  }
  else
  {
    /* No interface was specified so we have to try them all. This is
    * dictated by the D-Bus specification which states that method calls
    * omitting the interface must never be rejected.
    */

    for (iface_pptr = (const interface_t **) ((struct dbus_object_path *) data)->interfaces;
         iface_pptr && *iface_pptr;
         ++iface_pptr) {
      if ((*iface_pptr)->handler(*iface_pptr, &call)) {
        goto send_return;
      }
    }
  }

  lash_dbus_error(&call, LASH_DBUS_ERROR_UNKNOWN_METHOD,
                  "Method \"%s\" with signature \"%s\" on interface \"%s\" doesn't exist",
                  call.method_name, dbus_message_get_signature(message), interface_name);

send_return:
  method_return_send(&call);

handled:
  return DBUS_HANDLER_RESULT_HANDLED;
}

static void
object_path_handler_unregister(DBusConnection *conn,
                               void           *data)
{
#ifdef LASH_DEBUG
  struct dbus_object_path *path = data;
  lash_debug("Message handler of object path %s was unregistered",
             (path && path->name) ? path->name : "<unknown>");
#endif /* LASH_DEBUG */
}

/* EOF */
