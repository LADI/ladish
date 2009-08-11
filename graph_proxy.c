/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
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

#include <dbus/dbus.h>
#include <stdlib.h>
#include <assert.h>

#include "common.h"
#include "graph_proxy.h"
#include "common/klist.h"
#include "common/debug.h"
#include "dbus/helpers.h"

#define JACKDBUS_IFACE_PATCHBAY  "org.jackaudio.JackPatchbay"

#define JACKDBUS_PORT_FLAG_INPUT         0x00000001
#define JACKDBUS_PORT_FLAG_OUTPUT        0x00000002
#define JACKDBUS_PORT_FLAG_PHYSICAL      0x00000004
#define JACKDBUS_PORT_FLAG_CAN_MONITOR   0x00000008
#define JACKDBUS_PORT_FLAG_TERMINAL      0x00000010

#define JACKDBUS_PORT_TYPE_AUDIO    0
#define JACKDBUS_PORT_TYPE_MIDI     1

struct monitor
{
  struct list_head siblings;
  void * context;
  void (* clear)(void * context);
  void (* client_appeared)(void * context, uint64_t id, const char * name);
  void (* client_disappeared)(void * context, uint64_t id);
  void (* port_appeared)(void * context, uint64_t client_id, uint64_t port_id, const char * port_name, bool is_input, bool is_terminal, bool is_midi);
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
};

DBusHandlerResult message_hook(DBusConnection *, DBusMessage *, void *);

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
    lash_error("Unknown JACK D-Bus port type %d", (unsigned int)port_type);
    return;
  }

  is_input = port_flags & JACKDBUS_PORT_FLAG_INPUT;
  is_terminal = port_flags & JACKDBUS_PORT_FLAG_TERMINAL;
  is_midi = port_flags == JACKDBUS_PORT_TYPE_MIDI;

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

  if (force)
  {
    version = 0; // workaround module split/join stupidity
  }
  else
  {
    version = graph_ptr->version;
  }

  if (!dbus_call_simple(graph_ptr->service, graph_ptr->object, JACKDBUS_IFACE_PATCHBAY, "GetGraph", "t", &version, NULL, &reply_ptr))
  {
    lash_error("GetGraph() failed.");
    return;
  }

  reply_signature = dbus_message_get_signature(reply_ptr);

  if (strcmp(reply_signature, "ta(tsa(tsuu))a(tstststst)") != 0)
  {
    lash_error("GetGraph() reply signature mismatch. '%s'", reply_signature);
    goto unref;
  }

  dbus_message_iter_init(reply_ptr, &iter);

  //info_msg((std::string)"version " + (char)dbus_message_iter_get_arg_type(&iter));
  dbus_message_iter_get_basic(&iter, &version);
  dbus_message_iter_next(&iter);

  if (!force && version <= graph_ptr->version)
  {
    goto unref;
  }

  clear(graph_ptr);

  //info_msg(str(boost::format("got new graph version %llu") % version));
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
graph_create(
  const char * service,
  const char * object,
  graph_handle * graph_handle_ptr)
{
  struct graph * graph_ptr;
  char rule[1024];
  const char ** signal;

  const char * patchbay_signals[] = {
    "ClientAppeared",
    "ClientDisappeared",
    "PortAppeared",
    "PortDisappeared",
    "PortsConnected",
    "PortsDisconnected",
    NULL};

  graph_ptr = malloc(sizeof(struct graph));
  if (graph_ptr == NULL)
  {
    lash_error("malloc() failed to allocate struct graph");
    goto fail;
  }

  graph_ptr->service = strdup(service);
  if (graph_ptr->service == NULL)
  {
    lash_error("strdup() failed too duplicate service name '%s'", service);
    goto free_graph;
  }

  graph_ptr->object = strdup(object);
  if (graph_ptr->object == NULL)
  {
    lash_error("strdup() failed too duplicate object name '%s'", object);
    goto free_service;
  }

  INIT_LIST_HEAD(&graph_ptr->monitors);

  graph_ptr->version = 0;

  for (signal = patchbay_signals; *signal != NULL; signal++)
  {
    snprintf(
      rule,
      sizeof(rule),
      "type='signal',sender='%s',path='%s',interface='" JACKDBUS_IFACE_PATCHBAY "',member='%s'",
      service,
      object,
      *signal);

    dbus_bus_add_match(g_dbus_connection, rule, &g_dbus_error);
    if (dbus_error_is_set(&g_dbus_error))
    {
      lash_error("Failed to add D-Bus match rule: %s", g_dbus_error.message);
      dbus_error_free(&g_dbus_error);
      return false;
    }
  }

  dbus_connection_add_filter(g_dbus_connection, message_hook, graph_ptr, NULL);

  refresh_internal(graph_ptr, true);

  *graph_handle_ptr = (graph_handle)graph_ptr;

  return true;

free_service:
  free(graph_ptr->service);

free_graph:
  free(graph_ptr);

fail:
  return false;
}

#define graph_ptr ((struct graph *)graph)

void
graph_destroy(
  graph_handle graph)
{
  assert(list_empty(&graph_ptr->monitors));

  free(graph_ptr->object);
  free(graph_ptr->service);
  free(graph_ptr);
}

bool
graph_attach(
  graph_handle graph,
  void * context,
  void (* clear)(void * context),
  void (* client_appeared)(void * context, uint64_t id, const char * name),
  void (* client_disappeared)(void * context, uint64_t id),
  void (* port_appeared)(void * context, uint64_t client_id, uint64_t port_id, const char * port_name, bool is_input, bool is_terminal, bool is_midi),
  void (* port_disappeared)(void * context, uint64_t client_id, uint64_t port_id),
  void (* ports_connected)(void * context, uint64_t client1_id, uint64_t port1_id, uint64_t client2_id, uint64_t port2_id),
  void (* ports_disconnected)(void * context, uint64_t client1_id, uint64_t port1_id, uint64_t client2_id, uint64_t port2_id))
{
  struct monitor * monitor_ptr;

  monitor_ptr = malloc(sizeof(struct monitor));
  if (monitor_ptr == NULL)
  {
    lash_error("malloc() failed to allocate struct monitor");
    return false;
  }

  monitor_ptr->context = context;
  monitor_ptr->clear = clear;
  monitor_ptr->client_appeared = client_appeared;
  monitor_ptr->client_disappeared = client_disappeared;
  monitor_ptr->port_appeared = port_appeared;
  monitor_ptr->port_disappeared = port_disappeared;
  monitor_ptr->ports_connected = ports_connected;
  monitor_ptr->ports_disconnected = ports_disconnected;

  list_add_tail(&monitor_ptr->siblings, &graph_ptr->monitors);

  return true;
}

void
graph_detach(
  graph_handle graph,
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

  assert(false);
}

void
graph_connect_ports(
  graph_handle graph,
  uint64_t port1_id,
  uint64_t port2_id)
{
  if (!dbus_call_simple(graph_ptr->service, graph_ptr->object, JACKDBUS_IFACE_PATCHBAY, "ConnectPortsByID", "tt", &port1_id, &port2_id, ""))
  {
    lash_error("ConnectPortsByID() failed.");
  }
}

void
graph_disconnect_ports(
  graph_handle graph,
  uint64_t port1_id,
  uint64_t port2_id)
{
  if (!dbus_call_simple(graph_ptr->service, graph_ptr->object, JACKDBUS_IFACE_PATCHBAY, "DisconnectPortsByID", "tt", &port1_id, &port2_id, ""))
  {
    lash_error("DisconnectPortsByID() failed.");
  }
}

DBusHandlerResult
message_hook(
  DBusConnection * connection,
  DBusMessage * message,
  void * graph)
{
  const char * object_path;
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

  object_path = dbus_message_get_path(message);
  if (object_path == NULL || strcmp(object_path, graph_ptr->object) != 0)
  {
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  if (dbus_message_is_signal(message, JACKDBUS_IFACE_PATCHBAY, "ClientAppeared"))
  {
    if (!dbus_message_get_args(
          message,
          &g_dbus_error,
          DBUS_TYPE_UINT64, &new_graph_version,
          DBUS_TYPE_UINT64, &client_id,
          DBUS_TYPE_STRING, &client_name,
          DBUS_TYPE_INVALID))
    {
      lash_error("dbus_message_get_args() failed to extract ClientAppeared signal arguments (%s)", g_dbus_error.message);
      dbus_error_free(&g_dbus_error);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    lash_info("ClientAppeared, %s(%llu)", client_name, client_id);

    client_appeared(graph_ptr, client_id, client_name);

    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_signal(message, JACKDBUS_IFACE_PATCHBAY, "ClientDisappeared"))
  {
    if (!dbus_message_get_args(
          message,
          &g_dbus_error,
          DBUS_TYPE_UINT64, &new_graph_version,
          DBUS_TYPE_UINT64, &client_id,
          DBUS_TYPE_STRING, &client_name,
          DBUS_TYPE_INVALID))
    {
      lash_error("dbus_message_get_args() failed to extract ClientDisappeared signal arguments (%s)", g_dbus_error.message);
      dbus_error_free(&g_dbus_error);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    lash_info("ClientDisappeared, %s(%llu)", client_name, client_id);

    client_disappeared(graph_ptr, client_id);

    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_signal(message, JACKDBUS_IFACE_PATCHBAY, "PortAppeared"))
  {
    if (!dbus_message_get_args(
          message,
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
      lash_error("dbus_message_get_args() failed to extract PortAppeared signal arguments (%s)", g_dbus_error.message);
      dbus_error_free(&g_dbus_error);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    //me->info_msg(str(boost::format("PortAppeared, %s(%llu):%s(%llu), %lu, %lu") % client_name % client_id % port_name % port_id % port_flags % port_type));

    port_appeared(graph_ptr, client_id, port_id, port_name, port_flags, port_type);

    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_signal(message, JACKDBUS_IFACE_PATCHBAY, "PortDisappeared"))
  {
    if (!dbus_message_get_args(
          message,
          &g_dbus_error,
          DBUS_TYPE_UINT64, &new_graph_version,
          DBUS_TYPE_UINT64, &client_id,
          DBUS_TYPE_STRING, &client_name,
          DBUS_TYPE_UINT64, &port_id,
          DBUS_TYPE_STRING, &port_name,
          DBUS_TYPE_INVALID))
    {
      lash_error("dbus_message_get_args() failed to extract PortDisappeared signal arguments (%s)", g_dbus_error.message);
      dbus_error_free(&g_dbus_error);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    //me->info_msg(str(boost::format("PortDisappeared, %s(%llu):%s(%llu)") % client_name % client_id % port_name % port_id));

    port_disappeared(graph_ptr, client_id, port_id);

    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_signal(message, JACKDBUS_IFACE_PATCHBAY, "PortsConnected"))
  {
    if (!dbus_message_get_args(
          message,
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
      lash_error("dbus_message_get_args() failed to extract PortsConnected signal arguments (%s)", g_dbus_error.message);
      dbus_error_free(&g_dbus_error);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    ports_connected(graph_ptr, client_id, port_id, client2_id, port2_id);

    return DBUS_HANDLER_RESULT_HANDLED;
  }

  if (dbus_message_is_signal(message, JACKDBUS_IFACE_PATCHBAY, "PortsDisconnected"))
  {
    if (!dbus_message_get_args(
          message,
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
      lash_error("dbus_message_get_args() failed to extract PortsConnected signal arguments (%s)", g_dbus_error.message);
      dbus_error_free(&g_dbus_error);
      return DBUS_HANDLER_RESULT_HANDLED;
    }

    ports_disconnected(graph_ptr, client_id, port_id, client2_id, port2_id);

    return DBUS_HANDLER_RESULT_HANDLED;
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
