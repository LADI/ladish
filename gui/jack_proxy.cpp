/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains code that interface with JACK through D-Bus
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

#include "common.h"

#include <cassert>
#include <cstring>
#include <set>
#include <iostream>

#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "Patchage.hpp"
#include "jack_proxy.hpp"
#include "dbus_helpers.h"

#define JACKDBUS_SERVICE         "org.jackaudio.service"
#define JACKDBUS_OBJECT          "/org/jackaudio/Controller"
#define JACKDBUS_IFACE_CONTROL   "org.jackaudio.JackControl"

#define JACKDBUS_PORT_FLAG_INPUT         0x00000001
#define JACKDBUS_PORT_FLAG_OUTPUT        0x00000002
#define JACKDBUS_PORT_FLAG_PHYSICAL      0x00000004
#define JACKDBUS_PORT_FLAG_CAN_MONITOR   0x00000008
#define JACKDBUS_PORT_FLAG_TERMINAL      0x00000010

#define JACKDBUS_PORT_TYPE_AUDIO    0
#define JACKDBUS_PORT_TYPE_MIDI     1

jack_proxy::jack_proxy(Patchage* app)
  : _app(app)
  , _server_responding(false)
  , _server_started(false)
  , _graph_version(0)
  , _max_dsp_load(0.0)
{
  patchage_dbus_add_match("type='signal',interface='" DBUS_INTERFACE_DBUS "',member=NameOwnerChanged,arg0='" JACKDBUS_SERVICE "'");
  patchage_dbus_add_filter(dbus_message_hook, this);

  update_attached();

  if (!_server_responding) {
    return;
  }

  refresh();                    // the initial refresh
}


jack_proxy::~jack_proxy()
{
}

void
jack_proxy::update_attached()
{
  bool was_started = _server_started;
  _server_started = is_started();

  if (!_server_responding) {
    if (was_started) {
      signal_stopped.emit();
    }
    return;
  }

  if (_server_started && !was_started) {
    signal_started.emit();
    return;
  }

  if (!_server_started && was_started) {
    signal_stopped.emit();
    return;
  }
}


void
jack_proxy::on_jack_appeared()
{
  info_msg("JACK appeared.");
  update_attached();
}


void
jack_proxy::on_jack_disappeared()
{
  info_msg("JACK disappeared.");

  // we are not calling update_attached() here, because it will activate jackdbus

  _server_responding = false;

  if (_server_started) {
    signal_stopped.emit();
  }

  _server_started = false;
}


DBusHandlerResult
jack_proxy::dbus_message_hook(
  DBusConnection* connection,
  DBusMessage*    message,
  void*           jack_driver)
{
  const char *object_name;
  const char *old_owner;
  const char *new_owner;

  assert(jack_driver);
  jack_proxy* me = reinterpret_cast<jack_proxy*>(jack_driver);

  //me->info_msg("dbus_message_hook() called.");

  // Handle signals we have subscribed for in attach()

  if (dbus_message_is_signal(message, DBUS_INTERFACE_DBUS, "NameOwnerChanged")) {
    if (!dbus_message_get_args( message, &g_dbus_error,
          DBUS_TYPE_STRING, &object_name,
          DBUS_TYPE_STRING, &old_owner,
          DBUS_TYPE_STRING, &new_owner,
          DBUS_TYPE_INVALID)) {
      me->error_msg(str(boost::format("dbus_message_get_args() failed to extract "
          "NameOwnerChanged signal arguments (%s)") % g_dbus_error.message));
      dbus_error_free(&g_dbus_error);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    if ((std::string)object_name != JACKDBUS_SERVICE)
    {
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    if (old_owner[0] == '\0') {
      me->on_jack_appeared();
    } else if (new_owner[0] == '\0') {
      me->on_jack_disappeared();
    }

    return DBUS_HANDLER_RESULT_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

bool
jack_proxy::call(
  bool          response_expected,
  const char*   iface,
  const char*   method,
  DBusMessage** reply_ptr_ptr,
  int           in_type, ...)
{
  va_list ap;

  va_start(ap, in_type);

  _server_responding = patchage_dbus_call_valist(
    response_expected,
    JACKDBUS_SERVICE,
    JACKDBUS_OBJECT,
    iface,
    method,
    reply_ptr_ptr,
    in_type,
    ap);

  va_end(ap);

  return _server_responding;
}

bool
jack_proxy::is_started()
{
  DBusMessage* reply_ptr;
  dbus_bool_t started;

  if (!call(false, JACKDBUS_IFACE_CONTROL, "IsStarted", &reply_ptr, DBUS_TYPE_INVALID)) {
    return false;
  }

  if (!dbus_message_get_args(reply_ptr, &g_dbus_error,
        DBUS_TYPE_BOOLEAN, &started,
        DBUS_TYPE_INVALID)) {
    dbus_message_unref(reply_ptr);
    dbus_error_free(&g_dbus_error);
    error_msg("decoding reply of IsStarted failed.");
    return false;
  }

  dbus_message_unref(reply_ptr);

  return started;
}


void
jack_proxy::start_server()
{
  DBusMessage* reply_ptr;

  if (!call(false, JACKDBUS_IFACE_CONTROL, "StartServer", &reply_ptr, DBUS_TYPE_INVALID)) {
    return;
  }

  dbus_message_unref(reply_ptr);

  update_attached();
}


void
jack_proxy::stop_server()
{
  DBusMessage* reply_ptr;

  if (!call(false, JACKDBUS_IFACE_CONTROL, "StopServer", &reply_ptr, DBUS_TYPE_INVALID)) {
    return;
  }

  dbus_message_unref(reply_ptr);
  
  if (!_server_started) {
    _server_started = false;
    signal_stopped.emit();
  }
}

void
jack_proxy::on_port_added(
  dbus_uint64_t client_id,
  const char * client_name,
  dbus_uint64_t port_id,
  const char * port_name,
  dbus_uint32_t port_flags,
  dbus_uint32_t port_type)
{
  PortType local_port_type;

  switch (port_type) {
  case JACKDBUS_PORT_TYPE_AUDIO:
    local_port_type = JACK_AUDIO;
    break;
  case JACKDBUS_PORT_TYPE_MIDI:
    local_port_type = JACK_MIDI;
    break;
  default:
    error_msg("Unknown JACK D-Bus port type");
    return;
  }

  _app->on_port_added(
    client_name,
    port_name,
    local_port_type,
    port_flags & JACKDBUS_PORT_FLAG_INPUT,
    port_flags & JACKDBUS_PORT_FLAG_TERMINAL);
}

void
jack_proxy::on_port_removed(
  dbus_uint64_t client_id,
  const char * client_name,
  dbus_uint64_t port_id,
  const char * port_name)
{
  _app->on_port_removed(client_name, port_name);

  if (_app->is_canvas_empty())
  {
    if (_server_started)
    {
      signal_stopped.emit();
    }

    _server_started = false;
  }
}

void
jack_proxy::on_ports_connected(
  dbus_uint64_t connection_id,
  dbus_uint64_t client1_id,
  const char*   client1_name,
  dbus_uint64_t port1_id,
  const char*   port1_name,
  dbus_uint64_t client2_id,
  const char*   client2_name,
  dbus_uint64_t port2_id,
  const char*   port2_name)
{
  _app->on_ports_connected(client1_name, port1_name, client2_name, port2_name);
}

void
jack_proxy::on_ports_disconnected(
  dbus_uint64_t connection_id,
  dbus_uint64_t client1_id,
  const char*   client1_name,
  dbus_uint64_t port1_id,
  const char*   port1_name,
  dbus_uint64_t client2_id,
  const char*   client2_name,
  dbus_uint64_t port2_id,
  const char*   port2_name)
{
  _app->on_ports_disconnected(client1_name, port1_name, client2_name, port2_name);
}

void
jack_proxy::refresh()
{
}

uint32_t
jack_proxy::buffer_size()
{
  DBusMessage* reply_ptr;
  dbus_uint32_t buffer_size;

  if (_server_responding && !_server_started) {
    goto fail;
  }

  if (!call(true, JACKDBUS_IFACE_CONTROL, "GetBufferSize", &reply_ptr, DBUS_TYPE_INVALID)) {
    goto fail;
  }

  if (!dbus_message_get_args(reply_ptr, &g_dbus_error, DBUS_TYPE_UINT32, &buffer_size, DBUS_TYPE_INVALID)) {
    dbus_message_unref(reply_ptr);
    dbus_error_free(&g_dbus_error);
    error_msg("decoding reply of GetBufferSize failed.");
    goto fail;
  }

  dbus_message_unref(reply_ptr);

  return buffer_size;

fail:
  return 4096; // something fake, patchage needs it to match combobox value
}


bool
jack_proxy::set_buffer_size(uint32_t size)
{
  DBusMessage* reply_ptr;
  dbus_uint32_t buffer_size;

  buffer_size = size;

  if (!call(true, JACKDBUS_IFACE_CONTROL, "SetBufferSize", &reply_ptr, DBUS_TYPE_UINT32, &buffer_size, DBUS_TYPE_INVALID)) {
    return false;
  }

  dbus_message_unref(reply_ptr);

  return true;
}


float
jack_proxy::sample_rate()
{
  DBusMessage* reply_ptr;
  double sample_rate;

  if (!call(true, JACKDBUS_IFACE_CONTROL, "GetSampleRate", &reply_ptr, DBUS_TYPE_INVALID)) {
    return false;
  }

  if (!dbus_message_get_args(reply_ptr, &g_dbus_error, DBUS_TYPE_DOUBLE, &sample_rate, DBUS_TYPE_INVALID)) {
    dbus_message_unref(reply_ptr);
    dbus_error_free(&g_dbus_error);
    error_msg("decoding reply of GetSampleRate failed.");
    return false;
  }

  dbus_message_unref(reply_ptr);

  return sample_rate;
}


bool
jack_proxy::is_realtime()
{
  DBusMessage* reply_ptr;
  dbus_bool_t realtime;

  if (!call(true, JACKDBUS_IFACE_CONTROL, "IsRealtime", &reply_ptr, DBUS_TYPE_INVALID)) {
    return false;
  }

  if (!dbus_message_get_args(reply_ptr, &g_dbus_error, DBUS_TYPE_BOOLEAN, &realtime, DBUS_TYPE_INVALID)) {
    dbus_message_unref(reply_ptr);
    dbus_error_free(&g_dbus_error);
    error_msg("decoding reply of IsRealtime failed.");
    return false;
  }

  dbus_message_unref(reply_ptr);

  return realtime;
}


size_t
jack_proxy::xruns()
{
  DBusMessage* reply_ptr;
  dbus_uint32_t xruns;

  if (_server_responding && !_server_started) {
    return 0;
  }

  if (!call(true, JACKDBUS_IFACE_CONTROL, "GetXruns", &reply_ptr, DBUS_TYPE_INVALID)) {
    return 0;
  }

  if (!dbus_message_get_args(reply_ptr, &g_dbus_error, DBUS_TYPE_UINT32, &xruns, DBUS_TYPE_INVALID)) {
    dbus_message_unref(reply_ptr);
    dbus_error_free(&g_dbus_error);
    error_msg("decoding reply of GetXruns failed.");
    return 0;
  }

  dbus_message_unref(reply_ptr);

  return xruns;
}


void
jack_proxy::reset_xruns()
{
  DBusMessage* reply_ptr;

  if (!call(true, JACKDBUS_IFACE_CONTROL, "ResetXruns", &reply_ptr, DBUS_TYPE_INVALID)) {
    return;
  }

  dbus_message_unref(reply_ptr);
}


float
jack_proxy::get_dsp_load()
{
  DBusMessage* reply_ptr;
  double load;

  if (_server_responding && !_server_started) {
    return 0.0;
  }

  if (!call(true, JACKDBUS_IFACE_CONTROL, "GetLoad", &reply_ptr, DBUS_TYPE_INVALID)) {
    return 0.0;
  }

  if (!dbus_message_get_args(reply_ptr, &g_dbus_error, DBUS_TYPE_DOUBLE, &load, DBUS_TYPE_INVALID)) {
    dbus_message_unref(reply_ptr);
    dbus_error_free(&g_dbus_error);
    error_msg("decoding reply of GetLoad failed.");
    return 0.0;
  }

  dbus_message_unref(reply_ptr);

  return load;
}

void
jack_proxy::start_transport()
{
  //info_msg(__func__);
}


void
jack_proxy::stop_transport()
{
  //info_msg(__func__);
}


void
jack_proxy::rewind_transport()
{
  //info_msg(__func__);
}

void
jack_proxy::error_msg(const std::string& msg) const
{
  _app->error_msg((std::string)"[JACKDBUS] " + msg);
}

void
jack_proxy::info_msg(const std::string& msg) const
{
  _app->info_msg((std::string)"[JACKDBUS] " + msg);
}
