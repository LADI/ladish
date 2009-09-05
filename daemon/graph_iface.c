/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari
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
#include "graph_iface.h"
#include "../dbus/error.h"

#define impl_ptr ((struct graph_implementator *)call_ptr->iface_context)

static void get_all_ports(struct dbus_method_call * call_ptr)
{
  DBusMessageIter iter, sub_iter;

  call_ptr->reply = dbus_message_new_method_return(call_ptr->message);
  if (call_ptr->reply == NULL)
  {
    goto fail;
  }

  dbus_message_iter_init_append(call_ptr->reply, &iter);

  if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &sub_iter))
  {
    goto fail_unref;
  }

  if (!dbus_message_iter_close_container(&iter, &sub_iter))
  {
    goto fail_unref;
  }

  return;

fail_unref:
  dbus_message_unref(call_ptr->reply);
  call_ptr->reply = NULL;

fail:
  lash_error("Ran out of memory trying to construct method return");
}

static void get_graph(struct dbus_method_call * call_ptr)
{
  dbus_uint64_t known_version;
  dbus_uint64_t current_version;
  DBusMessageIter iter;
  DBusMessageIter clients_array_iter;
  DBusMessageIter connections_array_iter;
#if 0
  DBusMessageIter ports_array_iter;
  DBusMessageIter client_struct_iter;
  DBusMessageIter port_struct_iter;
  DBusMessageIter connection_struct_iter;
#endif

  //lash_info("get_graph() called");

  if (!dbus_message_get_args(call_ptr->message, &g_dbus_error, DBUS_TYPE_UINT64, &known_version, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  //lash_info("Getting graph, known version is %" PRIu64, known_version);

  call_ptr->reply = dbus_message_new_method_return(call_ptr->message);
  if (call_ptr->reply == NULL)
  {
    lash_error("Ran out of memory trying to construct method return");
    goto exit;
  }

  dbus_message_iter_init_append(call_ptr->reply, &iter);

  current_version = impl_ptr->get_graph_version(impl_ptr->this);
  if (known_version > current_version)
  {
    lash_dbus_error(
      call_ptr,
      LASH_DBUS_ERROR_INVALID_ARGS,
      "known graph version %" PRIu64 " is newer than actual version %" PRIu64,
      known_version,
      current_version);
    goto exit;
  }

  if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT64, &current_version))
  {
    goto nomem;
  }

  if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(tsa(tsuu))", &clients_array_iter))
  {
    goto nomem;
  }

  if (known_version < current_version)
  {
#if 0
    list_for_each(client_node_ptr, &patchbay_ptr->graph.clients)
    {
      client_ptr = list_entry(client_node_ptr, struct jack_graph_client, siblings);

      if (!dbus_message_iter_open_container (&clients_array_iter, DBUS_TYPE_STRUCT, NULL, &client_struct_iter))
      {
        goto nomem_close_clients_array;
      }

      if (!dbus_message_iter_append_basic(&client_struct_iter, DBUS_TYPE_UINT64, &client_ptr->id))
      {
        goto nomem_close_client_struct;
      }

      if (!dbus_message_iter_append_basic(&client_struct_iter, DBUS_TYPE_STRING, &client_ptr->name))
      {
        goto nomem_close_client_struct;
      }

      if (!dbus_message_iter_open_container(&client_struct_iter, DBUS_TYPE_ARRAY, "(tsuu)", &ports_array_iter))
      {
        goto nomem_close_client_struct;
      }

      list_for_each(port_node_ptr, &client_ptr->ports)
      {
        port_ptr = list_entry(port_node_ptr, struct jack_graph_port, siblings_client);

        if (!dbus_message_iter_open_container(&ports_array_iter, DBUS_TYPE_STRUCT, NULL, &port_struct_iter))
        {
          goto nomem_close_ports_array;
        }

        if (!dbus_message_iter_append_basic(&port_struct_iter, DBUS_TYPE_UINT64, &port_ptr->id))
        {
          goto nomem_close_port_struct;
        }

        if (!dbus_message_iter_append_basic(&port_struct_iter, DBUS_TYPE_STRING, &port_ptr->name))
        {
          goto nomem_close_port_struct;
        }

        if (!dbus_message_iter_append_basic(&port_struct_iter, DBUS_TYPE_UINT32, &port_ptr->flags))
        {
          goto nomem_close_port_struct;
        }

        if (!dbus_message_iter_append_basic(&port_struct_iter, DBUS_TYPE_UINT32, &port_ptr->type))
        {
          goto nomem_close_port_struct;
        }

        if (!dbus_message_iter_close_container(&ports_array_iter, &port_struct_iter))
        {
          goto nomem_close_ports_array;
        }
      }

      if (!dbus_message_iter_close_container(&client_struct_iter, &ports_array_iter))
      {
        goto nomem_close_client_struct;
      }

      if (!dbus_message_iter_close_container(&clients_array_iter, &client_struct_iter))
      {
        goto nomem_close_clients_array;
      }
    }
#endif
  }

  if (!dbus_message_iter_close_container(&iter, &clients_array_iter))
  {
    goto nomem;
  }

  if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(tstststst)", &connections_array_iter))
  {
    goto nomem;
  }

  if (known_version < current_version)
  {
#if 0
    list_for_each(connection_node_ptr, &patchbay_ptr->graph.connections)
    {
      connection_ptr = list_entry(connection_node_ptr, struct jack_graph_connection, siblings);

      if (!dbus_message_iter_open_container(&connections_array_iter, DBUS_TYPE_STRUCT, NULL, &connection_struct_iter))
      {
        goto nomem_close_connections_array;
      }

      if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_UINT64, &connection_ptr->port1->client->id))
      {
        goto nomem_close_connection_struct;
      }

      if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_STRING, &connection_ptr->port1->client->name))
      {
        goto nomem_close_connection_struct;
      }

      if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_UINT64, &connection_ptr->port1->id))
      {
        goto nomem_close_connection_struct;
      }

      if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_STRING, &connection_ptr->port1->name))
      {
        goto nomem_close_connection_struct;
      }

      if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_UINT64, &connection_ptr->port2->client->id))
      {
        goto nomem_close_connection_struct;
      }

      if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_STRING, &connection_ptr->port2->client->name))
      {
        goto nomem_close_connection_struct;
      }

      if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_UINT64, &connection_ptr->port2->id))
      {
        goto nomem_close_connection_struct;
      }

      if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_STRING, &connection_ptr->port2->name))
      {
        goto nomem_close_connection_struct;
      }

      if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_UINT64, &connection_ptr->id))
      {
        goto nomem_close_connection_struct;
      }

      if (!dbus_message_iter_close_container(&connections_array_iter, &connection_struct_iter))
      {
        goto nomem_close_connections_array;
      }
    }
#endif
  }

  if (!dbus_message_iter_close_container(&iter, &connections_array_iter))
  {
    goto nomem;
  }

  return;

#if 0
nomem_close_connection_struct:
  dbus_message_iter_close_container(&connections_array_iter, &connection_struct_iter);

nomem_close_connections_array:
  dbus_message_iter_close_container(&iter, &connections_array_iter);
  goto nomem_unlock;

nomem_close_port_struct:
  dbus_message_iter_close_container(&ports_array_iter, &port_struct_iter);

nomem_close_ports_array:
  dbus_message_iter_close_container(&client_struct_iter, &ports_array_iter);

nomem_close_client_struct:
  dbus_message_iter_close_container(&clients_array_iter, &client_struct_iter);

nomem_close_clients_array:
  dbus_message_iter_close_container(&iter, &clients_array_iter);
#endif

nomem:
  dbus_message_unref(call_ptr->reply);
  call_ptr->reply = NULL;
  lash_error("Ran out of memory trying to construct method return");

exit:
  return;
}

static void connect_ports_by_name(struct dbus_method_call * call_ptr)
{
  method_return_new_void(call_ptr);
}

static void connect_ports_by_id(struct dbus_method_call * call_ptr)
{
  method_return_new_void(call_ptr);
}

static void disconnect_ports_by_name(struct dbus_method_call * call_ptr)
{
  method_return_new_void(call_ptr);
}

static void disconnect_ports_by_id(struct dbus_method_call * call_ptr)
{
  method_return_new_void(call_ptr);
}

static void disconnect_ports_by_connection_id(struct dbus_method_call * call_ptr)
{
  method_return_new_void(call_ptr);
}

static void get_client_pid(struct dbus_method_call * call_ptr)
{
  int64_t pid = 0;
  method_return_new_single(call_ptr, DBUS_TYPE_INT64, &pid);
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
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
  INTERFACE_EXPOSE_SIGNALS
INTERFACE_END
