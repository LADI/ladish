/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the D-Bus patchbay interface helpers
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
#include "patchbay.h"

static void get_all_ports(method_call_t * call_ptr)
{
}

static void get_graph(method_call_t * call_ptr)
{
}

static void connect_ports_by_name(method_call_t * call_ptr)
{
}

static void connect_ports_by_id(method_call_t * call_ptr)
{
}

static void disconnect_ports_by_name(method_call_t * call_ptr)
{
}

static void disconnect_ports_by_id(method_call_t * call_ptr)
{
}

static void disconnect_ports_by_connection_id(method_call_t * call_ptr)
{
}

static void get_client_pid(method_call_t * call_ptr)
{
}


METHOD_ARGS_BEGIN(GetAllPorts, "Get all ports")
    METHOD_ARG_DESCRIBE_IN("ports_list", "as", "List of all ports")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(GetGraph, "Get whole graph")
METHOD_ARG_DESCRIBE_IN("known_graph_version", DBUS_TYPE_UINT64_AS_STRING, "Known graph version")
    METHOD_ARG_DESCRIBE_OUT("current_graph_version", DBUS_TYPE_UINT64_AS_STRING, "Current graph version")
    METHOD_ARG_DESCRIBE_OUT("clients_and_ports", "a(tsa(tsuu))", "Clients and their ports")
    METHOD_ARG_DESCRIBE_OUT("connections", "a(tstststst)", "Connections array")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(ConnectPortsByName, "Connect ports")
    METHOD_ARG_DESCRIBE_IN("client1_name", DBUS_TYPE_STRING_AS_STRING, "name first port client")
    METHOD_ARG_DESCRIBE_IN("port1_name", DBUS_TYPE_STRING_AS_STRING, "name of first port")
    METHOD_ARG_DESCRIBE_IN("client2_name", DBUS_TYPE_STRING_AS_STRING, "name second port client")
    METHOD_ARG_DESCRIBE_IN("port2_name", DBUS_TYPE_STRING_AS_STRING, "name of second port")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(ConnectPortsByID, "Connect ports")
    METHOD_ARG_DESCRIBE_IN("port1_id", DBUS_TYPE_UINT64_AS_STRING, "id of first port")
    METHOD_ARG_DESCRIBE_IN("port2_id", DBUS_TYPE_UINT64_AS_STRING, "if of second port")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(DisconnectPortsByName, "Disconnect ports")
    METHOD_ARG_DESCRIBE_IN("client1_name", DBUS_TYPE_STRING_AS_STRING, "name first port client")
    METHOD_ARG_DESCRIBE_IN("port1_name", DBUS_TYPE_STRING_AS_STRING, "name of first port")
    METHOD_ARG_DESCRIBE_IN("client2_name", DBUS_TYPE_STRING_AS_STRING, "name second port client")
    METHOD_ARG_DESCRIBE_IN("port2_name", DBUS_TYPE_STRING_AS_STRING, "name of second port")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(DisconnectPortsByID, "Disconnect ports")
    METHOD_ARG_DESCRIBE_IN("port1_id", DBUS_TYPE_UINT64_AS_STRING, "id of first port")
    METHOD_ARG_DESCRIBE_IN("port2_id", DBUS_TYPE_UINT64_AS_STRING, "if of second port")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(DisconnectPortsByConnectionID, "Disconnect ports")
    METHOD_ARG_DESCRIBE_IN("connection_id", DBUS_TYPE_UINT64_AS_STRING, "id of connection to disconnect")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(GetClientPID, "get process id of client")
    METHOD_ARG_DESCRIBE_IN("client_id", DBUS_TYPE_UINT64_AS_STRING, "id of client")
    METHOD_ARG_DESCRIBE_OUT("process_id", DBUS_TYPE_INT64_AS_STRING, "pid of client")
METHOD_ARGS_END

METHODS_BEGIN
    METHOD_DESCRIBE(GetAllPorts, get_all_ports)
    METHOD_DESCRIBE(GetGraph, get_graph)
    METHOD_DESCRIBE(ConnectPortsByName, connect_ports_by_name)
    METHOD_DESCRIBE(ConnectPortsByID, connect_ports_by_id)
    METHOD_DESCRIBE(DisconnectPortsByName, disconnect_ports_by_name)
    METHOD_DESCRIBE(DisconnectPortsByID, disconnect_ports_by_id)
    METHOD_DESCRIBE(DisconnectPortsByConnectionID, disconnect_ports_by_connection_id)
    METHOD_DESCRIBE(GetClientPID, get_client_pid)
METHODS_END

SIGNAL_ARGS_BEGIN(GraphChanged, "")
    SIGNAL_ARG_DESCRIBE("new_graph_version", DBUS_TYPE_UINT64_AS_STRING, "")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(ClientAppeared, "")
    SIGNAL_ARG_DESCRIBE("new_graph_version", DBUS_TYPE_UINT64_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("client_id", DBUS_TYPE_UINT64_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("client_name", DBUS_TYPE_STRING_AS_STRING, "")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(ClientDisappeared, "")
    SIGNAL_ARG_DESCRIBE("new_graph_version", DBUS_TYPE_UINT64_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("client_id", DBUS_TYPE_UINT64_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("client_name", DBUS_TYPE_STRING_AS_STRING, "")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(PortAppeared, "")
    SIGNAL_ARG_DESCRIBE("new_graph_version", DBUS_TYPE_UINT64_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("client_id", DBUS_TYPE_UINT64_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("client_name", DBUS_TYPE_STRING_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("port_id", DBUS_TYPE_UINT64_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("port_name", DBUS_TYPE_STRING_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("port_flags", DBUS_TYPE_UINT32_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("port_type", DBUS_TYPE_UINT32_AS_STRING, "")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(PortDisappeared, "")
    SIGNAL_ARG_DESCRIBE("new_graph_version", DBUS_TYPE_UINT64_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("client_id", DBUS_TYPE_UINT64_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("client_name", DBUS_TYPE_STRING_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("port_id", DBUS_TYPE_UINT64_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("port_name", DBUS_TYPE_STRING_AS_STRING, "")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(PortsConnected, "")
    SIGNAL_ARG_DESCRIBE("new_graph_version", DBUS_TYPE_UINT64_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("client1_id", DBUS_TYPE_UINT64_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("client1_name", DBUS_TYPE_STRING_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("port1_id", DBUS_TYPE_UINT64_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("port1_name", DBUS_TYPE_STRING_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("client2_id", DBUS_TYPE_UINT64_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("client2_name", DBUS_TYPE_STRING_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("port2_id", DBUS_TYPE_UINT64_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("port2_name", DBUS_TYPE_STRING_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("connection_id", DBUS_TYPE_UINT64_AS_STRING, "")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(PortsDisconnected, "")
    SIGNAL_ARG_DESCRIBE("new_graph_version", DBUS_TYPE_UINT64_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("client1_id", DBUS_TYPE_UINT64_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("client1_name", DBUS_TYPE_STRING_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("port1_id", DBUS_TYPE_UINT64_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("port1_name", DBUS_TYPE_STRING_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("client2_id", DBUS_TYPE_UINT64_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("client2_name", DBUS_TYPE_STRING_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("port2_id", DBUS_TYPE_UINT64_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("port2_name", DBUS_TYPE_STRING_AS_STRING, "")
    SIGNAL_ARG_DESCRIBE("connection_id", DBUS_TYPE_UINT64_AS_STRING, "")
SIGNAL_ARGS_END

SIGNALS_BEGIN
    SIGNAL_DESCRIBE(GraphChanged)
    SIGNAL_DESCRIBE(ClientAppeared)
    SIGNAL_DESCRIBE(ClientDisappeared)
    SIGNAL_DESCRIBE(PortAppeared)
    SIGNAL_DESCRIBE(PortDisappeared)
    SIGNAL_DESCRIBE(PortsConnected)
    SIGNAL_DESCRIBE(PortsDisconnected)
SIGNALS_END

INTERFACE_BEGIN(g_interface_patchbay, "org.jackaudio.JackPatchbay")
    INTERFACE_EXPOSE_METHODS
    INTERFACE_EXPOSE_SIGNALS
INTERFACE_END
