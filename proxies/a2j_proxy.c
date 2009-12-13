/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008,2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains code that interface with a2jmidid through D-Bus
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

#include "a2j_proxy.h"

#define A2J_SERVICE       "org.gna.home.a2jmidid"
#define A2J_OBJECT        "/"
#define A2J_IFACE_CONTROL "org.gna.home.a2jmidid.control"

static bool g_a2j_started = false;
static char * g_a2j_jack_client_name = NULL;

static void on_a2j_bridge_started(void * context, DBusMessage * message_ptr)
{
  log_info("a2j bridge start detected.");

  if (g_a2j_jack_client_name != NULL)
  {
    free(g_a2j_jack_client_name);
    g_a2j_jack_client_name = NULL;
  }

  g_a2j_started = true;
}

static void on_a2j_bridge_stopped(void * context, DBusMessage * message_ptr)
{
  if (g_a2j_jack_client_name != NULL)
  {
    free(g_a2j_jack_client_name);
    g_a2j_jack_client_name = NULL;
  }

  g_a2j_started = false;

  log_info("a2j bridge stop detected.");
}

static void on_a2j_life_status_changed(bool appeared)
{
  if (appeared)
  {
      log_info("a2j activatation detected.");
  }
  else
  {
      log_info("a2j deactivatation detected.");
  }
}

/* this must be static because it is referenced by the
 * dbus helper layer when hooks are active */
static struct dbus_signal_hook g_signal_hooks[] =
{
  {"bridge_started", on_a2j_bridge_started},
  {"bridge_stopped", on_a2j_bridge_stopped},
  {NULL, NULL}
};

bool a2j_proxy_init(void)
{
  g_a2j_started = a2j_proxy_is_started();
  if (g_a2j_started)
  {
    a2j_proxy_get_jack_client_name_noncached(&g_a2j_jack_client_name);
  }

  if (!dbus_register_service_lifetime_hook(g_dbus_connection, A2J_SERVICE, on_a2j_life_status_changed))
  {
    log_error("dbus_register_service_lifetime_hook() failed for a2j service");
    return false;
  }

  if (!dbus_register_object_signal_hooks(
        g_dbus_connection,
        A2J_SERVICE,
        A2J_OBJECT,
        A2J_IFACE_CONTROL,
        NULL,
        g_signal_hooks))
  {
    dbus_unregister_service_lifetime_hook(g_dbus_connection, A2J_SERVICE);
    log_error("dbus_register_object_signal_hooks() failed for a2j control interface");
    return false;
  }

  return true;
}

void a2j_proxy_uninit(void)
{
  dbus_unregister_object_signal_hooks(g_dbus_connection, A2J_SERVICE, A2J_OBJECT, A2J_IFACE_CONTROL);
  dbus_unregister_service_lifetime_hook(g_dbus_connection, A2J_SERVICE);
}

const char * a2j_proxy_get_jack_client_name_cached(void)
{
  if (g_a2j_jack_client_name == NULL)
  {
    a2j_proxy_get_jack_client_name_noncached(&g_a2j_jack_client_name);
  }

  return g_a2j_jack_client_name;
}

bool a2j_proxy_get_jack_client_name_noncached(char ** client_name_ptr_ptr)
{
  DBusMessage * reply_ptr;
  const char * name;

  if (!dbus_call(A2J_SERVICE, A2J_OBJECT, A2J_IFACE_CONTROL, "get_jack_client_name", "", NULL, &reply_ptr))
  {
    //log_error("a2j::get_jack_client_name() failed.");
    return false;
  }

  if (!dbus_message_get_args(reply_ptr, &g_dbus_error, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID))
  {
    dbus_message_unref(reply_ptr);
    dbus_error_free(&g_dbus_error);
    log_error("decoding reply of get_jack_client_name failed.");
    return false;
  }

  *client_name_ptr_ptr = strdup(name);

  dbus_message_unref(reply_ptr);

  if (*client_name_ptr_ptr == NULL)
  {
    log_error("strdup() failed for a2j jack client name string");
    return false;
  }

  return true;
}

bool
a2j_proxy_map_jack_port(
    const char * jack_port_name,
    char ** alsa_client_name_ptr_ptr,
    char ** alsa_port_name_ptr_ptr,
    uint32_t * alsa_client_id_ptr)
{
  DBusMessage * reply_ptr;
  dbus_uint32_t alsa_client_id;
  dbus_uint32_t alsa_port_id;
  const char * alsa_client_name;
  const char * alsa_port_name;

  if (!dbus_call(A2J_SERVICE, A2J_OBJECT, A2J_IFACE_CONTROL, "map_jack_port_to_alsa", "s", &jack_port_name, NULL, &reply_ptr))
  {
    log_error("a2j::map_jack_port_to_alsa() failed.");
    return false;
  }

  if (!dbus_message_get_args(
        reply_ptr,
        &g_dbus_error,
        DBUS_TYPE_UINT32,
        &alsa_client_id,
        DBUS_TYPE_UINT32,
        &alsa_port_id,
        DBUS_TYPE_STRING,
        &alsa_client_name,
        DBUS_TYPE_STRING,
        &alsa_port_name,
        DBUS_TYPE_INVALID))
  {
    dbus_message_unref(reply_ptr);
    dbus_error_free(&g_dbus_error);
    log_error("decoding reply of map_jack_port_to_alsa failed.");
    return false;
  }

  *alsa_client_name_ptr_ptr = strdup(alsa_client_name);
  if (*alsa_client_name_ptr_ptr == NULL)
  {
    dbus_message_unref(reply_ptr);
    log_error("strdup() failed for a2j alsa client name string");
    return false;
  }
  
  *alsa_port_name_ptr_ptr = strdup(alsa_port_name);
  if (*alsa_port_name_ptr_ptr == NULL)
  {
    dbus_message_unref(reply_ptr);
    log_error("strdup() failed for a2j alsa port name string");
    free(*alsa_client_name_ptr_ptr);
    return false;
  }

  *alsa_client_id_ptr = alsa_client_id;

  dbus_message_unref(reply_ptr);

  return true;
}

bool a2j_proxy_is_started(void)
{
  dbus_bool_t started;

  if (!dbus_call(A2J_SERVICE, A2J_OBJECT, A2J_IFACE_CONTROL, "is_started", "", "b", &started))
  {
    log_error("a2j::is_started() failed.");
    return false;
  }

  return started;
}

bool a2j_proxy_start_bridge(void)
{
  if (!dbus_call(A2J_SERVICE, A2J_OBJECT, A2J_IFACE_CONTROL, "start", "", ""))
  {
    log_error("a2j::start() failed.");
    return false;
  }

  return true;
}

bool a2j_proxy_stop_bridge(void)
{
  if (!dbus_call(A2J_SERVICE, A2J_OBJECT, A2J_IFACE_CONTROL, "stop", "", ""))
  {
    log_error("a2j::stop() failed.");
    return false;
  }

  return true;
}
