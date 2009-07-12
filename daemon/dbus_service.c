/*
 *   LASH
 *
 *   Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <string.h>
#include <dbus/dbus.h>

#include "dbus_service.h"
#include "server.h"
#include "dbus_iface_server.h"
#include "dbus_iface_control.h"
#include "../common/debug.h"
#include "../dbus/object_path.h"
#include "client.h"

static DBusHandlerResult
lashd_client_disconnect_handler(DBusConnection *conn,
                                DBusMessage    *msg,
                                void           *data);

/* The appropriate destructor is service_destroy() in dbus/service.c */
service_t *
lashd_dbus_service_new(void)
{
  service_t *service;
  DBusError err;

                     // TODO: move service name into public header
  service = service_new(DBUS_NAME_BASE, &g_server->quit,
                        1,
                        object_path_new("/", NULL, 2,
                                        &g_lashd_interface_server,
                                        &g_lashd_interface_control,
                                        NULL),
                        NULL);
  if (!service) {
    lash_error("Failed to create D-Bus service");
    return NULL;
  }

  dbus_error_init(&err);

  dbus_bus_add_match(service->connection,
                     "type='signal'"
                     ",sender='org.freedesktop.DBus'"
                     ",path='/org/freedesktop/DBus'"
                     ",interface='org.freedesktop.DBus'"
                     ",member='NameOwnerChanged'"
                     ",arg2=''",
                     &err);
  if (dbus_error_is_set(&err)) {
    lash_error("Failed to add D-Bus match rule: %s", err.message);
    dbus_error_free(&err);
    goto fail;
  }

  if (!dbus_connection_add_filter(service->connection,
                                  lashd_client_disconnect_handler,
                                  NULL, NULL)) {
    lash_error("Failed to add D-Bus filter");
    goto fail;
  }

  return service;

fail:
  service_destroy(service);
  return NULL;
}

static DBusHandlerResult
lashd_client_disconnect_handler(DBusConnection *connection,
                                DBusMessage    *message,
                                void           *data)
{
  /* If the message isn't a signal the object path handler may use it */
  if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  const char *member, *name, *old_name;
  struct lash_client *client;
  DBusError err;

  if (!(member = dbus_message_get_member(message))) {
    lash_error("Received JACK signal with NULL member");
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (strcmp(member, "NameOwnerChanged") != 0)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

  dbus_error_init(&err);

  if (!dbus_message_get_args(message, &err,
                             DBUS_TYPE_STRING, &name,
                             DBUS_TYPE_STRING, &old_name,
                             DBUS_TYPE_INVALID)) {
    lash_error("Cannot get message arguments: %s",
               err.message);
    dbus_error_free(&err);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  client = server_find_client_by_dbus_name(old_name);
  if (client)
  {
    client_disconnected(client);
    return DBUS_HANDLER_RESULT_HANDLED;
  }

  else
  {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
}

/* EOF */
