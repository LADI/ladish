/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains D-Bus related code
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

#include "internal.h"
#include "../dbus/helpers.h"
#include <dbus/dbus-glib-lowlevel.h>

void dbus_init(void)
{
  dbus_error_init(&g_dbus_error);

  // Connect to the bus
  g_dbus_connection = dbus_bus_get(DBUS_BUS_SESSION, &g_dbus_error);
  if (dbus_error_is_set(&g_dbus_error))
  {
    //error_msg("dbus_bus_get() failed");
    //error_msg(g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
  }

  dbus_connection_setup_with_g_main(g_dbus_connection, NULL);
}

void dbus_uninit(void)
{
  if (g_dbus_connection)
  {
    dbus_connection_flush(g_dbus_connection);
  }

  if (dbus_error_is_set(&g_dbus_error))
  {
    dbus_error_free(&g_dbus_error);
  }

  dbus_call_last_error_cleanup();
}
