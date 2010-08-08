/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
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
  bool hidden;
  bool link;
  uuid_t link_uuid_override;
};

struct ladish_graph_client
{
  struct list_head siblings;
  char * name;
  uint64_t id;
  ladish_client_handle client;
  struct list_head ports;
  bool hidden;
};

struct ladish_graph_connection
{
  struct list_head siblings;
  uint64_t id;
  bool hidden;
  struct ladish_graph_port * port1_ptr;
  struct ladish_graph_port * port2_ptr;
  ladish_dict_handle dict;
  bool changing;
};

struct ladish_graph
{
  char * opath;
  ladish_dict_handle dict;
  struct list_head clients;
  struct list_head ports;
  struct list_head connections;
  uint64_t graph_version;
  uint64_t next_client_id;
  uint64_t next_port_id;
  uint64_t next_connection_id;
  bool persist;

  void * context;
  ladish_graph_connect_request_handler connect_handler;
  ladish_graph_disconnect_request_handler disconnect_handler;
};

static struct ladish_graph_port * ladish_graph_find_port_by_id_internal(struct ladish_graph * graph_ptr, uint64_t port_id)
{
  struct list_head * node_ptr;
  struct ladish_graph_port * port_ptr;

  list_for_each(node_ptr, &graph_ptr->ports)
  {
    port_ptr = list_entry(node_ptr, struct ladish_graph_port, siblings_graph);
    if (port_ptr->id == port_id)
    {
      return port_ptr;
    }
  }

  return NULL;
}

static struct ladish_graph_port *
ladish_graph_find_port_by_uuid_internal(
  struct ladish_graph * graph_ptr,
  const uuid_t uuid,
  bool use_link_override_uuids)
{
  struct list_head * node_ptr;
  struct ladish_graph_port * port_ptr;
  uuid_t current_uuid;
  /* char uuid1_str[37]; */
  /* char uuid2_str[37]; */

  /* log_info("searching by uuid for port in graph %s", ladish_graph_get_description((ladish_graph_handle)graph_ptr)); */
  /* uuid_unparse(uuid, uuid1_str); */

  list_for_each(node_ptr, &graph_ptr->ports)
  {
    port_ptr = list_entry(node_ptr, struct ladish_graph_port, siblings_graph);

    /* if (port_ptr->link) */
    /* { */
    /*   uuid_unparse(port_ptr->link_uuid_override, uuid2_str); */
    /*   log_info("comparing link uuid %s with %s", uuid2_str, uuid1_str); */
    /* } */

    if (use_link_override_uuids && port_ptr->link && uuid_compare(port_ptr->link_uuid_override, uuid) == 0)
    {
      /* log_info("found port %p of client '%s'", port_ptr->port, port_ptr->client_ptr->name); */
      return port_ptr;
    }

    ladish_port_get_uuid(port_ptr->port, current_uuid);
    /* uuid_unparse(current_uuid, uuid2_str); */
    /* log_info("comparing port uuid %s with %s", uuid2_str, uuid1_str); */
    if (uuid_compare(current_uuid, uuid) == 0)
    {
      return port_ptr;
    }
  }

  return NULL;
}

static struct ladish_graph_connection * ladish_graph_find_connection_by_id(struct ladish_graph * graph_ptr, uint64_t connection_id)
{
  struct list_head * node_ptr;
  struct ladish_graph_connection * connection_ptr;

  list_for_each(node_ptr, &graph_ptr->connections)
  {
    connection_ptr = list_entry(node_ptr, struct ladish_graph_connection, siblings);
    if (connection_ptr->id == connection_id)
    {
      return connection_ptr;
    }
  }

  return NULL;
}

static
struct ladish_graph_connection *
ladish_graph_find_connection_by_ports(
  struct ladish_graph * graph_ptr,
  struct ladish_graph_port * port1_ptr,
  struct ladish_graph_port * port2_ptr)
{
  struct list_head * node_ptr;
  struct ladish_graph_connection * connection_ptr;

  list_for_each(node_ptr, &graph_ptr->connections)
  {
    connection_ptr = list_entry(node_ptr, struct ladish_graph_connection, siblings);
    if ((connection_ptr->port1_ptr == port1_ptr && connection_ptr->port2_ptr == port2_ptr) ||
        (connection_ptr->port1_ptr == port2_ptr && connection_ptr->port2_ptr == port1_ptr))
    {
      return connection_ptr;
    }
  }

  return NULL;
}

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
  struct list_head * connection_node_ptr;
  struct ladish_graph_connection * connection_ptr;
  DBusMessageIter connection_struct_iter;

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

      if (client_ptr->hidden)
      {
        continue;
      }

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

        if (port_ptr->hidden)
        {
          continue;
        }

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
    list_for_each(connection_node_ptr, &graph_ptr->connections)
    {
      connection_ptr = list_entry(connection_node_ptr, struct ladish_graph_connection, siblings);

      if (connection_ptr->hidden)
      {
        continue;
      }

      if (!dbus_message_iter_open_container(&connections_array_iter, DBUS_TYPE_STRUCT, NULL, &connection_struct_iter))
      {
        goto nomem_close_connections_array;
      }

      if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_UINT64, &connection_ptr->port1_ptr->client_ptr->id))
      {
        goto nomem_close_connection_struct;
      }

      if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_STRING, &connection_ptr->port1_ptr->client_ptr->name))
      {
        goto nomem_close_connection_struct;
      }

      if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_UINT64, &connection_ptr->port1_ptr->id))
      {
        goto nomem_close_connection_struct;
      }

      if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_STRING, &connection_ptr->port1_ptr->name))
      {
        goto nomem_close_connection_struct;
      }

      if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_UINT64, &connection_ptr->port2_ptr->client_ptr->id))
      {
        goto nomem_close_connection_struct;
      }

      if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_STRING, &connection_ptr->port2_ptr->client_ptr->name))
      {
        goto nomem_close_connection_struct;
      }

      if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_UINT64, &connection_ptr->port2_ptr->id))
      {
        goto nomem_close_connection_struct;
      }

      if (!dbus_message_iter_append_basic(&connection_struct_iter, DBUS_TYPE_STRING, &connection_ptr->port2_ptr->name))
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
  }

  if (!dbus_message_iter_close_container(&iter, &connections_array_iter))
  {
    goto nomem;
  }

  return;

nomem_close_connection_struct:
  dbus_message_iter_close_container(&connections_array_iter, &connection_struct_iter);

nomem_close_connections_array:
  dbus_message_iter_close_container(&iter, &connections_array_iter);
  goto nomem;

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
  log_info("connect_ports_by_name() called.");
  lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "connect by name is not implemented yet");
}

static void connect_ports_by_id(struct dbus_method_call * call_ptr)
{
  dbus_uint64_t port1_id;
  dbus_uint64_t port2_id;
  struct ladish_graph_port * port1_ptr;
  struct ladish_graph_port * port2_ptr;

  if (!dbus_message_get_args(call_ptr->message, &g_dbus_error, DBUS_TYPE_UINT64, &port1_id, DBUS_TYPE_UINT64, &port2_id, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s", call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  log_info("connect_ports_by_id(%"PRIu64",%"PRIu64") called.", port1_id, port2_id);

  if (graph_ptr->connect_handler == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "connect requests on graph %s cannot be handlined", graph_ptr->opath != NULL ? graph_ptr->opath : "JACK");
    return;
  }

  port1_ptr = ladish_graph_find_port_by_id_internal(graph_ptr, port1_id);
  if (port1_ptr == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Cannot connect unknown port with id %"PRIu64, port1_id);
    return;
  }

  port2_ptr = ladish_graph_find_port_by_id_internal(graph_ptr, port2_id);
  if (port2_ptr == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Cannot connect unknown port with id %"PRIu64, port2_id);
    return;
  }

  log_info("connecting '%s':'%s' to '%s':'%s'", port1_ptr->client_ptr->name, port1_ptr->name, port2_ptr->client_ptr->name, port2_ptr->name);

  if (graph_ptr->connect_handler(graph_ptr->context, (ladish_graph_handle)graph_ptr, port1_ptr->port, port2_ptr->port))
  {
    method_return_new_void(call_ptr);
  }
  else
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "connect failed");
  }
}

static void disconnect_ports(struct dbus_method_call * call_ptr, struct ladish_graph_connection * connection_ptr)
{
  log_info(
    "disconnecting '%s':'%s' from '%s':'%s'",
    connection_ptr->port1_ptr->client_ptr->name,
    connection_ptr->port1_ptr->name,
    connection_ptr->port2_ptr->client_ptr->name,
    connection_ptr->port2_ptr->name);

  connection_ptr->changing = true;
  if (graph_ptr->disconnect_handler(graph_ptr->context, (ladish_graph_handle)graph_ptr, connection_ptr->id))
  {
    method_return_new_void(call_ptr);
  }
  else
  {
    connection_ptr->changing = false;
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "disconnect failed");
  }
}

static void disconnect_ports_by_name(struct dbus_method_call * call_ptr)
{
  log_info("disconnect_ports_by_name() called.");
  lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "disconnect by name is not implemented yet");
}

static void disconnect_ports_by_id(struct dbus_method_call * call_ptr)
{
  dbus_uint64_t port1_id;
  dbus_uint64_t port2_id;
  struct ladish_graph_port * port1_ptr;
  struct ladish_graph_port * port2_ptr;
  struct ladish_graph_connection * connection_ptr;

  if (!dbus_message_get_args(call_ptr->message, &g_dbus_error, DBUS_TYPE_UINT64, &port1_id, DBUS_TYPE_UINT64, &port2_id, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s", call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  log_info("disconnect_ports_by_id(%"PRIu64",%"PRIu64") called.", port1_id, port2_id);

  if (graph_ptr->disconnect_handler == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "disconnect requests on graph %s cannot be handlined", graph_ptr->opath != NULL ? graph_ptr->opath : "JACK");
    return;
  }

  port1_ptr = ladish_graph_find_port_by_id_internal(graph_ptr, port1_id);
  if (port1_ptr == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Cannot disconnect unknown port with id %"PRIu64, port1_id);
    return;
  }

  port2_ptr = ladish_graph_find_port_by_id_internal(graph_ptr, port2_id);
  if (port2_ptr == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Cannot disconnect unknown port with id %"PRIu64, port2_id);
    return;
  }

  connection_ptr = ladish_graph_find_connection_by_ports(graph_ptr, port1_ptr, port2_ptr);
  if (connection_ptr == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Cannot disconnect not connected ports %"PRIu64" and %"PRIu64, port1_id, port2_id);
    return;
  }

  disconnect_ports(call_ptr, connection_ptr);
}

static void disconnect_ports_by_connection_id(struct dbus_method_call * call_ptr)
{
  dbus_uint64_t connection_id;
  struct ladish_graph_connection * connection_ptr;

  if (!dbus_message_get_args(call_ptr->message, &g_dbus_error, DBUS_TYPE_UINT64, &connection_id, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s", call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  log_info("disconnect_ports_by_connection_id(%"PRIu64") called.", connection_id);

  connection_ptr = ladish_graph_find_connection_by_id(graph_ptr, connection_id);
  if (connection_ptr == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Cannot find connection with id %"PRIu64, connection_id);
    return;
  }

  disconnect_ports(call_ptr, connection_ptr);
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
  INIT_LIST_HEAD(&graph_ptr->connections);

  graph_ptr->graph_version = 1;
  graph_ptr->next_client_id = 1;
  graph_ptr->next_port_id = 1;
  graph_ptr->next_connection_id = 1;

  graph_ptr->context = NULL;
  graph_ptr->connect_handler = NULL;
  graph_ptr->disconnect_handler = NULL;

  graph_ptr->persist = true;

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

static
struct ladish_graph_port *
ladish_graph_find_port(
  struct ladish_graph * graph_ptr,
  ladish_port_handle port)
{
  struct list_head * node_ptr;
  struct ladish_graph_port * port_ptr;

  //log_info("searching port %p", port);

  list_for_each(node_ptr, &graph_ptr->ports)
  {
    port_ptr = list_entry(node_ptr, struct ladish_graph_port, siblings_graph);
    //log_info("checking port %s:%s, %p", port_ptr->client_ptr->name, port_ptr->name, port_ptr->port);
    if (port_ptr->port == port)
    {
      return port_ptr;
    }
  }

  return NULL;
}

static
struct ladish_graph_port *
ladish_graph_find_port_by_jack_id_internal(
  struct ladish_graph * graph_ptr,
  uint64_t port_id,
  bool room,
  bool studio)
{
  struct list_head * node_ptr;
  struct ladish_graph_port * port_ptr;

  ASSERT(room || studio);

  list_for_each(node_ptr, &graph_ptr->ports)
  {
    port_ptr = list_entry(node_ptr, struct ladish_graph_port, siblings_graph);
    //log_info("checking jack port id of port %s:%s, %p", port_ptr->client_ptr->name, port_ptr->name, port_ptr->port);
    if ((studio && ladish_port_get_jack_id(port_ptr->port) == port_id) ||
        (room && port_ptr->link && ladish_port_get_jack_id_room(port_ptr->port) == port_id))
    {
      //log_info("found");
      return port_ptr;
    }
  }

  return NULL;
}

#if 0
static
struct ladish_graph_port *
ladish_graph_find_client_port(
  struct ladish_graph * graph_ptr,
  struct ladish_graph_client * client_ptr,
  ladish_port_handle port)
{
  struct list_head * node_ptr;
  struct ladish_graph_port * port_ptr;

  list_for_each(node_ptr, &client_ptr->ports)
  {
    port_ptr = list_entry(node_ptr, struct ladish_graph_port, siblings_client);
    if (port_ptr->port == port)
    {
      return port_ptr;
    }
  }

  return NULL;
}
#endif

static void ladish_graph_hide_connection_internal(struct ladish_graph * graph_ptr, struct ladish_graph_connection * connection_ptr)
{
  ASSERT(!connection_ptr->hidden);
  connection_ptr->hidden = true;
  graph_ptr->graph_version++;

  if (graph_ptr->opath != NULL)
  {
    dbus_signal_emit(
      g_dbus_connection,
      graph_ptr->opath,
      JACKDBUS_IFACE_PATCHBAY,
      "PortsDisconnected",
      "ttstststst",
      &graph_ptr->graph_version,
      &connection_ptr->port1_ptr->client_ptr->id,
      &connection_ptr->port1_ptr->client_ptr->name,
      &connection_ptr->port1_ptr->id,
      &connection_ptr->port1_ptr->name,
      &connection_ptr->port2_ptr->client_ptr->id,
      &connection_ptr->port2_ptr->client_ptr->name,
      &connection_ptr->port2_ptr->id,
      &connection_ptr->port2_ptr->name,
      &connection_ptr->id);
  }
}

static void ladish_graph_show_port_internal(struct ladish_graph * graph_ptr, struct ladish_graph_port * port_ptr)
{
  if (port_ptr->client_ptr->hidden)
  {
    port_ptr->client_ptr->hidden = false;
    graph_ptr->graph_version++;
    if (graph_ptr->opath != NULL)
    {
      dbus_signal_emit(
        g_dbus_connection,
        graph_ptr->opath,
        JACKDBUS_IFACE_PATCHBAY,
        "ClientAppeared",
        "tts",
        &graph_ptr->graph_version,
        &port_ptr->client_ptr->id,
        &port_ptr->client_ptr->name);
    }
  }

  ASSERT(port_ptr->hidden);
  port_ptr->hidden = false;
  graph_ptr->graph_version++;
  if (graph_ptr->opath != NULL)
  {
    dbus_signal_emit(
      g_dbus_connection,
      graph_ptr->opath,
      JACKDBUS_IFACE_PATCHBAY,
      "PortAppeared",
      "ttstsuu",
      &graph_ptr->graph_version,
      &port_ptr->client_ptr->id,
      &port_ptr->client_ptr->name,
      &port_ptr->id,
      &port_ptr->name,
      &port_ptr->flags,
      &port_ptr->type);

    ladish_try_connect_hidden_connections((ladish_graph_handle)graph_ptr);
  }
}

static void ladish_graph_hide_port_internal(struct ladish_graph * graph_ptr, struct ladish_graph_port * port_ptr)
{
  ASSERT(!port_ptr->hidden);
  port_ptr->hidden = true;
  graph_ptr->graph_version++;

  if (graph_ptr->opath != NULL)
  {
    dbus_signal_emit(
      g_dbus_connection,
      graph_ptr->opath,
      JACKDBUS_IFACE_PATCHBAY,
      "PortDisappeared",
      "ttsts",
      &graph_ptr->graph_version,
      &port_ptr->client_ptr->id,
      &port_ptr->client_ptr->name,
      &port_ptr->id,
      &port_ptr->name);
  }
}

static void ladish_graph_hide_client_internal(struct ladish_graph * graph_ptr, struct ladish_graph_client * client_ptr)
{
  ASSERT(!client_ptr->hidden);
  client_ptr->hidden = true;
  graph_ptr->graph_version++;

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
}

static void ladish_hide_connections(struct ladish_graph * graph_ptr, struct ladish_graph_port * port_ptr)
{
  struct list_head * node_ptr;
  struct ladish_graph_connection * connection_ptr;

  log_info("hidding connections of port %"PRIu64, port_ptr->id);

  ASSERT(graph_ptr->opath != NULL);

  list_for_each(node_ptr, &graph_ptr->connections)
  {
    connection_ptr = list_entry(node_ptr, struct ladish_graph_connection, siblings);
    if (!connection_ptr->hidden && (connection_ptr->port1_ptr == port_ptr || connection_ptr->port2_ptr == port_ptr))
    {
      log_info("hidding connection between ports %"PRIu64" and %"PRIu64, connection_ptr->port1_ptr->id, connection_ptr->port2_ptr->id);
      ladish_graph_hide_connection_internal(graph_ptr, connection_ptr);
    }
  }
}

static void ladish_graph_remove_connection_internal(struct ladish_graph * graph_ptr, struct ladish_graph_connection * connection_ptr)
{
  list_del(&connection_ptr->siblings);
  graph_ptr->graph_version++;

  if (!connection_ptr->hidden && graph_ptr->opath != NULL)
  {
    dbus_signal_emit(
      g_dbus_connection,
      graph_ptr->opath,
      JACKDBUS_IFACE_PATCHBAY,
      "PortsDisconnected",
      "ttstststst",
      &graph_ptr->graph_version,
      &connection_ptr->port1_ptr->client_ptr->id,
      &connection_ptr->port1_ptr->client_ptr->name,
      &connection_ptr->port1_ptr->id,
      &connection_ptr->port1_ptr->name,
      &connection_ptr->port2_ptr->client_ptr->id,
      &connection_ptr->port2_ptr->client_ptr->name,
      &connection_ptr->port2_ptr->id,
      &connection_ptr->port2_ptr->name,
      &connection_ptr->id);
  }

  ladish_dict_destroy(connection_ptr->dict);
  free(connection_ptr);
}

static void ladish_graph_remove_port_connections(struct ladish_graph * graph_ptr, struct ladish_graph_port * port_ptr)
{
  struct list_head * node_ptr;
  struct list_head * temp_node_ptr;
  struct ladish_graph_connection * connection_ptr;

  list_for_each_safe(node_ptr, temp_node_ptr, &graph_ptr->connections)
  {
    connection_ptr = list_entry(node_ptr, struct ladish_graph_connection, siblings);
    if (connection_ptr->port1_ptr == port_ptr || connection_ptr->port2_ptr == port_ptr)
    {
      log_info("removing connection between ports %"PRIu64" and %"PRIu64, connection_ptr->port1_ptr->id, connection_ptr->port2_ptr->id);
      ladish_graph_remove_connection_internal(graph_ptr, connection_ptr);
    }
  }
}

static
void
ladish_graph_remove_port_internal(
  struct ladish_graph * graph_ptr,
  struct ladish_graph_client * client_ptr,
  struct ladish_graph_port * port_ptr)
{
  ladish_graph_remove_port_connections(graph_ptr, port_ptr);

  ladish_port_del_ref(port_ptr->port);

  list_del(&port_ptr->siblings_client);
  list_del(&port_ptr->siblings_graph);

  log_info("removing port '%s':'%s' (%"PRIu64":%"PRIu64") from graph %s", client_ptr->name, port_ptr->name, client_ptr->id, port_ptr->id, graph_ptr->opath != NULL ? graph_ptr->opath : "JACK");
  if (graph_ptr->opath != NULL && !port_ptr->hidden)
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

static
void
ladish_graph_remove_client_internal(
  struct ladish_graph * graph_ptr,
  struct ladish_graph_client * client_ptr,
  bool destroy_client,
  ladish_graph_simple_port_callback port_callback)
{
  struct ladish_graph_port * port_ptr;

  while (!list_empty(&client_ptr->ports))
  {
    port_ptr = list_entry(client_ptr->ports.next, struct ladish_graph_port, siblings_client);
    if (port_callback != NULL)
    {
      port_callback(port_ptr->port);
    }
    ladish_graph_remove_port_internal(graph_ptr, client_ptr, port_ptr);
  }

  graph_ptr->graph_version++;
  list_del(&client_ptr->siblings);
  log_info("removing client '%s' (%"PRIu64") from graph %s", client_ptr->name, client_ptr->id, graph_ptr->opath != NULL ? graph_ptr->opath : "JACK");
  if (graph_ptr->opath != NULL && !client_ptr->hidden)
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

  if (destroy_client)
  {
    ladish_client_destroy(client_ptr->client);
  }

  free(client_ptr);
}

bool ladish_graph_client_looks_empty_internal(struct ladish_graph * graph_ptr, struct ladish_graph_client * client_ptr)
{
  struct list_head * node_ptr;
  struct ladish_graph_port * port_ptr;

  list_for_each(node_ptr, &client_ptr->ports)
  {
    port_ptr = list_entry(node_ptr, struct ladish_graph_port, siblings_client);
    if (!port_ptr->hidden)
    {
      //log_info("port '%s' is visible, client '%s' does not look empty", port_ptr->name, client_ptr->name);
      return false;
    }
    else
    {
      //log_info("port '%s' is invisible", port_ptr->name);
    }
  }

  //log_info("client '%s' looks empty in graph %s", client_ptr->name, graph_ptr->opath != NULL ? graph_ptr->opath : "JACK");
  return true;
}

#define graph_ptr ((struct ladish_graph *)graph_handle)

void ladish_graph_destroy(ladish_graph_handle graph_handle)
{
  ladish_graph_clear(graph_handle, NULL);
  ladish_dict_destroy(graph_ptr->dict);
  if (graph_ptr->opath != NULL)
  {
    free(graph_ptr->opath);
  }
  free(graph_ptr);
}

const char * ladish_graph_get_opath(ladish_graph_handle graph_handle)
{
  return graph_ptr->opath;
}

const char * ladish_graph_get_description(ladish_graph_handle graph_handle)
{
  return graph_ptr->opath != NULL ? graph_ptr->opath : "JACK";
}

void
ladish_graph_set_connection_handlers(
  ladish_graph_handle graph_handle,
  void * graph_context,
  ladish_graph_connect_request_handler connect_handler,
  ladish_graph_disconnect_request_handler disconnect_handler)
{
  log_info("setting connection handlers for graph '%s'", graph_ptr->opath != NULL ? graph_ptr->opath : "JACK");
  graph_ptr->context = graph_context;
  graph_ptr->connect_handler = connect_handler;
  graph_ptr->disconnect_handler = disconnect_handler;
}

void ladish_graph_clear(ladish_graph_handle graph_handle, ladish_graph_simple_port_callback port_callback)
{
  struct ladish_graph_client * client_ptr;
  struct ladish_graph_connection * connection_ptr;

  log_info("ladish_graph_clear() called for graph '%s'", graph_ptr->opath != NULL ? graph_ptr->opath : "JACK");

  while (!list_empty(&graph_ptr->connections))
  {
    connection_ptr = list_entry(graph_ptr->connections.next, struct ladish_graph_connection, siblings);
    ladish_graph_remove_connection_internal(graph_ptr, connection_ptr);
  }

  while (!list_empty(&graph_ptr->clients))
  {
    client_ptr = list_entry(graph_ptr->clients.next, struct ladish_graph_client, siblings);
    ladish_graph_remove_client_internal(graph_ptr, client_ptr, true, port_callback);
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

void ladish_graph_show_connection(ladish_graph_handle graph_handle, uint64_t connection_id)
{
  struct ladish_graph_connection * connection_ptr;

  log_info("ladish_graph_show_connection() called.");

  connection_ptr = ladish_graph_find_connection_by_id(graph_ptr, connection_id);
  if (connection_ptr == NULL)
  {
    ASSERT_NO_PASS;
    return;
  }

  ASSERT(graph_ptr->opath != NULL);
  ASSERT(connection_ptr->hidden);
  connection_ptr->hidden = false;
  connection_ptr->changing = false;
  graph_ptr->graph_version++;

  dbus_signal_emit(
    g_dbus_connection,
    graph_ptr->opath,
    JACKDBUS_IFACE_PATCHBAY,
    "PortsConnected",
    "ttstststst",
    &graph_ptr->graph_version,
    &connection_ptr->port1_ptr->client_ptr->id,
    &connection_ptr->port1_ptr->client_ptr->name,
    &connection_ptr->port1_ptr->id,
    &connection_ptr->port1_ptr->name,
    &connection_ptr->port2_ptr->client_ptr->id,
    &connection_ptr->port2_ptr->client_ptr->name,
    &connection_ptr->port2_ptr->id,
    &connection_ptr->port2_ptr->name,
    &connection_ptr->id);
}

void ladish_graph_show_port(ladish_graph_handle graph_handle, ladish_port_handle port_handle)
{
  struct ladish_graph_port * port_ptr;

  //log_info("ladish_graph_show_port() called.");

  port_ptr = ladish_graph_find_port(graph_ptr, port_handle);
  if (port_ptr == NULL)
  {
    ASSERT_NO_PASS;
    return;
  }

  //log_info("port '%s' is %s", port_ptr->name, port_ptr->hidden ? "invisible" : "visible");

  ladish_graph_show_port_internal(graph_ptr, port_ptr);
}

void ladish_graph_hide_port(ladish_graph_handle graph_handle, ladish_port_handle port_handle)
{
  struct ladish_graph_port * port_ptr;

  log_info("ladish_graph_hide_port() called.");

  port_ptr = ladish_graph_find_port(graph_ptr, port_handle);
  if (port_ptr == NULL)
  {
    ASSERT_NO_PASS;
    return;
  }

  log_info("Hidding port %"PRIu64, port_ptr->id);

  ASSERT(!port_ptr->hidden);

  if (graph_ptr->opath != NULL)
  {
    ladish_hide_connections(graph_ptr, port_ptr);
  }

  ladish_graph_hide_port_internal(graph_ptr, port_ptr);
}

void ladish_graph_hide_client(ladish_graph_handle graph_handle, ladish_client_handle client_handle)
{
  struct ladish_graph_client * client_ptr;

  log_info("ladish_graph_hide_client() called.");

  client_ptr = ladish_graph_find_client(graph_ptr, client_handle);
  if (client_ptr == NULL)
  {
    ASSERT_NO_PASS;
    return;
  }

  ladish_graph_hide_client_internal(graph_ptr, client_ptr);
}

void ladish_graph_show_client(ladish_graph_handle graph_handle, ladish_client_handle client_handle)
{
  struct ladish_graph_client * client_ptr;

  log_info("ladish_graph_show_client() called.");

  client_ptr = ladish_graph_find_client(graph_ptr, client_handle);
  if (client_ptr == NULL)
  {
    ASSERT_NO_PASS;
    return;
  }

  ASSERT(client_ptr->hidden);
  client_ptr->hidden = false;
  graph_ptr->graph_version++;

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
}

void ladish_graph_adjust_port(ladish_graph_handle graph_handle, ladish_port_handle port_handle, uint32_t type, uint32_t flags)
{
  struct ladish_graph_port * port_ptr;

  //log_info("ladish_graph_adjust_port() called.");

  port_ptr = ladish_graph_find_port(graph_ptr, port_handle);
  if (port_ptr == NULL)
  {
    ASSERT_NO_PASS;
    return;
  }

  port_ptr->type = type;
  port_ptr->flags = flags;
}

bool ladish_graph_add_client(ladish_graph_handle graph_handle, ladish_client_handle client_handle, const char * name, bool hidden)
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
  client_ptr->hidden = hidden;
  graph_ptr->graph_version++;

  INIT_LIST_HEAD(&client_ptr->ports);

  list_add_tail(&client_ptr->siblings, &graph_ptr->clients);

  if (!hidden && graph_ptr->opath != NULL)
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
  ladish_client_handle client_handle)
{
  struct ladish_graph_client * client_ptr;

  log_info("ladish_graph_remove_client() called.");

  client_ptr = ladish_graph_find_client(graph_ptr, client_handle);
  if (client_ptr != NULL)
  {
    ladish_graph_remove_client_internal(graph_ptr, client_ptr, false, NULL);
  }
  else
  {
    ASSERT_NO_PASS;
  }
}

bool
ladish_graph_add_port(
  ladish_graph_handle graph_handle,
  ladish_client_handle client_handle,
  ladish_port_handle port_handle,
  const char * name,
  uint32_t type,
  uint32_t flags,
  bool hidden)
{
  struct ladish_graph_client * client_ptr;
  struct ladish_graph_port * port_ptr;

  client_ptr = ladish_graph_find_client(graph_ptr, client_handle);
  if (client_ptr == NULL)
  {
    log_error("cannot find client to add port to. graph is %s", graph_ptr->opath != NULL ? graph_ptr->opath : "JACK");
    ASSERT_NO_PASS;
    return false;
  }

  log_info("adding port '%s' (%p) to client '%s' in graph %s", name, port_handle, client_ptr->name, graph_ptr->opath != NULL ? graph_ptr->opath : "JACK");

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
  ladish_port_add_ref(port_ptr->port);
  port_ptr->hidden = true;

  port_ptr->link = ladish_port_is_link(port_handle);
  if (port_ptr->link)
  {
    uuid_generate(port_ptr->link_uuid_override);
  }

  port_ptr->client_ptr = client_ptr;
  list_add_tail(&port_ptr->siblings_client, &client_ptr->ports);
  list_add_tail(&port_ptr->siblings_graph, &graph_ptr->ports);

  if (!hidden)
  {
    ladish_graph_show_port_internal(graph_ptr, port_ptr);
  }

  return true;
}

uint64_t
ladish_graph_add_connection(
  ladish_graph_handle graph_handle,
  ladish_port_handle port1_handle,
  ladish_port_handle port2_handle,
  bool hidden)
{
  struct ladish_graph_port * port1_ptr;
  struct ladish_graph_port * port2_ptr;
  struct ladish_graph_connection * connection_ptr;

  port1_ptr = ladish_graph_find_port(graph_ptr, port1_handle);
  ASSERT(port1_ptr != NULL);
  port2_ptr = ladish_graph_find_port(graph_ptr, port2_handle);
  ASSERT(port2_ptr != NULL);

  connection_ptr = malloc(sizeof(struct ladish_graph_connection));
  if (connection_ptr == NULL)
  {
    log_error("malloc() failed for struct ladish_graph_connection");
    return 0;
  }

  if (!ladish_dict_create(&connection_ptr->dict))
  {
    log_error("ladish_dict_create() failed for connection");
    free(connection_ptr);
    return 0;
  }

  connection_ptr->id = graph_ptr->next_connection_id++;
  connection_ptr->port1_ptr = port1_ptr;
  connection_ptr->port2_ptr = port2_ptr;
  connection_ptr->hidden = hidden;
  connection_ptr->changing = false;
  graph_ptr->graph_version++;

  list_add_tail(&connection_ptr->siblings, &graph_ptr->connections);

  /* log_info( */
  /*   "new connection %"PRIu64" between '%s':'%s' and '%s':'%s'", */
  /*   connection_ptr->id, */
  /*   port1_ptr->client_ptr->name, */
  /*   port1_ptr->name, */
  /*   port2_ptr->client_ptr->name, */
  /*   port2_ptr->name); */

  if (!hidden && graph_ptr->opath != NULL)
  {
    dbus_signal_emit(
      g_dbus_connection,
      graph_ptr->opath,
      JACKDBUS_IFACE_PATCHBAY,
      "PortsConnected",
      "ttstststst",
      &graph_ptr->graph_version,
      &port1_ptr->client_ptr->id,
      &port1_ptr->client_ptr->name,
      &port1_ptr->id,
      &port1_ptr->name,
      &port2_ptr->client_ptr->id,
      &port2_ptr->client_ptr->name,
      &port2_ptr->id,
      &port2_ptr->name,
      &connection_ptr->id);
  }

  return connection_ptr->id;
}

void
ladish_graph_remove_connection(
  ladish_graph_handle graph_handle,
  uint64_t connection_id,
  bool force)
{
  struct ladish_graph_connection * connection_ptr;

  connection_ptr = ladish_graph_find_connection_by_id(graph_ptr, connection_id);
  if (connection_ptr == NULL)
  {
    ASSERT_NO_PASS;
    return;
  }

  if (force || connection_ptr->changing || !graph_ptr->persist)
  {
    /* log_info( */
    /*   "removing connection '%s':'%s' - '%s':'%s'", */
    /*   connection_ptr->port1_ptr->client_ptr->name, */
    /*   connection_ptr->port1_ptr->name, */
    /*   connection_ptr->port2_ptr->client_ptr->name, */
    /*   connection_ptr->port2_ptr->name); */

    ladish_graph_remove_connection_internal(graph_ptr, connection_ptr);
  }
  else
  {
    /* log_info( */
    /*   "hiding connection '%s':'%s' - '%s':'%s'", */
    /*   connection_ptr->port1_ptr->client_ptr->name, */
    /*   connection_ptr->port1_ptr->name, */
    /*   connection_ptr->port2_ptr->client_ptr->name, */
    /*   connection_ptr->port2_ptr->name); */

    ladish_graph_hide_connection_internal(graph_ptr, connection_ptr);
  }
}

bool
ladish_graph_get_connection_ports(
  ladish_graph_handle graph_handle,
  uint64_t connection_id,
  ladish_port_handle * port1_handle_ptr,
  ladish_port_handle * port2_handle_ptr)
{
  struct ladish_graph_connection * connection_ptr;

  connection_ptr = ladish_graph_find_connection_by_id(graph_ptr, connection_id);
  if (connection_ptr == NULL)
  {
    return false;
  }

  *port1_handle_ptr = connection_ptr->port1_ptr->port;
  *port2_handle_ptr = connection_ptr->port2_ptr->port;

  return true;
}

ladish_dict_handle ladish_graph_get_connection_dict(ladish_graph_handle graph_handle, uint64_t connection_id)
{
  struct ladish_graph_connection * connection_ptr;

  connection_ptr = ladish_graph_find_connection_by_id(graph_ptr, connection_id);
  if (connection_ptr == NULL)
  {
    return NULL;
  }

  return connection_ptr->dict;
}

bool
ladish_graph_find_connection(
  ladish_graph_handle graph_handle,
  ladish_port_handle port1_handle,
  ladish_port_handle port2_handle,
  uint64_t * connection_id_ptr)
{
  struct ladish_graph_port * port1_ptr;
  struct ladish_graph_port * port2_ptr;
  struct ladish_graph_connection * connection_ptr;

  port1_ptr = ladish_graph_find_port(graph_ptr, port1_handle);
  if (port1_ptr == NULL)
  {
    return false;
  }
     
  port2_ptr = ladish_graph_find_port(graph_ptr, port2_handle);
  if (port1_ptr == NULL)
  {
    return false;
  }

  connection_ptr = ladish_graph_find_connection_by_ports(graph_ptr, port1_ptr, port2_ptr);
  if (connection_ptr == NULL)
  {
    return false;
  }

  *connection_id_ptr = connection_ptr->id;

  return true;
}

ladish_client_handle ladish_graph_find_client_by_name(ladish_graph_handle graph_handle, const char * name)
{
  struct list_head * node_ptr;
  struct ladish_graph_client * client_ptr;

  list_for_each(node_ptr, &graph_ptr->clients)
  {
    client_ptr = list_entry(node_ptr, struct ladish_graph_client, siblings);
    if (strcmp(client_ptr->name, name) == 0)
    {
      return client_ptr->client;
    }
  }

  return NULL;
}

ladish_port_handle ladish_graph_find_port_by_name(ladish_graph_handle graph_handle, ladish_client_handle client_handle, const char * name)
{
  struct ladish_graph_client * client_ptr;
  struct list_head * node_ptr;
  struct ladish_graph_port * port_ptr;

  client_ptr = ladish_graph_find_client(graph_ptr, client_handle);
  if (client_ptr != NULL)
  {
    list_for_each(node_ptr, &client_ptr->ports)
    {
      port_ptr = list_entry(node_ptr, struct ladish_graph_port, siblings_client);
      if (strcmp(port_ptr->name, name) == 0)
      {
        return port_ptr->port;
      }
    }
  }
  else
  {
    ASSERT_NO_PASS;
  }

  return NULL;
}

ladish_client_handle ladish_graph_find_client_by_uuid(ladish_graph_handle graph_handle, const uuid_t uuid)
{
  struct list_head * node_ptr;
  struct ladish_graph_client * client_ptr;
  uuid_t current_uuid;

  list_for_each(node_ptr, &graph_ptr->clients)
  {
    client_ptr = list_entry(node_ptr, struct ladish_graph_client, siblings);
    ladish_client_get_uuid(client_ptr->client, current_uuid);
    if (uuid_compare(current_uuid, uuid) == 0)
    {
      return client_ptr->client;
    }
  }

  return NULL;
}

ladish_port_handle ladish_graph_find_port_by_uuid(ladish_graph_handle graph_handle, const uuid_t uuid, bool use_link_override_uuids)
{
  struct ladish_graph_port * port_ptr;

  port_ptr = ladish_graph_find_port_by_uuid_internal(graph_ptr, uuid, use_link_override_uuids);
  if (port_ptr != NULL)
  {
    return port_ptr->port;
  }

  return NULL;
}

ladish_client_handle ladish_graph_get_port_client(ladish_graph_handle graph_handle, ladish_port_handle port_handle)
{
  struct ladish_graph_port * port_ptr;

  port_ptr = ladish_graph_find_port(graph_ptr, port_handle);
  if (port_ptr == NULL)
  {
    return NULL;
  }

  return port_ptr->client_ptr->client;
}

bool ladish_graph_is_port_present(ladish_graph_handle graph_handle, ladish_port_handle port_handle)
{
  return ladish_graph_find_port(graph_ptr, port_handle) != NULL;
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
  struct ladish_graph_port * port_ptr;

  port_ptr = ladish_graph_find_port_by_id_internal(graph_ptr, port_id);
  if (port_ptr == NULL)
  {
    return NULL;
  }

  return port_ptr->port;
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

ladish_port_handle ladish_graph_find_port_by_jack_id(ladish_graph_handle graph_handle, uint64_t port_id, bool room, bool studio)
{
  struct ladish_graph_port * port_ptr;

  port_ptr = ladish_graph_find_port_by_jack_id_internal(graph_ptr, port_id, room, studio);
  if (port_ptr == NULL)
  {
    return NULL;
  }

  return port_ptr->port;
}

ladish_client_handle
ladish_graph_remove_port(
  ladish_graph_handle graph_handle,
  ladish_port_handle port)
{
  struct ladish_graph_port * port_ptr;
  ladish_client_handle client;

  port_ptr = ladish_graph_find_port(graph_ptr, port);
  if (port_ptr == NULL)
  {
    return NULL;
  }

  client = port_ptr->client_ptr->client;
  ladish_graph_remove_port_internal(graph_ptr, port_ptr->client_ptr, port_ptr);
  return client;
}

ladish_client_handle
ladish_graph_remove_port_by_jack_id(
  ladish_graph_handle graph_handle,
  uint64_t jack_port_id,
  bool room,
  bool studio)
{
  struct ladish_graph_port * port_ptr;
  ladish_client_handle client;

  port_ptr = ladish_graph_find_port_by_jack_id_internal(graph_ptr, jack_port_id, room, studio);
  if (port_ptr == NULL)
  {
    return NULL;
  }

  client = port_ptr->client_ptr->client;
  ladish_graph_remove_port_internal(graph_ptr, port_ptr->client_ptr, port_ptr);
  return client;
}

bool
ladish_graph_rename_client(
  ladish_graph_handle graph_handle,
  ladish_client_handle client_handle,
  const char * new_client_name)
{
  char * name;
  struct ladish_graph_client * client_ptr;
  char * old_name;

  name = strdup(new_client_name);
  if (name == NULL)
  {
    log_error("strdup('%s') failed.", new_client_name);
    return false;
  }

  client_ptr = ladish_graph_find_client(graph_ptr, client_handle);
  if (client_ptr == NULL)
  {
    free(name);
    ASSERT_NO_PASS;
    return false;
  }

  old_name = client_ptr->name;
  client_ptr->name = name;

  graph_ptr->graph_version++;

  if (!client_ptr->hidden && graph_ptr->opath != NULL)
  {
    dbus_signal_emit(
      g_dbus_connection,
      graph_ptr->opath,
      JACKDBUS_IFACE_PATCHBAY,
      "ClientRenamed",
      "ttss",
      &graph_ptr->graph_version,
      &client_ptr->id,
      &old_name,
      &client_ptr->name);
  }

  free(old_name);

  return true;
}

bool
ladish_graph_rename_port(
  ladish_graph_handle graph_handle,
  ladish_port_handle port_handle,
  const char * new_port_name)
{
  char * name;
  struct ladish_graph_port * port_ptr;
  char * old_name;

  name = strdup(new_port_name);
  if (name == NULL)
  {
    log_error("strdup('%s') failed.", new_port_name);
    return false;
  }

  port_ptr = ladish_graph_find_port(graph_ptr, port_handle);
  if (port_ptr == NULL)
  {
    ASSERT_NO_PASS;
    free(name);
    return false;
  }

  old_name = port_ptr->name;
  port_ptr->name = name;

  graph_ptr->graph_version++;

  if (!port_ptr->hidden && graph_ptr->opath != NULL)
  {
    dbus_signal_emit(
      g_dbus_connection,
      graph_ptr->opath,
      JACKDBUS_IFACE_PATCHBAY,
      "PortRenamed",
      "ttstss",
      &graph_ptr->graph_version,
      &port_ptr->client_ptr->id,
      &port_ptr->client_ptr->name,
      &port_ptr->id,
      &old_name,
      &port_ptr->name);
  }

  free(old_name);

  return true;
}

const char * ladish_graph_get_client_name(ladish_graph_handle graph_handle, ladish_client_handle client_handle)
{
  struct ladish_graph_client * client_ptr;

  client_ptr = ladish_graph_find_client(graph_ptr, client_handle);
  if (client_ptr != NULL)
  {
      return client_ptr->name;
  }

  ASSERT_NO_PASS;
  return NULL;
}

bool ladish_graph_client_is_empty(ladish_graph_handle graph_handle, ladish_client_handle client_handle)
{
  struct ladish_graph_client * client_ptr;

  client_ptr = ladish_graph_find_client(graph_ptr, client_handle);
  if (client_ptr != NULL)
  {
    return list_empty(&client_ptr->ports);
  }

  ASSERT_NO_PASS;
  return true;
}

bool ladish_graph_client_looks_empty(ladish_graph_handle graph_handle, ladish_client_handle client_handle)
{
  struct ladish_graph_client * client_ptr;

  client_ptr = ladish_graph_find_client(graph_ptr, client_handle);
  if (client_ptr == NULL)
  {
    ASSERT_NO_PASS;
    return true;
  }

  if (ladish_graph_client_looks_empty_internal(graph_ptr, client_ptr))
  {
    //log_info("client '%s' looks empty in graph %s", client_ptr->name, graph_ptr->opath != NULL ? graph_ptr->opath : "JACK");
    return true;
  }

  return false;
}

bool ladish_graph_client_is_hidden(ladish_graph_handle graph_handle, ladish_client_handle client_handle)
{
  struct ladish_graph_client * client_ptr;

  client_ptr = ladish_graph_find_client(graph_ptr, client_handle);
  if (client_ptr != NULL)
  {
    return client_ptr->hidden;
  }

  ASSERT_NO_PASS;
  return true;
}

void ladish_try_connect_hidden_connections(ladish_graph_handle graph_handle)
{
  struct list_head * node_ptr;
  struct ladish_graph_connection * connection_ptr;

  if (!list_empty(&graph_ptr->connections) && graph_ptr->connect_handler == NULL)
  {
    ASSERT_NO_PASS;
    return;
  }

  ASSERT(graph_ptr->opath != NULL);

  list_for_each(node_ptr, &graph_ptr->connections)
  {
    connection_ptr = list_entry(node_ptr, struct ladish_graph_connection, siblings);
    if (connection_ptr->hidden &&
        !connection_ptr->changing &&
        !connection_ptr->port1_ptr->hidden &&
        !connection_ptr->port2_ptr->hidden)
    {
      log_info(
        "auto connecting '%s':'%s' to '%s':'%s'",
        connection_ptr->port1_ptr->client_ptr->name,
        connection_ptr->port1_ptr->name,
        connection_ptr->port2_ptr->client_ptr->name,
        connection_ptr->port2_ptr->name);

      connection_ptr->changing = true;
      if (!graph_ptr->connect_handler(graph_ptr->context, graph_handle, connection_ptr->port1_ptr->port, connection_ptr->port2_ptr->port))
      {
        connection_ptr->changing = false;
        log_error("auto connect failed.");
      }
    }
  }
}

void ladish_graph_hide_non_virtual(ladish_graph_handle graph_handle)
{
  struct list_head * node_ptr;
  struct ladish_graph_connection * connection_ptr;
  struct ladish_graph_port * port_ptr;
  struct ladish_graph_client * client_ptr;

  log_info("hiding everything in graph %s", graph_ptr->opath != NULL ? graph_ptr->opath : "JACK");

  list_for_each(node_ptr, &graph_ptr->connections)
  {
    connection_ptr = list_entry(node_ptr, struct ladish_graph_connection, siblings);
    if (!connection_ptr->hidden &&
        (ladish_client_get_jack_id(connection_ptr->port1_ptr->client_ptr->client) != 0 ||
         ladish_client_get_jack_id(connection_ptr->port2_ptr->client_ptr->client) != 0))
    {
      log_debug("hidding connection between ports %"PRIu64" and %"PRIu64, connection_ptr->port1_ptr->id, connection_ptr->port2_ptr->id);
      ladish_graph_hide_connection_internal(graph_ptr, connection_ptr);
    }
  }

  list_for_each(node_ptr, &graph_ptr->ports)
  {
    port_ptr = list_entry(node_ptr, struct ladish_graph_port, siblings_graph);
    if (!port_ptr->hidden && ladish_client_get_jack_id(port_ptr->client_ptr->client) != 0)
    {
      ladish_port_set_jack_id(port_ptr->port, 0);
      ladish_graph_hide_port_internal(graph_ptr, port_ptr);
    }
  }

  list_for_each(node_ptr, &graph_ptr->clients)
  {
    client_ptr = list_entry(node_ptr, struct ladish_graph_client, siblings);
    if (!client_ptr->hidden && ladish_client_get_jack_id(client_ptr->client) != 0)
    {
      ladish_client_set_jack_id(client_ptr->client, 0);
      ladish_graph_hide_client_internal(graph_ptr, client_ptr);
    }
  }
}

void ladish_graph_get_port_uuid(ladish_graph_handle graph_handle, ladish_port_handle port, uuid_t uuid_ptr)
{
  struct ladish_graph_port * port_ptr;

  port_ptr = ladish_graph_find_port(graph_ptr, port);
  ASSERT(port_ptr != NULL);
  ASSERT(port_ptr->link);

  uuid_copy(uuid_ptr, port_ptr->link_uuid_override);
}

const char * ladish_graph_get_port_name(ladish_graph_handle graph_handle, ladish_port_handle port)
{
  struct ladish_graph_port * port_ptr;

  port_ptr = ladish_graph_find_port(graph_ptr, port);
  ASSERT(port_ptr != NULL);

  return port_ptr->name;
}

void ladish_graph_set_link_port_override_uuid(ladish_graph_handle graph_handle, const uuid_t uuid, const uuid_t override_uuid)
{
  struct ladish_graph_port * port_ptr;

  port_ptr = ladish_graph_find_port_by_uuid_internal(graph_ptr, uuid, false);
  ASSERT(ladish_port_is_link(port_ptr->port));
  uuid_copy(port_ptr->link_uuid_override, override_uuid);
}

bool
ladish_graph_iterate_nodes(
  ladish_graph_handle graph_handle,
  bool skip_hidden,
  void * vgraph_filter,
  void * callback_context,
  bool
  (* client_begin_callback)(
    void * context,
    ladish_graph_handle graph_handle,
    ladish_client_handle client_handle,
    const char * client_name,
    void ** client_iteration_context_ptr_ptr),
  bool
  (* port_callback)(
    void * context,
    ladish_graph_handle graph_handle,
    void * client_iteration_context_ptr,
    ladish_client_handle client_handle,
    const char * client_name,
    ladish_port_handle port_handle,
    const char * port_name,
    uint32_t port_type,
    uint32_t port_flags),
  bool
  (* client_end_callback)(
    void * context,
    ladish_graph_handle graph_handle,
    ladish_client_handle client_handle,
    const char * client_name,
    void * client_iteration_context_ptr))
{
  struct list_head * client_node_ptr;
  struct ladish_graph_client * client_ptr;
  void * client_context;
  struct list_head * port_node_ptr;
  struct ladish_graph_port * port_ptr;
  void * vgraph;

  list_for_each(client_node_ptr, &graph_ptr->clients)
  {
    client_ptr = list_entry(client_node_ptr, struct ladish_graph_client, siblings);

    if (skip_hidden && client_ptr->hidden)
    {
      continue;
    }

    if (vgraph_filter != NULL)
    {
      vgraph = ladish_client_get_vgraph(client_ptr->client);
      if (vgraph != vgraph_filter)
      {
        continue;
      }
    }

    if (client_begin_callback != NULL)
    {
      if (!client_begin_callback(callback_context, graph_handle, client_ptr->client, client_ptr->name, &client_context))
      {
        return false;
      }
    }
    else
    {
      client_context = NULL;
    }

    if (port_callback == NULL)
    {
      continue;
    }

    list_for_each(port_node_ptr, &client_ptr->ports)
    {
      port_ptr = list_entry(port_node_ptr, struct ladish_graph_port, siblings_client);

      if (skip_hidden && port_ptr->hidden)
      {
        continue;
      }

      if (!port_callback(
            callback_context,
            graph_handle,
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

    if (client_end_callback != NULL)
    {
      if (!client_end_callback(callback_context, graph_handle, client_ptr->client, client_ptr->name, &client_context))
      {
        return false;
      }
    }
  }

  return true;
}

bool
ladish_graph_iterate_connections(
  ladish_graph_handle graph_handle,
  bool skip_hidden,
  void * callback_context,
  bool (* callback)(
    void * context,
    ladish_graph_handle graph_handle,
    ladish_port_handle port1_handle,
    ladish_port_handle port2_handle,
    ladish_dict_handle dict))
{
  struct list_head * node_ptr;
  struct ladish_graph_connection * connection_ptr;

  list_for_each(node_ptr, &graph_ptr->connections)
  {
    connection_ptr = list_entry(node_ptr, struct ladish_graph_connection, siblings);

    if (skip_hidden && connection_ptr->hidden)
    {
      continue;
    }

    if (!callback(callback_context, graph_handle, connection_ptr->port1_ptr->port, connection_ptr->port2_ptr->port, connection_ptr->dict))
    {
      return false;
    }
  }

  return true;
}

static
bool
dump_dict_entry(
  void * context,
  const char * key,
  const char * value)
{
  log_info("%s  key '%s' with value '%s'", (const char *)context, key, value);
  return true;
}

static
void
dump_dict(
  const char * indent,
  ladish_dict_handle dict)
{
  if (ladish_dict_is_empty(dict))
  {
    return;
  }

  log_info("%sdict:", indent);
  ladish_dict_iterate(dict, (void *)indent, dump_dict_entry);
}

void ladish_graph_dump(ladish_graph_handle graph_handle)
{
  struct list_head * client_node_ptr;
  struct ladish_graph_client * client_ptr;
  struct list_head * port_node_ptr;
  struct ladish_graph_port * port_ptr;
  struct list_head * connection_node_ptr;
  struct ladish_graph_connection * connection_ptr;
  uuid_t uuid;
  char uuid_str[37];

  log_info("graph %s", graph_ptr->opath != NULL ? graph_ptr->opath : "JACK");
  log_info("  version %"PRIu64, graph_ptr->graph_version);
  log_info("  persist: %s", graph_ptr->persist ? "yes" : "no");
  dump_dict("  ", graph_ptr->dict);
  log_info("  clients:");
  list_for_each(client_node_ptr, &graph_ptr->clients)
  {
    client_ptr = list_entry(client_node_ptr, struct ladish_graph_client, siblings);
    log_info("    %s client '%s', id=%"PRIu64", ptr=%p%", client_ptr->hidden ? "invisible" : "visible", client_ptr->name, client_ptr->id, client_ptr->client);
    ladish_client_get_uuid(client_ptr->client, uuid);
    uuid_unparse(uuid, uuid_str);
    log_info("    uuid=%s", uuid_str);
    if (ladish_client_get_interlink(client_ptr->client, uuid))
    {
      uuid_unparse(uuid, uuid_str);
      log_info("    interlink=%s", uuid_str);
    }
    else
    {
      log_info("    no interlink");
    }
    dump_dict("      ", ladish_client_get_dict(client_ptr->client));
    log_info("      ports:");
    list_for_each(port_node_ptr, &client_ptr->ports)
    {
      port_ptr = list_entry(port_node_ptr, struct ladish_graph_port, siblings_client);

      ladish_port_get_uuid(port_ptr->port, uuid);
      uuid_unparse(uuid, uuid_str);

      log_info("        %s port '%s', uuid=%s, id=%"PRIu64", type=0x%"PRIX32", flags=0x%"PRIX32", ptr=%p", port_ptr->hidden ? "invisible" : "visible", port_ptr->name, uuid_str, port_ptr->id, port_ptr->type, port_ptr->flags, port_ptr->port);
      dump_dict("        ", ladish_port_get_dict(port_ptr->port));
    }
  }
  log_info("  connections:");
  list_for_each(connection_node_ptr, &graph_ptr->connections)
  {
    connection_ptr = list_entry(connection_node_ptr, struct ladish_graph_connection, siblings);

    log_info(
      "    %s connection '%s':'%s' - '%s':'%s'%s",
      connection_ptr->hidden ? "invisible" : "visible",
      connection_ptr->port1_ptr->client_ptr->name,
      connection_ptr->port1_ptr->name,
      connection_ptr->port2_ptr->client_ptr->name,
      connection_ptr->port2_ptr->name,
      connection_ptr->changing ? " [changing]" : "");
    dump_dict("      ", connection_ptr->dict);
  }
}

void ladish_graph_clear_persist(ladish_graph_handle graph_handle)
{
  log_info("Clearing persist flag for graph", graph_ptr->opath != NULL ? graph_ptr->opath : "JACK");
  graph_ptr->persist = false;
}

void ladish_graph_set_persist(ladish_graph_handle graph_handle)
{
  log_info("Setting persist flag for graph", graph_ptr->opath != NULL ? graph_ptr->opath : "JACK");
  graph_ptr->persist = true;
}

bool ladish_graph_is_persist(ladish_graph_handle graph_handle)
{
  return graph_ptr->persist;
}

bool ladish_graph_looks_empty(ladish_graph_handle graph_handle)
{
  struct list_head * node_ptr;
  struct ladish_graph_connection * connection_ptr;
  struct ladish_graph_client * client_ptr;

  list_for_each(node_ptr, &graph_ptr->connections)
  {
    connection_ptr = list_entry(node_ptr, struct ladish_graph_connection, siblings);

    if (!connection_ptr->hidden)
    {
      return false;
    }
  }

  list_for_each(node_ptr, &graph_ptr->clients)
  {
    client_ptr = list_entry(node_ptr, struct ladish_graph_client, siblings);
    if (!ladish_graph_client_looks_empty_internal(graph_ptr, client_ptr))
    {
      return false;
    }
  }

  return true;
}

#undef graph_ptr
#define graph_ptr ((struct ladish_graph *)context)

static
bool
ladish_graph_copy_client_begin_callback(
  void * context,
  ladish_graph_handle graph_handle,
  ladish_client_handle client_handle,
  const char * client_name,
  void ** client_iteration_context_ptr_ptr)
{
  ladish_client_handle copy;

  if (!ladish_client_create_copy(client_handle, &copy))
  {
    return false;
  }

  if (!ladish_graph_add_client(context, copy, client_name, false))
  {
    ladish_client_destroy(copy);
    return false;
  }

  *client_iteration_context_ptr_ptr = copy;

  return true;
}

static
bool
ladish_graph_copy_port_callback(
  void * context,
  ladish_graph_handle graph_handle,
  void * client_iteration_context_ptr,
  ladish_client_handle client_handle,
  const char * client_name,
  ladish_port_handle port_handle,
  const char * port_name,
  uint32_t port_type,
  uint32_t port_flags)
{
  ladish_port_handle copy;

  if (!ladish_port_create_copy(port_handle, &copy))
  {
    return false;
  }

  if (!ladish_graph_add_port(context, client_iteration_context_ptr, copy, port_name, port_type, port_flags, true))
  {
    ladish_port_destroy(copy);
    return false;
  }

  return true;
}

#undef graph_ptr

bool ladish_graph_copy(ladish_graph_handle src, ladish_graph_handle dest, bool skip_hidden)
{
  return ladish_graph_iterate_nodes(
    src,
    skip_hidden,
    NULL,
    dest,
    ladish_graph_copy_client_begin_callback,
    ladish_graph_copy_port_callback,
    NULL);
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

INTERFACE_BEGIN(g_interface_patchbay, JACKDBUS_IFACE_PATCHBAY)
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
  INTERFACE_EXPOSE_SIGNALS
INTERFACE_END
