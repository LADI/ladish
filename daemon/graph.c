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
#include "graph.h"
#include "../dbus/error.h"
#include "../dbus_constants.h"

struct ladish_graph_port
{
  struct list_head siblings_client;
  struct list_head siblings_graph;
  struct ladish_graph_client * client_ptr;
  char * name;
  uint32_t type;
  uint32_t flags;
  uint64_t id;
  ladish_port_handle port;
};

struct ladish_graph_client
{
  struct list_head siblings;
  char * name;
  uint64_t id;
  ladish_client_handle client;
  struct list_head ports;
};

struct ladish_graph
{
  char * opath;
  ladish_dict_handle dict;
  struct list_head clients;
  struct list_head ports;
  uint64_t graph_version;
  uint64_t next_client_id;
  uint64_t next_port_id;
  uint64_t next_connection_id;
};

#define graph_ptr ((struct ladish_graph *)call_ptr->iface_context)

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
  log_error("Ran out of memory trying to construct method return");
}

static void get_graph(struct dbus_method_call * call_ptr)
{
  dbus_uint64_t known_version;
  dbus_uint64_t current_version;
  DBusMessageIter iter;
  DBusMessageIter clients_array_iter;
  DBusMessageIter connections_array_iter;
  DBusMessageIter client_struct_iter;
  struct list_head * client_node_ptr;
  struct ladish_graph_client * client_ptr;
  DBusMessageIter ports_array_iter;
  struct list_head * port_node_ptr;
  struct ladish_graph_port * port_ptr;
  DBusMessageIter port_struct_iter;
  //DBusMessageIter connection_struct_iter;

  //log_info("get_graph() called");

  if (!dbus_message_get_args(call_ptr->message, &g_dbus_error, DBUS_TYPE_UINT64, &known_version, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  //log_info("Getting graph, known version is %" PRIu64, known_version);

  call_ptr->reply = dbus_message_new_method_return(call_ptr->message);
  if (call_ptr->reply == NULL)
  {
    log_error("Ran out of memory trying to construct method return");
    goto exit;
  }

  dbus_message_iter_init_append(call_ptr->reply, &iter);

  current_version = graph_ptr->graph_version;
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
    list_for_each(client_node_ptr, &graph_ptr->clients)
    {
      client_ptr = list_entry(client_node_ptr, struct ladish_graph_client, siblings);

      if (!dbus_message_iter_open_container (&clients_array_iter, DBUS_TYPE_STRUCT, NULL, &client_struct_iter))
      {
        goto nomem_close_clients_array;
      }

      if (!dbus_message_iter_append_basic(&client_struct_iter, DBUS_TYPE_UINT64, &client_ptr->id))
      {
        goto nomem_close_client_struct;
      }

      log_info("client '%s' (%llu)", client_ptr->name, (unsigned long long)client_ptr->id);
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
        port_ptr = list_entry(port_node_ptr, struct ladish_graph_port, siblings_client);

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
#endif

nomem_close_port_struct:
  dbus_message_iter_close_container(&ports_array_iter, &port_struct_iter);

nomem_close_ports_array:
  dbus_message_iter_close_container(&client_struct_iter, &ports_array_iter);

nomem_close_client_struct:
  dbus_message_iter_close_container(&clients_array_iter, &client_struct_iter);

nomem_close_clients_array:
  dbus_message_iter_close_container(&iter, &clients_array_iter);

nomem:
  dbus_message_unref(call_ptr->reply);
  call_ptr->reply = NULL;
  log_error("Ran out of memory trying to construct method return");

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

#undef graph_ptr

bool ladish_graph_create(ladish_graph_handle * graph_handle_ptr, const char * opath)
{
  struct ladish_graph * graph_ptr;

  graph_ptr = malloc(sizeof(struct ladish_graph));
  if (graph_ptr == NULL)
  {
    log_error("malloc() failed to allocate struct graph_implementator");
    return false;
  }

  if (opath != NULL)
  {
    graph_ptr->opath = strdup(opath);
    if (graph_ptr->opath == NULL)
    {
      log_error("strdup() failed for graph opath");
      free(graph_ptr);
      return false;
    }
  }
  else
  {
    graph_ptr->opath = NULL;
  }

  if (!ladish_dict_create(&graph_ptr->dict))
  {
    log_error("ladish_dict_create() failed for graph");
    if (graph_ptr->opath != NULL)
    {
      free(graph_ptr->opath);
    }
    free(graph_ptr);
    return false;
  }

  INIT_LIST_HEAD(&graph_ptr->clients);
  INIT_LIST_HEAD(&graph_ptr->ports);

  graph_ptr->graph_version = 1;
  graph_ptr->next_client_id = 1;
  graph_ptr->next_port_id = 1;
  graph_ptr->next_connection_id = 1;

  *graph_handle_ptr = (ladish_graph_handle)graph_ptr;
  return true;
}

static
struct ladish_graph_client *
ladish_graph_find_client(
  struct ladish_graph * graph_ptr,
  ladish_client_handle client)
{
  struct list_head * node_ptr;
  struct ladish_graph_client * client_ptr;

  list_for_each(node_ptr, &graph_ptr->clients)
  {
    client_ptr = list_entry(node_ptr, struct ladish_graph_client, siblings);
    if (client_ptr->client == client)
    {
      return client_ptr;
    }
  }

  return NULL;
}

void
ladish_graph_remove_port_internal(
  struct ladish_graph * graph_ptr,
  struct ladish_graph_client * client_ptr,
  struct ladish_graph_port * port_ptr,
  bool destroy)
{
  if (destroy)
  {
    ladish_port_destroy(port_ptr->port);
  }

  list_del(&port_ptr->siblings_client);
  list_del(&port_ptr->siblings_graph);

  log_info("removing port '%s':'%s' (%"PRIu64":%"PRIu64") from graph %s", client_ptr->name, port_ptr->name, client_ptr->id, port_ptr->id, graph_ptr->opath != NULL ? graph_ptr->opath : "JACK");
  if (graph_ptr->opath != NULL)
  {
    dbus_signal_emit(
      g_dbus_connection,
      graph_ptr->opath,
      JACKDBUS_IFACE_PATCHBAY,
      "PortDisappeared",
      "ttsts",
      &graph_ptr->graph_version,
      &client_ptr->id,
      &client_ptr->name,
      &port_ptr->id,
      &port_ptr->name);
  }

  free(port_ptr->name);
  free(port_ptr);
}

void
ladish_graph_remove_client_internal(
  struct ladish_graph * graph_ptr,
  struct ladish_graph_client * client_ptr,
  bool destroy_ports)
{
  struct ladish_graph_port * port_ptr;

  while (!list_empty(&client_ptr->ports))
  {
    port_ptr = list_entry(client_ptr->ports.next, struct ladish_graph_port, siblings_client);
    ladish_graph_remove_port_internal(graph_ptr, client_ptr, port_ptr, destroy_ports);
  }

  graph_ptr->graph_version++;
  list_del(&client_ptr->siblings);
  log_info("removing client '%s' (%"PRIu64") from graph %s", client_ptr->name, client_ptr->id, graph_ptr->opath != NULL ? graph_ptr->opath : "JACK");
  if (graph_ptr->opath != NULL)
  {
    dbus_signal_emit(
      g_dbus_connection,
      graph_ptr->opath,
      JACKDBUS_IFACE_PATCHBAY,
      "ClientDisappeared",
      "tts",
      &graph_ptr->graph_version,
      &client_ptr->id,
      &client_ptr->name);
  }

  free(client_ptr->name);
  free(client_ptr);
}

#define graph_ptr ((struct ladish_graph *)graph_handle)

void ladish_graph_destroy(ladish_graph_handle graph_handle)
{
  ladish_graph_clear(graph_handle);
  ladish_dict_destroy(graph_ptr->dict);
  if (graph_ptr->opath != NULL)
  {
    free(graph_ptr->opath);
  }
  free(graph_ptr);
}

void ladish_graph_clear(ladish_graph_handle graph_handle)
{
  struct ladish_graph_client * client_ptr;

  log_info("ladish_graph_clear() called.");

  while (!list_empty(&graph_ptr->clients))
  {
    client_ptr = list_entry(graph_ptr->clients.next, struct ladish_graph_client, siblings);
    ladish_graph_remove_client_internal(graph_ptr, client_ptr, true);
  }
}

void * ladish_graph_get_dbus_context(ladish_graph_handle graph_handle)
{
  return graph_handle;
}

ladish_dict_handle ladish_graph_get_dict(ladish_graph_handle graph_handle)
{
  return graph_ptr->dict;
}

bool ladish_graph_add_client(ladish_graph_handle graph_handle, ladish_client_handle client_handle, const char * name)
{
  struct ladish_graph_client * client_ptr;

  log_info("adding client '%s' (%p) to graph %s", name, client_handle, graph_ptr->opath != NULL ? graph_ptr->opath : "JACK");

  client_ptr = malloc(sizeof(struct ladish_graph_client));
  if (client_ptr == NULL)
  {
    log_error("malloc() failed for struct ladish_graph_client");
    return false;
  }

  client_ptr->name = strdup(name);
  if (client_ptr->name == NULL)
  {
    log_error("strdup() failed for graph client name");
    free(client_ptr);
    return false;
  }

  client_ptr->id = graph_ptr->next_client_id++;
  client_ptr->client = client_handle;
  graph_ptr->graph_version++;

  INIT_LIST_HEAD(&client_ptr->ports);

  list_add_tail(&client_ptr->siblings, &graph_ptr->clients);

  if (graph_ptr->opath != NULL)
  {
    dbus_signal_emit(
      g_dbus_connection,
      graph_ptr->opath,
      JACKDBUS_IFACE_PATCHBAY,
      "ClientAppeared",
      "tts",
      &graph_ptr->graph_version,
      &client_ptr->id,
      &client_ptr->name);
  }

  return true;
}

void
ladish_graph_remove_client(
  ladish_graph_handle graph_handle,
  ladish_client_handle client_handle,
  bool destroy_ports)
{
  struct ladish_graph_client * client_ptr;

  log_info("ladish_graph_remove_client() called.");

  client_ptr = ladish_graph_find_client(graph_ptr, client_handle);
  if (client_ptr != NULL)
  {
    ladish_graph_remove_client_internal(graph_ptr, client_ptr, destroy_ports);
  }
  else
  {
    assert(false);
  }
}

bool
ladish_graph_add_port(
  ladish_graph_handle graph_handle,
  ladish_client_handle client_handle,
  ladish_port_handle port_handle,
  const char * name,
  uint32_t type,
  uint32_t flags)
{
  struct ladish_graph_client * client_ptr;
  struct ladish_graph_port * port_ptr;

  client_ptr = ladish_graph_find_client(graph_ptr, client_handle);
  if (client_ptr == NULL)
  {
    log_error("cannot find client to add port to");
    assert(false);
    return false;
  }

  log_info("adding port '%s':'%s' (%p) to graph %s", client_ptr->name, name, port_handle, graph_ptr->opath != NULL ? graph_ptr->opath : "JACK");

  port_ptr = malloc(sizeof(struct ladish_graph_port));
  if (port_ptr == NULL)
  {
    log_error("malloc() failed for struct ladish_graph_port");
    return false;
  }

  port_ptr->name = strdup(name);
  if (port_ptr->name == NULL)
  {
    log_error("strdup() failed for graph port name");
    free(port_ptr);
    return false;
  }

  port_ptr->type = type;
  port_ptr->flags = flags;

  port_ptr->id = graph_ptr->next_port_id++;
  port_ptr->port = port_handle;
  graph_ptr->graph_version++;

  port_ptr->client_ptr = client_ptr;
  list_add_tail(&port_ptr->siblings_client, &client_ptr->ports);
  list_add_tail(&port_ptr->siblings_graph, &graph_ptr->ports);

  if (graph_ptr->opath != NULL)
  {
    dbus_signal_emit(
      g_dbus_connection,
      graph_ptr->opath,
      JACKDBUS_IFACE_PATCHBAY,
      "PortAppeared",
      "ttstsuu",
      &graph_ptr->graph_version,
      &client_ptr->id,
      &client_ptr->name,
      &port_ptr->id,
      &port_ptr->name,
      &flags,
      &type);
  }

  return true;
}

ladish_client_handle ladish_graph_find_client_by_id(ladish_graph_handle graph_handle, uint64_t client_id)
{
  struct list_head * node_ptr;
  struct ladish_graph_client * client_ptr;

  list_for_each(node_ptr, &graph_ptr->clients)
  {
    client_ptr = list_entry(node_ptr, struct ladish_graph_client, siblings);
    if (client_ptr->id == client_id)
    {
      return client_ptr->client;
    }
  }

  return NULL;
}

ladish_port_handle ladish_graph_find_port_by_id(ladish_graph_handle graph_handle, uint64_t port_id)
{
  struct list_head * node_ptr;
  struct ladish_graph_port * port_ptr;

  list_for_each(node_ptr, &graph_ptr->ports)
  {
    port_ptr = list_entry(node_ptr, struct ladish_graph_port, siblings_graph);
    if (port_ptr->id == port_id)
    {
      return port_ptr->port;
    }
  }

  return NULL;
}

ladish_client_handle ladish_graph_find_client_by_jack_id(ladish_graph_handle graph_handle, uint64_t client_id)
{
  struct list_head * node_ptr;
  struct ladish_graph_client * client_ptr;

  list_for_each(node_ptr, &graph_ptr->clients)
  {
    client_ptr = list_entry(node_ptr, struct ladish_graph_client, siblings);
    if (ladish_client_get_jack_id(client_ptr->client) == client_id)
    {
      return client_ptr->client;
    }
  }

  return NULL;
}

ladish_port_handle ladish_graph_find_port_by_jack_id(ladish_graph_handle graph_handle, uint64_t port_id)
{
  struct list_head * node_ptr;
  struct ladish_graph_port * port_ptr;

  list_for_each(node_ptr, &graph_ptr->ports)
  {
    port_ptr = list_entry(node_ptr, struct ladish_graph_port, siblings_graph);
    if (ladish_port_get_jack_id(port_ptr->port) == port_id)
    {
      return port_ptr->port;
    }
  }

  return NULL;
}

ladish_client_handle
ladish_graph_remove_port(
  ladish_graph_handle graph_handle,
  ladish_port_handle port)
{
  struct list_head * node_ptr;
  struct ladish_graph_port * port_ptr;

  list_for_each(node_ptr, &graph_ptr->ports)
  {
    port_ptr = list_entry(node_ptr, struct ladish_graph_port, siblings_graph);
    if (port_ptr->port == port)
    {
      ladish_graph_remove_port_internal(graph_ptr, port_ptr->client_ptr, port_ptr, false);
      return port_ptr->client_ptr->client;
    }
  }

  return NULL;
}

const char * ladish_graph_get_client_name(ladish_graph_handle graph_handle, ladish_client_handle client_handle)
{
  struct ladish_graph_client * client_ptr;

  client_ptr = ladish_graph_find_client(graph_ptr, client_handle);
  if (client_ptr != NULL)
  {
      return client_ptr->name;
  }

  assert(false);
  return NULL;
}

bool ladish_graph_is_client_empty(ladish_graph_handle graph_handle, ladish_client_handle client_handle)
{
  struct ladish_graph_client * client_ptr;

  client_ptr = ladish_graph_find_client(graph_ptr, client_handle);
  if (client_ptr != NULL)
  {
    return list_empty(&client_ptr->ports);
  }

  assert(false);
  return true;
}

bool
ladish_graph_iterate_nodes(
  ladish_graph_handle graph_handle,
  void * callback_context,
  bool
  (* client_callback)(
    void * context,
    ladish_client_handle client_handle,
    const char * client_name,
    void ** client_iteration_context_ptr_ptr),
  bool
  (* port_callback)(
    void * context,
    void * client_iteration_context_ptr,
    ladish_client_handle client_handle,
    const char * client_name,
    ladish_port_handle port_handle,
    const char * port_name,
    uint32_t port_type,
    uint32_t port_flags))
{
  struct list_head * client_node_ptr;
  struct ladish_graph_client * client_ptr;
  void * client_context;
  struct list_head * port_node_ptr;
  struct ladish_graph_port * port_ptr;

  list_for_each(client_node_ptr, &graph_ptr->clients)
  {
    client_ptr = list_entry(client_node_ptr, struct ladish_graph_client, siblings);
    if (!client_callback(callback_context, client_ptr->client, client_ptr->name, &client_context))
    {
      return false;
    }

    list_for_each(port_node_ptr, &client_ptr->ports)
    {
      port_ptr = list_entry(port_node_ptr, struct ladish_graph_port, siblings_client);

      if (!port_callback(
            callback_context,
            client_context,
            client_ptr->client,
            client_ptr->name,
            port_ptr->port,
            port_ptr->name,
            port_ptr->type,
            port_ptr->flags))
      {
        return false;
      }
    }
  }

  return true;
}

#undef graph_ptr

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

INTERFACE_BEGIN(g_interface_patchbay, JACKDBUS_IFACE_PATCHBAY)
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
  INTERFACE_EXPOSE_SIGNALS
INTERFACE_END
