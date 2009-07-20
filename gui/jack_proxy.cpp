// -*- Mode: C++ ; indent-tabs-mode: t -*-
/* This file is part of Patchage.
 * Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
 * 
 * Patchage is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Patchage is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <cassert>
#include <cstring>
#include <set>
#include <iostream>

#include "config.h"

#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "common.hpp"
#include "Patchage.hpp"
#include "jack_proxy.hpp"
#include "dbus_helpers.h"

using namespace std;

#define JACKDBUS_SERVICE         "org.jackaudio.service"
#define JACKDBUS_OBJECT          "/org/jackaudio/Controller"
#define JACKDBUS_IFACE_CONTROL   "org.jackaudio.JackControl"
#define JACKDBUS_IFACE_PATCHBAY  "org.jackaudio.JackPatchbay"

#define JACKDBUS_PORT_FLAG_INPUT         0x00000001
#define JACKDBUS_PORT_FLAG_OUTPUT        0x00000002
#define JACKDBUS_PORT_FLAG_PHYSICAL      0x00000004
#define JACKDBUS_PORT_FLAG_CAN_MONITOR   0x00000008
#define JACKDBUS_PORT_FLAG_TERMINAL      0x00000010

#define JACKDBUS_PORT_TYPE_AUDIO    0
#define JACKDBUS_PORT_TYPE_MIDI     1

//#define USE_FULL_REFRESH


jack_proxy::jack_proxy(Patchage* app)
  : _app(app)
  , _server_responding(false)
  , _server_started(false)
  , _graph_version(0)
  , _max_dsp_load(0.0)
{
  patchage_dbus_add_match("type='signal',interface='" DBUS_INTERFACE_DBUS "',member=NameOwnerChanged,arg0='" JACKDBUS_SERVICE "'");
#if defined(USE_FULL_REFRESH)
  patchage_dbus_add_match("type='signal',interface='" JACKDBUS_IFACE_PATCHBAY "',member=GraphChanged");
#else
  //   patchage_dbus_add_match("type='signal',interface='" JACKDBUS_IFACE_PATCHBAY "',member=ClientAppeared");
  //   patchage_dbus_add_match("type='signal',interface='" JACKDBUS_IFACE_PATCHBAY "',member=ClientDisappeared");
  patchage_dbus_add_match("type='signal',interface='" JACKDBUS_IFACE_PATCHBAY "',member=PortAppeared");
  patchage_dbus_add_match("type='signal',interface='" JACKDBUS_IFACE_PATCHBAY "',member=PortDisappeared");
  patchage_dbus_add_match("type='signal',interface='" JACKDBUS_IFACE_PATCHBAY "',member=PortsConnected");
  patchage_dbus_add_match("type='signal',interface='" JACKDBUS_IFACE_PATCHBAY "',member=PortsDisconnected");
#endif
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
  dbus_uint64_t new_graph_version;
  dbus_uint64_t client_id;
  const char *client_name;
  dbus_uint64_t port_id;
  const char *port_name;
  dbus_uint32_t port_flags;
  dbus_uint32_t port_type;
  dbus_uint64_t client2_id;
  const char *client2_name;
  dbus_uint64_t port2_id;
  const char *port2_name;
  dbus_uint64_t connection_id;
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

    if ((string)object_name != JACKDBUS_SERVICE)
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

#if defined(USE_FULL_REFRESH)
  if (dbus_message_is_signal(message, JACKDBUS_IFACE_PATCHBAY, "GraphChanged")) {
    if (!dbus_message_get_args(message, &g_dbus_error,
          DBUS_TYPE_UINT64, &new_graph_version,
          DBUS_TYPE_INVALID)) {
      me->error_msg(str(boost::format("dbus_message_get_args() failed to extract "
          "GraphChanged signal arguments (%s)") % g_dbus_error.message));
      dbus_error_free(&g_dbus_error);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (!me->_server_started) {
      me->_server_started = true;
      me->signal_attached.emit();
    }

    if (new_graph_version > me->_graph_version) {
      me->refresh_internal(false);
    }

    return DBUS_HANDLER_RESULT_HANDLED;
  }
#else
//  if (dbus_message_is_signal(message, JACKDBUS_IFACE_PATCHBAY, "ClientAppeared")) {
//    me->info_msg("ClientAppeared");
//     return DBUS_HANDLER_RESULT_HANDLED;
//  }

//  if (dbus_message_is_signal(message, JACKDBUS_IFACE_PATCHBAY, "ClientDisappeared")) {
//    me->info_msg("ClientDisappeared");
//     return DBUS_HANDLER_RESULT_HANDLED;
//  }

  if (dbus_message_is_signal(message, JACKDBUS_IFACE_PATCHBAY, "PortAppeared")) {
    if (!dbus_message_get_args( message, &g_dbus_error,
          DBUS_TYPE_UINT64, &new_graph_version,
          DBUS_TYPE_UINT64, &client_id,
          DBUS_TYPE_STRING, &client_name,
          DBUS_TYPE_UINT64, &port_id,
          DBUS_TYPE_STRING, &port_name,
          DBUS_TYPE_UINT32, &port_flags,
          DBUS_TYPE_UINT32, &port_type,
          DBUS_TYPE_INVALID)) {
      me->error_msg(str(boost::format("dbus_message_get_args() failed to extract "
          "PortAppeared signal arguments (%s)") % g_dbus_error.message));
      dbus_error_free(&g_dbus_error);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    //me->info_msg(str(boost::format("PortAppeared, %s(%llu):%s(%llu), %lu, %lu") % client_name % client_id % port_name % port_id % port_flags % port_type));

    if (!me->_server_started) {
      me->_server_started = true;
      me->signal_started.emit();
    }

    me->on_port_added(client_id, client_name, port_id, port_name, port_flags, port_type);

    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_signal(message, JACKDBUS_IFACE_PATCHBAY, "PortDisappeared")) {
    if (!dbus_message_get_args( message, &g_dbus_error,
          DBUS_TYPE_UINT64, &new_graph_version,
          DBUS_TYPE_UINT64, &client_id,
          DBUS_TYPE_STRING, &client_name,
          DBUS_TYPE_UINT64, &port_id,
          DBUS_TYPE_STRING, &port_name,
          DBUS_TYPE_INVALID)) {
      me->error_msg(str(boost::format("dbus_message_get_args() failed to extract "
          "PortDisappeared signal arguments (%s)") % g_dbus_error.message));
      dbus_error_free(&g_dbus_error);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    //me->info_msg(str(boost::format("PortDisappeared, %s(%llu):%s(%llu)") % client_name % client_id % port_name % port_id));

    if (!me->_server_started) {
      me->_server_started = true;
      me->signal_started.emit();
    }

    me->on_port_removed(client_id, client_name, port_id, port_name);

    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_signal(message, JACKDBUS_IFACE_PATCHBAY, "PortsConnected")) {
    if (!dbus_message_get_args(message, &g_dbus_error,
        DBUS_TYPE_UINT64, &new_graph_version,
        DBUS_TYPE_UINT64, &client_id,
        DBUS_TYPE_STRING, &client_name,
        DBUS_TYPE_UINT64, &port_id,
        DBUS_TYPE_STRING, &port_name,
        DBUS_TYPE_UINT64, &client2_id,
        DBUS_TYPE_STRING, &client2_name,
        DBUS_TYPE_UINT64, &port2_id,
        DBUS_TYPE_STRING, &port2_name,
        DBUS_TYPE_UINT64, &connection_id,
        DBUS_TYPE_INVALID)) {
      me->error_msg(str(boost::format("dbus_message_get_args() failed to extract "
          "PortsConnected signal arguments (%s)") % g_dbus_error.message));
      dbus_error_free(&g_dbus_error);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (!me->_server_started) {
      me->_server_started = true;
      me->signal_started.emit();
    }

    me->on_ports_connected(
      connection_id,
      client_id, client_name,
      port_id, port_name,
      client2_id, client2_name,
      port2_id, port2_name);

    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_signal(message, JACKDBUS_IFACE_PATCHBAY, "PortsDisconnected")) {
    if (!dbus_message_get_args(message, &g_dbus_error,
          DBUS_TYPE_UINT64, &new_graph_version,
          DBUS_TYPE_UINT64, &client_id,
          DBUS_TYPE_STRING, &client_name,
          DBUS_TYPE_UINT64, &port_id,
          DBUS_TYPE_STRING, &port_name,
          DBUS_TYPE_UINT64, &client2_id,
          DBUS_TYPE_STRING, &client2_name,
          DBUS_TYPE_UINT64, &port2_id,
          DBUS_TYPE_STRING, &port2_name,
          DBUS_TYPE_UINT64, &connection_id,
          DBUS_TYPE_INVALID)) {
      me->error_msg(str(boost::format("dbus_message_get_args() failed to extract "
          "PortsConnected signal arguments (%s)") % g_dbus_error.message));
      dbus_error_free(&g_dbus_error);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (!me->_server_started) {
      me->_server_started = true;
      me->signal_started.emit();
    }

    me->on_ports_disconnected(
      connection_id,
      client_id, client_name,
      port_id, port_name,
      client2_id, client2_name,
      port2_id, port2_name);

    return DBUS_HANDLER_RESULT_HANDLED;
  }
#endif

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
jack_proxy::refresh_internal(bool force)
{
  DBusMessage* reply_ptr;
  DBusMessageIter iter;
  dbus_uint64_t version;
  const char * reply_signature;
  DBusMessageIter clients_array_iter;
  DBusMessageIter client_struct_iter;
  DBusMessageIter ports_array_iter;
  DBusMessageIter port_struct_iter;
  DBusMessageIter connections_array_iter;
  DBusMessageIter connection_struct_iter;
  dbus_uint64_t client_id;
  const char *client_name;
  dbus_uint64_t port_id;
  const char *port_name;
  dbus_uint32_t port_flags;
  dbus_uint32_t port_type;
  dbus_uint64_t client2_id;
  const char *client2_name;
  dbus_uint64_t port2_id;
  const char *port2_name;
  dbus_uint64_t connection_id;

  if (!_server_started)
  {
    info_msg("ignoring refresh request because JACK server is stopped");
    return;
  }

  if (force) {
    version = 0; // workaround module split/join stupidity
  } else {
    version = _graph_version;
  }

  if (!call(true, JACKDBUS_IFACE_PATCHBAY, "GetGraph", &reply_ptr, DBUS_TYPE_UINT64, &version, DBUS_TYPE_INVALID)) {
    error_msg("GetGraph() failed.");
    return;
  }

  reply_signature = dbus_message_get_signature(reply_ptr);

  if (strcmp(reply_signature, "ta(tsa(tsuu))a(tstststst)") != 0) {
    error_msg((string )"GetGraph() reply signature mismatch. " + reply_signature);
    goto unref;
  }

  dbus_message_iter_init(reply_ptr, &iter);

  //info_msg((string)"version " + (char)dbus_message_iter_get_arg_type(&iter));
  dbus_message_iter_get_basic(&iter, &version);
  dbus_message_iter_next(&iter);

  if (!force && version <= _graph_version) {
    goto unref;
  }

  _app->clear_canvas();

  //info_msg(str(boost::format("got new graph version %llu") % version));
  _graph_version = version;

  //info_msg((string)"clients " + (char)dbus_message_iter_get_arg_type(&iter));

  for (dbus_message_iter_recurse(&iter, &clients_array_iter);
       dbus_message_iter_get_arg_type(&clients_array_iter) != DBUS_TYPE_INVALID;
       dbus_message_iter_next(&clients_array_iter)) {
    //info_msg((string)"a client " + (char)dbus_message_iter_get_arg_type(&clients_array_iter));
    dbus_message_iter_recurse(&clients_array_iter, &client_struct_iter);

    dbus_message_iter_get_basic(&client_struct_iter, &client_id);
    dbus_message_iter_next(&client_struct_iter);

    dbus_message_iter_get_basic(&client_struct_iter, &client_name);
    dbus_message_iter_next(&client_struct_iter);

    //info_msg((string)"client '" + client_name + "'");

    for (dbus_message_iter_recurse(&client_struct_iter, &ports_array_iter);
         dbus_message_iter_get_arg_type(&ports_array_iter) != DBUS_TYPE_INVALID;
         dbus_message_iter_next(&ports_array_iter)) {
      //info_msg((string)"a port " + (char)dbus_message_iter_get_arg_type(&ports_array_iter));
      dbus_message_iter_recurse(&ports_array_iter, &port_struct_iter);

      dbus_message_iter_get_basic(&port_struct_iter, &port_id);
      dbus_message_iter_next(&port_struct_iter);

      dbus_message_iter_get_basic(&port_struct_iter, &port_name);
      dbus_message_iter_next(&port_struct_iter);

      dbus_message_iter_get_basic(&port_struct_iter, &port_flags);
      dbus_message_iter_next(&port_struct_iter);

      dbus_message_iter_get_basic(&port_struct_iter, &port_type);
      dbus_message_iter_next(&port_struct_iter);

      //info_msg((string)"port: " + port_name);

      on_port_added(client_id, client_name, port_id, port_name, port_flags, port_type);
    }

    dbus_message_iter_next(&client_struct_iter);
  }

  dbus_message_iter_next(&iter);

  for (dbus_message_iter_recurse(&iter, &connections_array_iter);
       dbus_message_iter_get_arg_type(&connections_array_iter) != DBUS_TYPE_INVALID;
       dbus_message_iter_next(&connections_array_iter)) {
    //info_msg((string)"a connection " + (char)dbus_message_iter_get_arg_type(&connections_array_iter));
    dbus_message_iter_recurse(&connections_array_iter, &connection_struct_iter);

    dbus_message_iter_get_basic(&connection_struct_iter, &client_id);
    dbus_message_iter_next(&connection_struct_iter);

    dbus_message_iter_get_basic(&connection_struct_iter, &client_name);
    dbus_message_iter_next(&connection_struct_iter);

    dbus_message_iter_get_basic(&connection_struct_iter, &port_id);
    dbus_message_iter_next(&connection_struct_iter);

    dbus_message_iter_get_basic(&connection_struct_iter, &port_name);
    dbus_message_iter_next(&connection_struct_iter);

    dbus_message_iter_get_basic(&connection_struct_iter, &client2_id);
    dbus_message_iter_next(&connection_struct_iter);

    dbus_message_iter_get_basic(&connection_struct_iter, &client2_name);
    dbus_message_iter_next(&connection_struct_iter);

    dbus_message_iter_get_basic(&connection_struct_iter, &port2_id);
    dbus_message_iter_next(&connection_struct_iter);

    dbus_message_iter_get_basic(&connection_struct_iter, &port2_name);
    dbus_message_iter_next(&connection_struct_iter);

    dbus_message_iter_get_basic(&connection_struct_iter, &connection_id);
    dbus_message_iter_next(&connection_struct_iter);

    //info_msg(str(boost::format("connection(%llu) %s(%llu):%s(%llu) <-> %s(%llu):%s(%llu)") %
    //    connection_id %
    //    client_name %
    //    client_id %
    //    port_name %
    //    port_id %
    //    client2_name %
    //    client2_id %
    //    port2_name %
    //    port2_id));

    on_ports_connected(
      connection_id,
      client_id, client_name,
      port_id, port_name,
      client2_id, client2_name,
      port2_id, port2_name);
  }

unref:
  dbus_message_unref(reply_ptr);
}


void
jack_proxy::refresh()
{
  refresh_internal(true);
}


bool
jack_proxy::connect(
  const char * client1_name,
  const char * port1_name,
  const char * client2_name,
  const char * port2_name)
{
  DBusMessage* reply_ptr;

  if (!call(true, JACKDBUS_IFACE_PATCHBAY, "ConnectPortsByName", &reply_ptr,
        DBUS_TYPE_STRING, &client1_name,
        DBUS_TYPE_STRING, &port1_name,
        DBUS_TYPE_STRING, &client2_name,
        DBUS_TYPE_STRING, &port2_name,
        DBUS_TYPE_INVALID)) {
    error_msg("ConnectPortsByName() failed.");
    return false;
  }

  return true;
}


bool
jack_proxy::disconnect(
  const char * client1_name,
  const char * port1_name,
  const char * client2_name,
  const char * port2_name)
{
  DBusMessage* reply_ptr;

  if (!call(true, JACKDBUS_IFACE_PATCHBAY, "DisconnectPortsByName", &reply_ptr,
        DBUS_TYPE_STRING, &client1_name,
        DBUS_TYPE_STRING, &port1_name,
        DBUS_TYPE_STRING, &client2_name,
        DBUS_TYPE_STRING, &port2_name,
        DBUS_TYPE_INVALID)) {
    error_msg("DisconnectPortsByName() failed.");
    return false;
  }

  return true;
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
