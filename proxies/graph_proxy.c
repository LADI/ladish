/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009,2010,2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation graph object that is backed through D-Bus
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

#include "graph_proxy.h"

struct monitor
{
  struct list_head siblings;
  void * context;
  void (* clear)(void * context);
  void (* client_appeared)(void * context, uint64_t id, const char * name);
  void (* client_renamed)(void * context, uint64_t client_id, const char * old_client_name, const char * new_client_name);
  void (* client_disappeared)(void * context, uint64_t id);
  void (* port_appeared)(void * context, uint64_t client_id, uint64_t port_id, const char * port_name, bool is_input, bool is_terminal, bool is_midi);
  void (* port_renamed)(void * context, uint64_t client_id, uint64_t port_id, const char * old_port_name, const char * new_port_name);
  void (* port_disappeared)(void * context, uint64_t client_id, uint64_t port_id);
  void (* ports_connected)(void * context, uint64_t client1_id, uint64_t port1_id, uint64_t client2_id, uint64_t port2_id);
  void (* ports_disconnected)(void * context, uint64_t client1_id, uint64_t port1_id, uint64_t client2_id, uint64_t port2_id);
};

struct graph
{
  struct list_head monitors;
  char * service;
  char * object;
  uint64_t version;
  bool active;
  bool graph_dict_supported;
  bool graph_manager_supported;
};

static struct dbus_signal_hook g_signal_hooks[];

static void clear(struct graph * graph_ptr)
{
  struct list_head * node_ptr;
  struct monitor * monitor_ptr;

  list_for_each(node_ptr, &graph_ptr->monitors)
  {
    monitor_ptr = list_entry(node_ptr, struct monitor, siblings);
    monitor_ptr->clear(monitor_ptr->context);
  }
}

static void client_appeared(struct graph * graph_ptr, uint64_t id, const char * name)
{
  struct list_head * node_ptr;
  struct monitor * monitor_ptr;

  list_for_each(node_ptr, &graph_ptr->monitors)
  {
    monitor_ptr = list_entry(node_ptr, struct monitor, siblings);
    monitor_ptr->client_appeared(monitor_ptr->context, id, name);
  }
}

static
void
client_renamed(
  struct graph * graph_ptr,
  uint64_t client_id,
  const char * old_client_name,
  const char * new_client_name)
{
  struct list_head * node_ptr;
  struct monitor * monitor_ptr;

  list_for_each(node_ptr, &graph_ptr->monitors)
  {
    monitor_ptr = list_entry(node_ptr, struct monitor, siblings);
    if (monitor_ptr->client_renamed != NULL)
    {
      monitor_ptr->client_renamed(monitor_ptr->context, client_id, old_client_name, new_client_name);
    }
  }
}

static void client_disappeared(struct graph * graph_ptr, uint64_t id)
{
  struct list_head * node_ptr;
  struct monitor * monitor_ptr;

  list_for_each(node_ptr, &graph_ptr->monitors)
  {
    monitor_ptr = list_entry(node_ptr, struct monitor, siblings);
    monitor_ptr->client_disappeared(monitor_ptr->context, id);
  }
}

static
void
port_appeared(
  struct graph * graph_ptr,
  uint64_t client_id,
  uint64_t port_id,
  const char * port_name,
  uint32_t port_flags,
  uint32_t port_type)
{
  struct list_head * node_ptr;
  struct monitor * monitor_ptr;
  bool is_input;
  bool is_terminal;
  bool is_midi;

  if (port_type != JACKDBUS_PORT_TYPE_AUDIO && port_type != JACKDBUS_PORT_TYPE_MIDI)
  {
    log_error("Unknown JACK D-Bus port type %d", (unsigned int)port_type);
    return;
  }

  is_input = port_flags & JACKDBUS_PORT_FLAG_INPUT;
  is_terminal = port_flags & JACKDBUS_PORT_FLAG_TERMINAL;
  is_midi = port_type == JACKDBUS_PORT_TYPE_MIDI;

  list_for_each(node_ptr, &graph_ptr->monitors)
  {
    monitor_ptr = list_entry(node_ptr, struct monitor, siblings);
    monitor_ptr->port_appeared(monitor_ptr->context, client_id, port_id, port_name, is_input, is_terminal, is_midi);
  }
}

static
void
port_disappeared(
  struct graph * graph_ptr,
  uint64_t client_id,
  uint64_t port_id)
{
  struct list_head * node_ptr;
  struct monitor * monitor_ptr;

  list_for_each(node_ptr, &graph_ptr->monitors)
  {
    monitor_ptr = list_entry(node_ptr, struct monitor, siblings);
    monitor_ptr->port_disappeared(monitor_ptr->context, client_id, port_id);
  }
}

static
void
port_renamed(
  struct graph * graph_ptr,
  uint64_t client_id,
  uint64_t port_id,
  const char * old_port_name,
  const char * new_port_name)
{
  struct list_head * node_ptr;
  struct monitor * monitor_ptr;

  list_for_each(node_ptr, &graph_ptr->monitors)
  {
    monitor_ptr = list_entry(node_ptr, struct monitor, siblings);
    monitor_ptr->port_renamed(monitor_ptr->context, client_id, port_id, old_port_name, new_port_name);
  }
}

static
void
ports_connected(
  struct graph * graph_ptr,
  uint64_t client1_id,
  uint64_t port1_id,
  uint64_t client2_id,
  uint64_t port2_id)
{
  struct list_head * node_ptr;
  struct monitor * monitor_ptr;

  list_for_each(node_ptr, &graph_ptr->monitors)
  {
    monitor_ptr = list_entry(node_ptr, struct monitor, siblings);
    monitor_ptr->ports_connected(monitor_ptr->context, client1_id, port1_id, client2_id, port2_id);
  }
}

static
void
ports_disconnected(
  struct graph * graph_ptr,
  uint64_t client1_id,
  uint64_t port1_id,
  uint64_t client2_id,
  uint64_t port2_id)
{
  struct list_head * node_ptr;
  struct monitor * monitor_ptr;

  list_for_each(node_ptr, &graph_ptr->monitors)
  {
    monitor_ptr = list_entry(node_ptr, struct monitor, siblings);
    monitor_ptr->ports_disconnected(monitor_ptr->context, client1_id, port1_id, client2_id, port2_id);
  }
}

static void refresh_internal(struct graph * graph_ptr, bool force)
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

  log_info("refresh_internal() called");

  if (force)
  {
    version = 0; // workaround module split/join stupidity
  }
  else
  {
    version = graph_ptr->version;
  }

  if (!dbus_call(0, graph_ptr->service, graph_ptr->object, JACKDBUS_IFACE_PATCHBAY, "GetGraph", "t", &version, NULL, &reply_ptr))
  {
    log_error("GetGraph() failed.");
    return;
  }

  reply_signature = dbus_message_get_signature(reply_ptr);

  if (strcmp(reply_signature, "ta(tsa(tsuu))a(tstststst)") != 0)
  {
    log_error("GetGraph() reply signature mismatch. '%s'", reply_signature);
    goto unref;
  }

  dbus_message_iter_init(reply_ptr, &iter);

  //log_info_msg("version " + (char)dbus_message_iter_get_arg_type(&iter));
  dbus_message_iter_get_basic(&iter, &version);
  dbus_message_iter_next(&iter);

  if (!force && version <= graph_ptr->version)
  {
    goto unref;
  }

  clear(graph_ptr);

  //log_info("got new graph version %llu", (unsigned long long)version);
  graph_ptr->version = version;

  //info_msg((std::string)"clients " + (char)dbus_message_iter_get_arg_type(&iter));

  for (dbus_message_iter_recurse(&iter, &clients_array_iter);
       dbus_message_iter_get_arg_type(&clients_array_iter) != DBUS_TYPE_INVALID;
       dbus_message_iter_next(&clients_array_iter))
  {
    //info_msg((std::string)"a client " + (char)dbus_message_iter_get_arg_type(&clients_array_iter));
    dbus_message_iter_recurse(&clients_array_iter, &client_struct_iter);

    dbus_message_iter_get_basic(&client_struct_iter, &client_id);
    dbus_message_iter_next(&client_struct_iter);

    dbus_message_iter_get_basic(&client_struct_iter, &client_name);
    dbus_message_iter_next(&client_struct_iter);

    //info_msg((std::string)"client '" + client_name + "'");

    client_appeared(graph_ptr, client_id, client_name);

    for (dbus_message_iter_recurse(&client_struct_iter, &ports_array_iter);
         dbus_message_iter_get_arg_type(&ports_array_iter) != DBUS_TYPE_INVALID;
         dbus_message_iter_next(&ports_array_iter))
    {
      //info_msg((std::string)"a port " + (char)dbus_message_iter_get_arg_type(&ports_array_iter));
      dbus_message_iter_recurse(&ports_array_iter, &port_struct_iter);

      dbus_message_iter_get_basic(&port_struct_iter, &port_id);
      dbus_message_iter_next(&port_struct_iter);

      dbus_message_iter_get_basic(&port_struct_iter, &port_name);
      dbus_message_iter_next(&port_struct_iter);

      dbus_message_iter_get_basic(&port_struct_iter, &port_flags);
      dbus_message_iter_next(&port_struct_iter);

      dbus_message_iter_get_basic(&port_struct_iter, &port_type);
      dbus_message_iter_next(&port_struct_iter);

      //info_msg((std::string)"port: " + port_name);

      port_appeared(graph_ptr, client_id, port_id, port_name, port_flags, port_type);
    }

    dbus_message_iter_next(&client_struct_iter);
  }

  dbus_message_iter_next(&iter);

  for (dbus_message_iter_recurse(&iter, &connections_array_iter);
       dbus_message_iter_get_arg_type(&connections_array_iter) != DBUS_TYPE_INVALID;
       dbus_message_iter_next(&connections_array_iter))
  {
    //info_msg((std::string)"a connection " + (char)dbus_message_iter_get_arg_type(&connections_array_iter));
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

    ports_connected(graph_ptr, client_id, port_id, client2_id, port2_id);
  }

unref:
  dbus_message_unref(reply_ptr);
}

bool
graph_proxy_create(
  const char * service,
  const char * object,
  bool graph_dict_supported,
  bool graph_manager_supported,
  graph_proxy_handle * graph_proxy_handle_ptr)
{
  struct graph * graph_ptr;

  graph_ptr = malloc(sizeof(struct graph));
  if (graph_ptr == NULL)
  {
    log_error("malloc() failed to allocate struct graph");
    goto fail;
  }

  graph_ptr->service = strdup(service);
  if (graph_ptr->service == NULL)
  {
    log_error("strdup() failed too duplicate service name '%s'", service);
    goto free_graph;
  }

  graph_ptr->object = strdup(object);
  if (graph_ptr->object == NULL)
  {
    log_error("strdup() failed too duplicate object name '%s'", object);
    goto free_service;
  }

  INIT_LIST_HEAD(&graph_ptr->monitors);

  graph_ptr->version = 0;
  graph_ptr->active = false;

  graph_ptr->graph_dict_supported = graph_dict_supported;
  graph_ptr->graph_manager_supported = graph_manager_supported;

  *graph_proxy_handle_ptr = (graph_proxy_handle)graph_ptr;

  return true;

free_service:
  free(graph_ptr->service);

free_graph:
  free(graph_ptr);

fail:
  return false;
}

#define graph_ptr ((struct graph *)graph)

const char * graph_proxy_get_service(graph_proxy_handle graph)
{
  return graph_ptr->service;
}

const char * graph_proxy_get_object(graph_proxy_handle graph)
{
  return graph_ptr->object;
}

void
graph_proxy_destroy(
  graph_proxy_handle graph)
{
  ASSERT(list_empty(&graph_ptr->monitors));

  if (graph_ptr->active)
  {
    dbus_unregister_object_signal_hooks(
      g_dbus_connection,
      graph_ptr->service,
      graph_ptr->object,
      JACKDBUS_IFACE_PATCHBAY);
  }

  free(graph_ptr->object);
  free(graph_ptr->service);
  free(graph_ptr);
}

bool
graph_proxy_activate(
  graph_proxy_handle graph)
{
  if (list_empty(&graph_ptr->monitors))
  {
    log_error("no monitors to activate");
    return false;
  }

  if (graph_ptr->active)
  {
    log_error("graph already active");
    return false;
  }

  if (!dbus_register_object_signal_hooks(
        g_dbus_connection,
        graph_ptr->service,
        graph_ptr->object,
        JACKDBUS_IFACE_PATCHBAY,
        graph_ptr,
        g_signal_hooks))
  {
    return false;
  }

  graph_ptr->active = true;

  refresh_internal(graph_ptr, true);

  return true;
}

bool
graph_proxy_attach(
  graph_proxy_handle graph,
  void * context,
  void (* clear)(void * context),
  void (* client_appeared)(void * context, uint64_t id, const char * name),
  void (* client_renamed)(void * context, uint64_t client_id, const char * old_client_name, const char * new_client_name),
  void (* client_disappeared)(void * context, uint64_t id),
  void (* port_appeared)(void * context, uint64_t client_id, uint64_t port_id, const char * port_name, bool is_input, bool is_terminal, bool is_midi),
  void (* port_renamed)(void * context, uint64_t client_id, uint64_t port_id, const char * old_port_name, const char * new_port_name),
  void (* port_disappeared)(void * context, uint64_t client_id, uint64_t port_id),
  void (* ports_connected)(void * context, uint64_t client1_id, uint64_t port1_id, uint64_t client2_id, uint64_t port2_id),
  void (* ports_disconnected)(void * context, uint64_t client1_id, uint64_t port1_id, uint64_t client2_id, uint64_t port2_id))
{
  struct monitor * monitor_ptr;

  if (graph_ptr->active)
  {
    return false;
  }

  monitor_ptr = malloc(sizeof(struct monitor));
  if (monitor_ptr == NULL)
  {
    log_error("malloc() failed to allocate struct monitor");
    return false;
  }

  monitor_ptr->context = context;
  monitor_ptr->clear = clear;
  monitor_ptr->client_appeared = client_appeared;
  monitor_ptr->client_renamed = client_renamed;
  monitor_ptr->client_disappeared = client_disappeared;
  monitor_ptr->port_appeared = port_appeared;
  monitor_ptr->port_renamed = port_renamed;
  monitor_ptr->port_disappeared = port_disappeared;
  monitor_ptr->ports_connected = ports_connected;
  monitor_ptr->ports_disconnected = ports_disconnected;

  list_add_tail(&monitor_ptr->siblings, &graph_ptr->monitors);

  return true;
}

void
graph_proxy_detach(
  graph_proxy_handle graph,
  void * context)
{
  struct list_head * node_ptr;
  struct monitor * monitor_ptr;

  list_for_each(node_ptr, &graph_ptr->monitors)
  {
    monitor_ptr = list_entry(node_ptr, struct monitor, siblings);
    if (monitor_ptr->context == context)
    {
      list_del(&monitor_ptr->siblings);
      free(monitor_ptr);
      return;
    }
  }

  ASSERT(false);
}

bool
graph_proxy_connect_ports(
  graph_proxy_handle graph,
  uint64_t port1_id,
  uint64_t port2_id)
{
  if (!dbus_call(0, graph_ptr->service, graph_ptr->object, JACKDBUS_IFACE_PATCHBAY, "ConnectPortsByID", "tt", &port1_id, &port2_id, ""))
  {
    log_error("ConnectPortsByID() failed.");
    return false;
  }

  return true;
}

bool
graph_proxy_disconnect_ports(
  graph_proxy_handle graph,
  uint64_t port1_id,
  uint64_t port2_id)
{
  if (!dbus_call(0, graph_ptr->service, graph_ptr->object, JACKDBUS_IFACE_PATCHBAY, "DisconnectPortsByID", "tt", &port1_id, &port2_id, ""))
  {
    log_error("DisconnectPortsByID() failed.");
    return false;
  }

  return true;
}

static void on_client_appeared(void * graph, DBusMessage * message_ptr)
{
  dbus_uint64_t new_graph_version;
  dbus_uint64_t client_id;
  const char * client_name;

  if (!dbus_message_get_args(
        message_ptr,
        &g_dbus_error,
        DBUS_TYPE_UINT64, &new_graph_version,
        DBUS_TYPE_UINT64, &client_id,
        DBUS_TYPE_STRING, &client_name,
        DBUS_TYPE_INVALID))
  {
    log_error("dbus_message_get_args() failed to extract ClientAppeared signal arguments (%s)", g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  //log_info("ClientAppeared, %s(%llu), graph %llu", client_name, client_id, new_graph_version);

  if (new_graph_version > graph_ptr->version)
  {
    //log_info("got new graph version %llu", (unsigned long long)new_graph_version);
    graph_ptr->version = new_graph_version;
    client_appeared(graph_ptr, client_id, client_name);
  }
}

static void on_client_renamed(void * graph, DBusMessage * message_ptr)
{
  dbus_uint64_t new_graph_version;
  dbus_uint64_t client_id;
  const char * old_client_name;
  const char * new_client_name;

  if (!dbus_message_get_args(
        message_ptr,
        &g_dbus_error,
        DBUS_TYPE_UINT64, &new_graph_version,
        DBUS_TYPE_UINT64, &client_id,
        DBUS_TYPE_STRING, &old_client_name,
        DBUS_TYPE_STRING, &new_client_name,
        DBUS_TYPE_INVALID))
  {
    log_error("dbus_message_get_args() failed to extract ClientRenamed signal arguments (%s)", g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  if (new_graph_version > graph_ptr->version)
  {
    //log_info("got new graph version %llu", (unsigned long long)new_graph_version);
    graph_ptr->version = new_graph_version;
    client_renamed(graph_ptr, client_id, old_client_name, new_client_name);
  }
}

static void on_client_disappeared(void * graph, DBusMessage * message_ptr)
{
  dbus_uint64_t new_graph_version;
  dbus_uint64_t client_id;
  const char * client_name;

  if (!dbus_message_get_args(
        message_ptr,
        &g_dbus_error,
        DBUS_TYPE_UINT64, &new_graph_version,
        DBUS_TYPE_UINT64, &client_id,
        DBUS_TYPE_STRING, &client_name,
        DBUS_TYPE_INVALID))
  {
    log_error("dbus_message_get_args() failed to extract ClientDisappeared signal arguments (%s)", g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  //log_info("ClientDisappeared, %s(%llu)", client_name, client_id);

  if (new_graph_version > graph_ptr->version)
  {
    //log_info("got new graph version %llu", (unsigned long long)new_graph_version);
    graph_ptr->version = new_graph_version;
    client_disappeared(graph_ptr, client_id);
  }
}

static void on_port_appeared(void * graph, DBusMessage * message_ptr)
{
  dbus_uint64_t new_graph_version;
  dbus_uint64_t client_id;
  const char * client_name;
  dbus_uint64_t port_id;
  const char * port_name;
  dbus_uint32_t port_flags;
  dbus_uint32_t port_type;

  if (!dbus_message_get_args(
        message_ptr,
        &g_dbus_error,
        DBUS_TYPE_UINT64, &new_graph_version,
        DBUS_TYPE_UINT64, &client_id,
        DBUS_TYPE_STRING, &client_name,
        DBUS_TYPE_UINT64, &port_id,
        DBUS_TYPE_STRING, &port_name,
        DBUS_TYPE_UINT32, &port_flags,
        DBUS_TYPE_UINT32, &port_type,
        DBUS_TYPE_INVALID))
  {
    log_error("dbus_message_get_args() failed to extract PortAppeared signal arguments (%s)", g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  //me->info_msg(str(boost::format("PortAppeared, %s(%llu):%s(%llu), %lu, %lu") % client_name % client_id % port_name % port_id % port_flags % port_type));

  if (new_graph_version > graph_ptr->version)
  {
    //log_info("got new graph version %llu", (unsigned long long)new_graph_version);
    graph_ptr->version = new_graph_version;
    port_appeared(graph_ptr, client_id, port_id, port_name, port_flags, port_type);
  }
}

static void on_port_renamed(void * graph, DBusMessage * message_ptr)
{
  dbus_uint64_t new_graph_version;
  dbus_uint64_t client_id;
  const char * client_name;
  dbus_uint64_t port_id;
  const char * old_port_name;
  const char * new_port_name;

  if (!dbus_message_get_args(
        message_ptr,
        &g_dbus_error,
        DBUS_TYPE_UINT64, &new_graph_version,
        DBUS_TYPE_UINT64, &client_id,
        DBUS_TYPE_STRING, &client_name,
        DBUS_TYPE_UINT64, &port_id,
        DBUS_TYPE_STRING, &old_port_name,
        DBUS_TYPE_STRING, &new_port_name,
        DBUS_TYPE_INVALID))
  {
    log_error("dbus_message_get_args() failed to extract PortRenamed signal arguments (%s)", g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  if (new_graph_version > graph_ptr->version)
  {
    //log_info("got new graph version %llu", (unsigned long long)new_graph_version);
    graph_ptr->version = new_graph_version;
    port_renamed(graph_ptr, client_id, port_id, old_port_name, new_port_name);
  }
}

static void on_port_disappeared(void * graph, DBusMessage * message_ptr)
{
  dbus_uint64_t new_graph_version;
  dbus_uint64_t client_id;
  const char * client_name;
  dbus_uint64_t port_id;
  const char * port_name;

  if (!dbus_message_get_args(
        message_ptr,
        &g_dbus_error,
        DBUS_TYPE_UINT64, &new_graph_version,
        DBUS_TYPE_UINT64, &client_id,
        DBUS_TYPE_STRING, &client_name,
        DBUS_TYPE_UINT64, &port_id,
        DBUS_TYPE_STRING, &port_name,
        DBUS_TYPE_INVALID))
  {
    log_error("dbus_message_get_args() failed to extract PortDisappeared signal arguments (%s)", g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  //me->info_msg(str(boost::format("PortDisappeared, %s(%llu):%s(%llu)") % client_name % client_id % port_name % port_id));

  if (new_graph_version > graph_ptr->version)
  {
    //log_info("got new graph version %llu", (unsigned long long)new_graph_version);
    graph_ptr->version = new_graph_version;
    port_disappeared(graph_ptr, client_id, port_id);
  }
}

static void on_ports_connected(void * graph, DBusMessage * message_ptr)
{
  dbus_uint64_t new_graph_version;
  dbus_uint64_t client_id;
  const char * client_name;
  dbus_uint64_t port_id;
  const char * port_name;
  dbus_uint64_t client2_id;
  const char * client2_name;
  dbus_uint64_t port2_id;
  const char * port2_name;
  dbus_uint64_t connection_id;

  if (!dbus_message_get_args(
        message_ptr,
        &g_dbus_error,
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
        DBUS_TYPE_INVALID))
  {
    log_error("dbus_message_get_args() failed to extract PortsConnected signal arguments (%s)", g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  if (new_graph_version > graph_ptr->version)
  {
    //log_info("got new graph version %llu", (unsigned long long)new_graph_version);
    graph_ptr->version = new_graph_version;
    ports_connected(graph_ptr, client_id, port_id, client2_id, port2_id);
  }
}

static void on_ports_disconnected(void * graph, DBusMessage * message_ptr)
{
  dbus_uint64_t new_graph_version;
  dbus_uint64_t client_id;
  const char * client_name;
  dbus_uint64_t port_id;
  const char * port_name;
  dbus_uint64_t client2_id;
  const char * client2_name;
  dbus_uint64_t port2_id;
  const char * port2_name;
  dbus_uint64_t connection_id;

  if (!dbus_message_get_args(
        message_ptr,
        &g_dbus_error,
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
        DBUS_TYPE_INVALID))
  {
    log_error("dbus_message_get_args() failed to extract PortsConnected signal arguments (%s)", g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  if (new_graph_version > graph_ptr->version)
  {
    //log_info("got new graph version %llu", (unsigned long long)new_graph_version);
    graph_ptr->version = new_graph_version;
    ports_disconnected(graph_ptr, client_id, port_id, client2_id, port2_id);
  }
}

bool
graph_proxy_dict_entry_set(
  graph_proxy_handle graph,
  uint32_t object_type,
  uint64_t object_id,
  const char * key,
  const char * value)
{
  if (!graph_ptr->graph_dict_supported)
  {
    return false;
  }

  if (!dbus_call(0, graph_ptr->service, graph_ptr->object, IFACE_GRAPH_DICT, "Set", "utss", &object_type, &object_id, &key, &value, ""))
  {
    log_error(IFACE_GRAPH_DICT ".Set() failed.");
    return false;
  }

  return true;
}

bool
graph_proxy_dict_entry_get(
  graph_proxy_handle graph,
  uint32_t object_type,
  uint64_t object_id,
  const char * key,
  char ** value_ptr_ptr)
{
  DBusMessage * reply_ptr;
  const char * reply_signature;
  DBusMessageIter iter;
  const char * cvalue_ptr;
  char * value_ptr;

  if (!graph_ptr->graph_dict_supported)
  {
    return false;
  }

  if (!dbus_call(0, graph_ptr->service, graph_ptr->object, IFACE_GRAPH_DICT, "Get", "uts", &object_type, &object_id, &key, NULL, &reply_ptr))
  {
    log_error(IFACE_GRAPH_DICT ".Get() failed.");
    return false;
  }

  reply_signature = dbus_message_get_signature(reply_ptr);

  if (strcmp(reply_signature, "s") != 0)
  {
    log_error("reply signature is '%s' but expected signature is 's'", reply_signature);
    dbus_message_unref(reply_ptr);
    return false;
  }

  dbus_message_iter_init(reply_ptr, &iter);
  dbus_message_iter_get_basic(&iter, &cvalue_ptr);
  value_ptr = strdup(cvalue_ptr);
  dbus_message_unref(reply_ptr);
  if (value_ptr == NULL)
  {
    log_error("strdup() failed for dict value");
    return false;
  }
  *value_ptr_ptr = value_ptr;
  return true;
}

bool
graph_proxy_dict_entry_drop(
  graph_proxy_handle graph,
  uint32_t object_type,
  uint64_t object_id,
  const char * key)
{
  if (!graph_ptr->graph_dict_supported)
  {
    return false;
  }

  if (!dbus_call(0, graph_ptr->service, graph_ptr->object, IFACE_GRAPH_DICT, "Drop", "uts", &object_type, &object_id, &key, ""))
  {
    log_error(IFACE_GRAPH_DICT ".Drop() failed.");
    return false;
  }

  return true;
}

bool graph_proxy_get_client_pid(graph_proxy_handle graph, uint64_t client_id, pid_t * pid_ptr)
{
  int64_t pid;

  if (!dbus_call(0, graph_ptr->service, graph_ptr->object, JACKDBUS_IFACE_PATCHBAY, "GetClientPID", "t", &client_id, "x", &pid))
  {
    log_error("GetClientPID() failed.");
    return false;
  }

  *pid_ptr = pid;

  return true;
}

bool
graph_proxy_split(
  graph_proxy_handle graph,
  uint64_t client_id)
{
  if (!graph_ptr->graph_manager_supported)
  {
    return false;
  }

  if (!dbus_call(0, graph_ptr->service, graph_ptr->object, IFACE_GRAPH_MANAGER, "Split", "t", &client_id, ""))
  {
    log_error(IFACE_GRAPH_MANAGER ".Split() failed.");
    return false;
  }

  return true;
}

bool
graph_proxy_join(
  graph_proxy_handle graph,
  uint64_t client1_id,
  uint64_t client2_id)
{
  if (!graph_ptr->graph_manager_supported)
  {
    return false;
  }

  if (!dbus_call(0, graph_ptr->service, graph_ptr->object, IFACE_GRAPH_MANAGER, "Join", "tt", &client1_id, &client2_id, ""))
  {
    log_error(IFACE_GRAPH_MANAGER ".Join() failed.");
    return false;
  }

  return true;
}

bool
graph_proxy_rename_client(
  graph_proxy_handle graph,
  uint64_t client_id,
  const char * newname)
{
  if (!graph_ptr->graph_manager_supported)
  {
    return false;
  }

  if (!dbus_call(0, graph_ptr->service, graph_ptr->object, IFACE_GRAPH_MANAGER, "RenameClient", "ts", &client_id, &newname, ""))
  {
    log_error(IFACE_GRAPH_MANAGER ".RenameClient() failed.");
    return false;
  }

  return true;
}

bool
graph_proxy_rename_port(
  graph_proxy_handle graph,
  uint64_t port_id,
  const char * newname)
{
  if (!graph_ptr->graph_manager_supported)
  {
    return false;
  }

  if (!dbus_call(0, graph_ptr->service, graph_ptr->object, IFACE_GRAPH_MANAGER, "RenamePort", "ts", &port_id, &newname, ""))
  {
    log_error(IFACE_GRAPH_MANAGER ".RenamePort() failed.");
    return false;
  }

  return true;
}

bool
graph_proxy_move_port(
  graph_proxy_handle graph,
  uint64_t port_id,
  uint64_t client_id)
{
  if (!graph_ptr->graph_manager_supported)
  {
    return false;
  }

  if (!dbus_call(0, graph_ptr->service, graph_ptr->object, IFACE_GRAPH_MANAGER, "MovePort", "tt", &port_id, &client_id, ""))
  {
    log_error(IFACE_GRAPH_MANAGER ".MovePort() failed.");
    return false;
  }

  return true;
}

bool
graph_proxy_new_client(
  graph_proxy_handle graph,
  const char * name,
  uint64_t * client_id_ptr)
{
  if (!graph_ptr->graph_manager_supported)
  {
    return false;
  }

  if (!dbus_call(0, graph_ptr->service, graph_ptr->object, IFACE_GRAPH_MANAGER, "NewClient", "s", &name, "t", client_id_ptr))
  {
    log_error(IFACE_GRAPH_MANAGER ".NewClient() failed.");
    return false;
  }

  return true;
}

/* this must be static because it is referenced by the
 * dbus helper layer when hooks are active */
static struct dbus_signal_hook g_signal_hooks[] =
{
  {"ClientAppeared", on_client_appeared},
  {"ClientRenamed", on_client_renamed},
  {"ClientDisappeared", on_client_disappeared},
  {"PortAppeared", on_port_appeared},
  {"PortRenamed", on_port_renamed},
  {"PortDisappeared", on_port_disappeared},
  {"PortsConnected", on_ports_connected},
  {"PortsDisconnected", on_ports_disconnected},
  {NULL, NULL}
};
