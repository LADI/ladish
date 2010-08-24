/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
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

//#define LADISH_DEBUG

#include "jack_proxy.h"

jack_proxy_callback_server_started g_on_server_started;
jack_proxy_callback_server_stopped g_on_server_stopped;
jack_proxy_callback_server_appeared g_on_server_appeared;
jack_proxy_callback_server_disappeared g_on_server_disappeared;

static void on_jack_server_started(void * context, DBusMessage * message_ptr)
{
  log_debug("JACK server start signal received.");
  if (g_on_server_started != NULL)
  {
    g_on_server_started();
  }
}

static void on_jack_server_stopped(void * context, DBusMessage * message_ptr)
{
  log_debug("JACK server stop signal received.");
  if (g_on_server_stopped != NULL)
  {
    g_on_server_stopped();
  }
}

static void on_jack_life_status_changed(bool appeared)
{
  if (appeared)
  {
    log_debug("JACK serivce appeared");
    if (g_on_server_appeared != NULL)
    {
      g_on_server_appeared();
    }
  }
  else
  {
    log_debug("JACK serivce disappeared");
    if (g_on_server_disappeared != NULL)
    {
      g_on_server_disappeared();
    }
  }
}

/* this must be static because it is referenced by the
 * dbus helper layer when hooks are active */
static struct dbus_signal_hook g_control_signal_hooks[] =
{
  {"ServerStarted", on_jack_server_started},
  {"ServerStopped", on_jack_server_stopped},
  {NULL, NULL}
};

bool
jack_proxy_init(
  jack_proxy_callback_server_started server_started,
  jack_proxy_callback_server_stopped server_stopped,
  jack_proxy_callback_server_appeared server_appeared,
  jack_proxy_callback_server_disappeared server_disappeared)
{
  g_on_server_started = server_started;
  g_on_server_stopped = server_stopped;
  g_on_server_appeared = server_appeared;
  g_on_server_disappeared = server_disappeared;

  if (!dbus_register_service_lifetime_hook(g_dbus_connection, JACKDBUS_SERVICE_NAME, on_jack_life_status_changed))
  {
    log_error("dbus_register_service_lifetime_hook() failed for jackdbus service");
    return false;
  }

  if (!dbus_register_object_signal_hooks(
        g_dbus_connection,
        JACKDBUS_SERVICE_NAME,
        JACKDBUS_OBJECT_PATH,
        JACKDBUS_IFACE_CONTROL,
        NULL,
        g_control_signal_hooks))
  {
    dbus_unregister_service_lifetime_hook(g_dbus_connection, JACKDBUS_SERVICE_NAME);
    log_error("dbus_register_object_signal_hooks() failed for jackdbus control interface");
    return false;
  }

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
  dbus_unregister_object_signal_hooks(g_dbus_connection, JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONTROL);
  dbus_unregister_service_lifetime_hook(g_dbus_connection, JACKDBUS_SERVICE_NAME);
}

bool
jack_proxy_is_started(
  bool * started_ptr)
{
  dbus_bool_t started;

  if (!dbus_call(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONTROL, "IsStarted", "", "b", &started))
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
  int64_t pid;

  if (!dbus_call(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_PATCHBAY, "GetClientPID", "t", &client_id, "x", &pid))
  {
    return false;
  }

  *pid_ptr = pid;
  return true;
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
    log_error("dbus_message_iter_open_container() failed.");
    return false;
  }

  if (address != NULL)
  {
    component = address;
    while (*component != 0)
    {
      if (!dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, &component))
      {
        log_error("dbus_message_iter_append_basic() failed.");
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
    log_error("dbus_message_new_method_call() failed.");
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
    log_error("no reply from JACK server, error is '%s'", g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return false;
  }

  reply_signature = dbus_message_get_signature(reply_ptr);

  if (strcmp(reply_signature, "bas") != 0)
  {
    log_error("ReadContainer() reply signature mismatch. '%s'", reply_signature);
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
  log_debug("variant signature: '%s'", dbus_message_iter_get_signature(&variant_iter));

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
      log_error("strdup failed.");
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

  log_error("Unknown D-Bus parameter type %i", (int)type);
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
    log_error("dbus_message_new_method_call() failed.");
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
    log_error("no reply from JACK server, error is '%s'", g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return false;
  }

  reply_signature = dbus_message_get_signature(reply_ptr);

  if (strcmp(reply_signature, "bvv") != 0)
  {
    log_error("GetParameterValue() reply signature mismatch. '%s'", reply_signature);
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

bool
jack_proxy_set_parameter_value(
  const char * address,
  const struct jack_parameter_variant * parameter_ptr)
{
  DBusMessage * request_ptr;
  DBusMessage * reply_ptr;
  DBusMessageIter top_iter;
  const char * reply_signature;
  int type;
  const void * value_ptr;
  dbus_bool_t boolean;

  switch (parameter_ptr->type)
  {
  case jack_int32:
    type = DBUS_TYPE_INT32;
    value_ptr = &parameter_ptr->value.int32;
    break;
  case jack_uint32:
    type = DBUS_TYPE_UINT32;
    value_ptr = &parameter_ptr->value.uint32;
    break;
  case jack_byte:
    type = DBUS_TYPE_BYTE;
    value_ptr = &parameter_ptr->value.byte;
    break;
  case jack_string:
    type = DBUS_TYPE_STRING;
    value_ptr = &parameter_ptr->value.string;
    break;
  case jack_boolean:
    type = DBUS_TYPE_BOOLEAN;
    boolean = parameter_ptr->value.boolean;
    value_ptr = &boolean;
    break;
  default:
    log_error("Unknown jack parameter type %i", (int)parameter_ptr->type);
    return false;
  }

  request_ptr = dbus_message_new_method_call(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONFIGURE, "SetParameterValue");
  if (request_ptr == NULL)
  {
    log_error("dbus_message_new_method_call() failed.");
    return false;
  }

  dbus_message_iter_init_append(request_ptr, &top_iter);

  if (!add_address(&top_iter, address))
  {
    dbus_message_unref(request_ptr);
    return false;
  }

  if (!dbus_iter_append_variant(&top_iter, type, value_ptr))
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
    log_error("no reply from JACK server, error is '%s'", g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return false;
  }

  reply_signature = dbus_message_get_signature(reply_ptr);

  dbus_message_unref(reply_ptr);

  if (strcmp(reply_signature, "") != 0)
  {
    log_error("SetParameterValue() reply signature mismatch. '%s'", reply_signature);
    return false;
  }

  return true;
}

bool
jack_proxy_reset_parameter_value(
  const char * address)
{
  DBusMessage * request_ptr;
  DBusMessage * reply_ptr;
  DBusMessageIter top_iter;
  const char * reply_signature;

  request_ptr = dbus_message_new_method_call(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONFIGURE, "ResetParameterValue");
  if (request_ptr == NULL)
  {
    log_error("dbus_message_new_method_call() failed.");
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
    log_error("no reply from JACK server, error is '%s'", g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return false;
  }

  reply_signature = dbus_message_get_signature(reply_ptr);

  dbus_message_unref(reply_ptr);

  if (strcmp(reply_signature, "") != 0)
  {
    log_error("ResetParameterValue() reply signature mismatch. '%s'", reply_signature);
    return false;
  }

  return true;
}

bool jack_proxy_start_server(void)
{
  return dbus_call(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONTROL, "StartServer", "", "");
}

bool jack_proxy_stop_server(void)
{
  return dbus_call(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONTROL, "StopServer", "", "");
}

bool jack_proxy_is_realtime(bool * realtime_ptr)
{
  dbus_bool_t realtime;

  if (!dbus_call(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONTROL, "IsStarted", "", "b", &realtime))
  {
    return false;
  }

  *realtime_ptr = realtime;
  return true;
}

bool jack_proxy_sample_rate(uint32_t * sample_rate_ptr)
{
  return dbus_call(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONTROL, "GetSampleRate", "", "u", sample_rate_ptr);
}

bool jack_proxy_get_xruns(uint32_t * xruns_ptr)
{
  return dbus_call(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONTROL, "GetXruns", "", "u", xruns_ptr);
}

bool jack_proxy_get_dsp_load(double * dsp_load_ptr)
{
  return dbus_call(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONTROL, "GetLoad", "", "d", dsp_load_ptr);
}

bool jack_proxy_get_buffer_size(uint32_t * size_ptr)
{
  return dbus_call(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONTROL, "GetBufferSize", "", "u", size_ptr);
}

bool jack_proxy_set_buffer_size(uint32_t size)
{
  return dbus_call(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONTROL, "SetBufferSize", "u", &size, "");
}

bool jack_proxy_reset_xruns(void)
{
  return dbus_call(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, JACKDBUS_IFACE_CONTROL, "ResetXruns", "", "");
}

static
bool
reset_callback(
  void * context,
  bool leaf,
  const char * address,
  char * child)
{
  const char * component;
  char * dst;
  size_t len;

  component = address;
  while (*component != 0)
  {
    component += strlen(component) + 1;
  }

  /* address always is same buffer as the one in the jack_reset_all_params() stack */
  dst = (char *)component;

  len = strlen(child) + 1;
  memcpy(dst, child, len);
  dst[len] = 0;

  if (leaf)
  {
    if (!jack_proxy_reset_parameter_value(address))
    {
      log_error("cannot reset value of parameter");
      return false;
    }
  }
  else
  {
    if (!jack_proxy_read_conf_container(address, context, reset_callback))
    {
      log_error("cannot read container");
      return false;
    }
  }

  *dst = 0;

  return true;
}

bool jack_reset_all_params(void)
{
  char address[1024] = "";

  return jack_proxy_read_conf_container(address, NULL, reset_callback);
}
