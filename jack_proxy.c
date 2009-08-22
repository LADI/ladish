/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains helper functionality for accessing JACK through D-Bus
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

//#define LASH_DEBUG

#include <dbus/dbus.h>

#include "common.h"
#include "jack_proxy.h"
#include "dbus/helpers.h"
#include "common/debug.h"
#include "dbus_constants.h"

jack_proxy_callback_server_started g_on_server_started;
jack_proxy_callback_server_stopped g_on_server_stopped;
jack_proxy_callback_server_appeared g_on_server_appeared;
jack_proxy_callback_server_disappeared g_on_server_disappeared;

static
void
on_jack_control_signal(
  DBusMessage * message_ptr,
  const char * signal_name)
{
  if (strcmp(signal_name, "ServerStarted") == 0)
  {
    lash_debug("JACK server start detected.");
    if (g_on_server_started != NULL)
    {
      g_on_server_started();
    }

    return;
  }

  if (strcmp(signal_name, "ServerStopped") == 0)
  {
    lash_debug("JACK server stop detected.");
    if (g_on_server_stopped != NULL)
    {
      g_on_server_stopped();
    }

    return;
  }
}

static
DBusHandlerResult
on_bus_signal(
  DBusMessage * message_ptr,
  const char * signal_name)
{
  const char * object_name;
  const char * old_owner;
  const char * new_owner;

  //lash_info("bus signal '%s' received", signal_name);

  dbus_error_init(&g_dbus_error);

  if (strcmp(signal_name, "NameOwnerChanged") == 0)
  {
    //lash_info("NameOwnerChanged signal received");

    if (!dbus_message_get_args(
          message_ptr,
          &g_dbus_error,
          DBUS_TYPE_STRING, &object_name,
          DBUS_TYPE_STRING, &old_owner,
          DBUS_TYPE_STRING, &new_owner,
          DBUS_TYPE_INVALID)) {
      lash_error("Cannot get message arguments: %s", g_dbus_error.message);
      dbus_error_free(&g_dbus_error);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (strcmp(object_name, JACKDBUS_SERVICE_NAME) != 0)
    {
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    if (old_owner[0] == '\0')
    {
      lash_debug("JACK serivce appeared");
      if (g_on_server_appeared != NULL)
      {
        g_on_server_appeared();
      }
    }
    else if (new_owner[0] == '\0')
    {
      lash_debug("JACK serivce disappeared");
      if (g_on_server_disappeared != NULL)
      {
        g_on_server_disappeared();
      }
    }

    return DBUS_HANDLER_RESULT_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static
DBusHandlerResult
dbus_signal_handler(
  DBusConnection * connection_ptr,
  DBusMessage * message_ptr,
  void * data)
{
  const char * object_path;
  const char * interface;
  const char * signal_name;

  /* Non-signal messages are ignored */
  if (dbus_message_get_type(message_ptr) != DBUS_MESSAGE_TYPE_SIGNAL)
  {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  interface = dbus_message_get_interface(message_ptr);
  if (interface == NULL)
  {
    /* Signals with no interface are ignored */
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  object_path = dbus_message_get_path(message_ptr);

  signal_name = dbus_message_get_member(message_ptr);
  if (signal_name == NULL)
  {
    lash_error("Received signal with NULL member");
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  lash_debug("'%s' sent signal '%s'::'%s'", object_path, interface, signal_name);

  /* Handle JACK patchbay and control interface signals */
  if (object_path != NULL && strcmp(object_path, JACKDBUS_OBJECT_PATH) == 0)
  {
    if (strcmp(interface, JACKDBUS_IFACE_CONTROL) == 0)
    {
      on_jack_control_signal(message_ptr, signal_name);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  /* Handle session bus signals to track JACK service alive state */
  if (strcmp(interface, DBUS_INTERFACE_DBUS) == 0)
  {
    return on_bus_signal(message_ptr, signal_name);
  }

  /* Let everything else pass through */
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

bool
jack_proxy_init(
  jack_proxy_callback_server_started server_started,
  jack_proxy_callback_server_stopped server_stopped,
  jack_proxy_callback_server_appeared server_appeared,
  jack_proxy_callback_server_disappeared server_disappeared)
{
  DBusError err;
  char rule[1024];
  const char ** signal;

  const char * control_signals[] = {
    "ServerStarted",
    "ServerStopped",
    NULL};

  dbus_bus_add_match(
    g_dbus_connection,
    "type='signal',interface='"DBUS_INTERFACE_DBUS"',member=NameOwnerChanged,arg0='"JACKDBUS_SERVICE_NAME"'",
    &err);
  if (dbus_error_is_set(&err))
  {
    lash_error("Failed to add D-Bus match rule: %s", err.message);
    dbus_error_free(&err);
    return false;
  }

  for (signal = control_signals; *signal != NULL; signal++)
  {
    snprintf(
      rule,
      sizeof(rule),
      "type='signal',sender='"JACKDBUS_SERVICE_NAME"',path='"JACKDBUS_OBJECT_PATH"',interface='"JACKDBUS_IFACE_CONTROL"',member='%s'",
      *signal);

    dbus_bus_add_match(g_dbus_connection, rule, &err);
    if (dbus_error_is_set(&err))
    {
      lash_error("Failed to add D-Bus match rule: %s", err.message);
      dbus_error_free(&err);
      return false;
    }
  }

  if (!dbus_connection_add_filter(g_dbus_connection, dbus_signal_handler, NULL, NULL))
  {
    lash_error("Failed to add D-Bus filter");
    return false;
  }

  g_on_server_started = server_started;
  g_on_server_stopped = server_stopped;
  g_on_server_appeared = server_appeared;
  g_on_server_disappeared = server_disappeared;

  {
    bool started;

    if (jack_proxy_is_started(&started))
    {
      if (g_on_server_appeared != NULL)
      {
        g_on_server_appeared();
      }

      if (g_on_server_started != NULL && started)
      {
        g_on_server_started();
      }
    }
  }

  return true;
}

void
jack_proxy_uninit(
  void)
{
  dbus_connection_remove_filter(g_dbus_connection, dbus_signal_handler, NULL);
}

bool
jack_proxy_is_started(
  bool * started_ptr)
{
  dbus_bool_t started;

  if (!dbus_call_simple(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONTROL, "IsStarted", "", "b", &started))
  {
    return false;
  }

  *started_ptr = started;
  return true;
}

bool
jack_proxy_get_client_pid(
  uint64_t client_id,
  pid_t * pid_ptr)
{
  return false;
}

bool
jack_proxy_connect_ports(
  uint64_t port1_id,
  uint64_t port2_id)
{
  return false;
}

static
bool
add_address(
  DBusMessageIter * iter_ptr,
  const char * address)
{
  DBusMessageIter array_iter;
  const char * component;

  if (!dbus_message_iter_open_container(iter_ptr, DBUS_TYPE_ARRAY, "s", &array_iter))
  {
    lash_error("dbus_message_iter_open_container() failed.");
    return false;
  }

  if (address != NULL)
  {
    component = address;
    while (*component != 0)
    {
      if (!dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, &component))
      {
        lash_error("dbus_message_iter_append_basic() failed.");
        return false;
      }

      component += strlen(component) + 1;
    }
  }

  dbus_message_iter_close_container(iter_ptr, &array_iter);

  return true;
}

bool
jack_proxy_read_conf_container(
  const char * address,
  void * callback_context,
  bool (* callback)(void * context, bool leaf, const char * address, char * child))
{
  DBusMessage * request_ptr;
  DBusMessage * reply_ptr;
  DBusMessageIter top_iter;
  DBusMessageIter array_iter;
  const char * reply_signature;
  dbus_bool_t leaf;           /* Whether children are parameters (true) or containers (false) */
  char * child;

  request_ptr = dbus_message_new_method_call(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONFIGURE, "ReadContainer");
  if (request_ptr == NULL)
  {
    lash_error("dbus_message_new_method_call() failed.");
    return false;
  }

  dbus_message_iter_init_append(request_ptr, &top_iter);

  if (!add_address(&top_iter, address))
  {
    dbus_message_unref(request_ptr);
    return false;
  }

  // send message and get a handle for a reply
  reply_ptr = dbus_connection_send_with_reply_and_block(
    g_dbus_connection,
    request_ptr,
    DBUS_CALL_DEFAULT_TIMEOUT,
    &g_dbus_error);

  dbus_message_unref(request_ptr);

  if (reply_ptr == NULL)
  {
    lash_error("no reply from JACK server, error is '%s'", g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return false;
  }

  reply_signature = dbus_message_get_signature(reply_ptr);

  if (strcmp(reply_signature, "bas") != 0)
  {
    lash_error("ReadContainer() reply signature mismatch. '%s'", reply_signature);
    dbus_message_unref(reply_ptr);
    return false;
  }

  dbus_message_iter_init(reply_ptr, &top_iter);

  dbus_message_iter_get_basic(&top_iter, &leaf);
  dbus_message_iter_next(&top_iter);

  dbus_message_iter_recurse(&top_iter, &array_iter);

  while (dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_INVALID)
  {
    dbus_message_iter_get_basic(&array_iter, &child);

    if (!callback(callback_context, leaf, address, child))
    {
      break;
    }

    dbus_message_iter_next(&array_iter);
  }

  dbus_message_unref(reply_ptr);

  return true;
}

bool
get_variant(
  DBusMessageIter * iter_ptr,
  struct jack_parameter_variant * parameter_ptr)
{
  DBusMessageIter variant_iter;
  int type;
  dbus_bool_t boolean;
  dbus_int32_t int32;
  dbus_uint32_t uint32;
  char * string;

  dbus_message_iter_recurse(iter_ptr, &variant_iter);
  lash_debug("variant signature: '%s'", dbus_message_iter_get_signature(&variant_iter));

  type = dbus_message_iter_get_arg_type(&variant_iter);
  switch (type)
  {
  case DBUS_TYPE_INT32:
    dbus_message_iter_get_basic(&variant_iter, &int32);
    parameter_ptr->value.int32 = int32;
    parameter_ptr->type = jack_int32;
    return true;
  case DBUS_TYPE_UINT32:
    dbus_message_iter_get_basic(&variant_iter, &uint32);
    parameter_ptr->value.uint32 = uint32;
    parameter_ptr->type = jack_uint32;
    return true;
  case DBUS_TYPE_BYTE:
    dbus_message_iter_get_basic(&variant_iter, &parameter_ptr->value.byte);
    parameter_ptr->type = jack_byte;
    return true;
  case DBUS_TYPE_STRING:
    dbus_message_iter_get_basic(&variant_iter, &string);
    string = strdup(string);
    if (string == NULL)
    {
      lash_error("strdup failed.");
      return false;
    }

    parameter_ptr->value.string = string;
    parameter_ptr->type = jack_string;
    return true;
  case DBUS_TYPE_BOOLEAN:
    dbus_message_iter_get_basic(&variant_iter, &boolean);
    parameter_ptr->value.boolean = boolean;
    parameter_ptr->type = jack_boolean;
    return true;
  }

  lash_error("Unknown D-Bus parameter type %i", (int)type);
  return false;
}

bool
jack_proxy_get_parameter_value(
  const char * address,
  bool * is_set_ptr,
  struct jack_parameter_variant * parameter_ptr)
{
  DBusMessage * request_ptr;
  DBusMessage * reply_ptr;
  DBusMessageIter top_iter;
  const char * reply_signature;
  dbus_bool_t is_set;
  struct jack_parameter_variant default_value;

  request_ptr = dbus_message_new_method_call(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONFIGURE, "GetParameterValue");
  if (request_ptr == NULL)
  {
    lash_error("dbus_message_new_method_call() failed.");
    return false;
  }

  dbus_message_iter_init_append(request_ptr, &top_iter);

  if (!add_address(&top_iter, address))
  {
    dbus_message_unref(request_ptr);
    return false;
  }

  // send message and get a handle for a reply
  reply_ptr = dbus_connection_send_with_reply_and_block(
    g_dbus_connection,
    request_ptr,
    DBUS_CALL_DEFAULT_TIMEOUT,
    &g_dbus_error);

  dbus_message_unref(request_ptr);

  if (reply_ptr == NULL)
  {
    lash_error("no reply from JACK server, error is '%s'", g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return false;
  }

  reply_signature = dbus_message_get_signature(reply_ptr);

  if (strcmp(reply_signature, "bvv") != 0)
  {
    lash_error("GetParameterValue() reply signature mismatch. '%s'", reply_signature);
    dbus_message_unref(reply_ptr);
    return false;
  }

  dbus_message_iter_init(reply_ptr, &top_iter);

  dbus_message_iter_get_basic(&top_iter, &is_set);
  dbus_message_iter_next(&top_iter);

  if (!get_variant(&top_iter, &default_value))
  {
    dbus_message_unref(reply_ptr);
    return false;
  }

  if (default_value.type == jack_string)
  {
    free(default_value.value.string);
  }

  dbus_message_iter_next(&top_iter);

  if (!get_variant(&top_iter, parameter_ptr))
  {
    dbus_message_unref(reply_ptr);
    return false;
  }

  dbus_message_unref(reply_ptr);

  *is_set_ptr = is_set;

  return true;
}

bool jack_proxy_start_server(void)
{
  return dbus_call_simple(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONTROL, "StartServer", "", "");
}

bool jack_proxy_stop_server(void)
{
  return dbus_call_simple(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONTROL, "StopServer", "", "");
}

bool jack_proxy_is_realtime(bool * realtime_ptr)
{
  dbus_bool_t realtime;

  if (!dbus_call_simple(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONTROL, "IsStarted", "", "b", &realtime))
  {
    return false;
  }

  *realtime_ptr = realtime;
  return true;
}

bool jack_proxy_sample_rate(uint32_t * sample_rate_ptr)
{
  return dbus_call_simple(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONTROL, "GetSampleRate", "", "u", sample_rate_ptr);
}

bool jack_proxy_get_xruns(uint32_t * xruns_ptr)
{
  return dbus_call_simple(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONTROL, "GetXruns", "", "u", xruns_ptr);
}

bool jack_proxy_get_dsp_load(double * dsp_load_ptr)
{
  return dbus_call_simple(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONTROL, "GetLoad", "", "d", dsp_load_ptr);
}

bool jack_proxy_get_buffer_size(uint32_t * size_ptr)
{
  return dbus_call_simple(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONTROL, "GetBufferSize", "", "u", size_ptr);
}

bool jack_proxy_set_buffer_size(uint32_t size)
{
  return dbus_call_simple(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONTROL, "SetBufferSize", "u", &size, "");
}

bool jack_proxy_reset_xruns(void)
{
  return dbus_call_simple(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONTROL, "ResetXruns", "", "");
}
