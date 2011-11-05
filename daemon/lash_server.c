/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of lash_server singleton object
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

#include "lash_server.h"
#include "../dbus_constants.h"
#include "virtualizer.h"

static cdbus_object_path ladishd_g_lash_server_dbus_object;
extern const struct cdbus_interface_descriptor g_iface_lash_server;

bool lash_server_init(void)
{
  ladishd_g_lash_server_dbus_object = cdbus_object_path_new(
    LASH_SERVER_OBJECT_PATH,
    &g_iface_lash_server, NULL,
    NULL);
  if (ladishd_g_lash_server_dbus_object == NULL)
  {
    return false;
  }

  if (!cdbus_object_path_register(cdbus_g_dbus_connection, ladishd_g_lash_server_dbus_object))
  {
    cdbus_object_path_destroy(cdbus_g_dbus_connection, ladishd_g_lash_server_dbus_object);
    return false;
  }

  return true;
}

void lash_server_uninit(void)
{
  cdbus_object_path_destroy(cdbus_g_dbus_connection, ladishd_g_lash_server_dbus_object);
}

/**********************************************************************************/
/*                                D-Bus methods                                   */
/**********************************************************************************/

static void lash_server_register_client(struct cdbus_method_call * call_ptr)
{
  const char * sender;
  const char * class;
  dbus_uint64_t pid;
  dbus_uint32_t flags;
  ladish_app_handle app;

  if (!dbus_message_get_args(
        call_ptr->message,
        &cdbus_g_dbus_error,
        DBUS_TYPE_UINT64, &pid,
        DBUS_TYPE_STRING, &class,
        DBUS_TYPE_UINT32, &flags,
        DBUS_TYPE_INVALID))
  {
    cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  sender = dbus_message_get_sender(call_ptr->message);

  log_info("LASH client registered. pid=%"PRIu64" dbusname='%s' class='%s' flags=0x%"PRIu32")", pid, sender, class, flags);

  app = ladish_find_app_by_pid((pid_t)pid, NULL);
  if (app == NULL)
  {
    log_error("Unknown LASH app registered");
    return;
  }

  ladish_app_set_dbus_name(app, sender);
  ladish_app_restore(app);
}

METHOD_ARGS_BEGIN(RegisterClient, "Register LASH client")
  METHOD_ARG_DESCRIBE_IN("class", DBUS_TYPE_STRING_AS_STRING, "LASH app class")
  METHOD_ARG_DESCRIBE_IN("flags", DBUS_TYPE_UINT32_AS_STRING, "LASH app flags")
METHOD_ARGS_END

METHODS_BEGIN
  METHOD_DESCRIBE(RegisterClient, lash_server_register_client)
METHODS_END

INTERFACE_BEGIN(g_iface_lash_server, IFACE_LASH_SERVER)
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
INTERFACE_END
