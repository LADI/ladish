/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010,2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains  code that interfaces the jmcore through D-Bus
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

#include "jmcore_proxy.h"
#include "../dbus_constants.h"

int64_t g_jmcore_pid;

static void on_jmcore_life_status_changed(bool appeared)
{
  g_jmcore_pid = 0;
}

bool jmcore_proxy_init(void)
{
  if (!cdbus_register_service_lifetime_hook(cdbus_g_dbus_connection, JMCORE_SERVICE_NAME, on_jmcore_life_status_changed))
  {
    log_error("dbus_register_service_lifetime_hook() failed for a2j service");
    return false;
  }

  return jmcore_proxy_get_pid_noncached(&g_jmcore_pid);
}

void jmcore_proxy_uninit(void)
{
  cdbus_unregister_service_lifetime_hook(cdbus_g_dbus_connection, JMCORE_SERVICE_NAME);
}

int64_t jmcore_proxy_get_pid_cached(void)
{
  if (g_jmcore_pid == 0)
  {
    jmcore_proxy_get_pid_noncached(&g_jmcore_pid);
  }

  return g_jmcore_pid;
}

bool jmcore_proxy_get_pid_noncached(int64_t * pid_ptr)
{
  if (!cdbus_call(0, JMCORE_SERVICE_NAME, JMCORE_OBJECT_PATH, JMCORE_IFACE, "get_pid", "", "x", pid_ptr))
  {
    log_error("jmcore::get_pid() failed.");
    return false;
  }

  return true;
}

bool jmcore_proxy_create_link(bool midi, const char * input_port_name, const char * output_port_name)
{
  dbus_bool_t dbus_midi = midi;

  if (!cdbus_call(0, JMCORE_SERVICE_NAME, JMCORE_OBJECT_PATH, JMCORE_IFACE, "create", "bss", &dbus_midi, &input_port_name, &output_port_name, ""))
  {
    log_error("jmcore::create() failed: %s", cdbus_call_last_error_get_message());
    return false;
  }

  return true;
}

bool jmcore_proxy_destroy_link(const char * port_name)
{
  if (!cdbus_call(0, JMCORE_SERVICE_NAME, JMCORE_OBJECT_PATH, JMCORE_IFACE, "destroy", "s", &port_name, ""))
  {
    log_error("jmcore::destroy() failed.");
    return false;
  }

  return true;
}
