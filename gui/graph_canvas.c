/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of graph canvas object
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

#include <locale.h>

#include "graph_canvas.h"
#include "../dbus_constants.h"

struct graph_canvas
{
  graph_proxy_handle graph;
  canvas_handle canvas;
  struct list_head clients;
};

struct client
{
  struct list_head siblings;
  uint64_t id;
  canvas_module_handle canvas_module;
  struct list_head ports;
  struct graph_canvas * owner_ptr;
};

struct port
{
  struct list_head siblings;
  uint64_t id;
  canvas_port_handle canvas_port;
  struct graph_canvas * graph_canvas;
};

static
struct client *
find_client(
  struct graph_canvas * graph_canvas_ptr,
  uint64_t id)
{
  struct list_head * node_ptr;
  struct client * client_ptr;

  list_for_each(node_ptr, &graph_canvas_ptr->clients)
  {
    client_ptr = list_entry(node_ptr, struct client, siblings);
    if (client_ptr->id == id)
    {
      return client_ptr;
    }
  }

  return NULL;
}

static
struct port *
find_port(
  struct client * client_ptr,
  uint64_t id)
{
  struct list_head * node_ptr;
  struct port * port_ptr;

  list_for_each(node_ptr, &client_ptr->ports)
  {
    port_ptr = list_entry(node_ptr, struct port, siblings);
    if (port_ptr->id == id)
    {
      return port_ptr;
    }
  }

  return NULL;
}

#define port1_ptr ((struct port *)port1_context)
#define port2_ptr ((struct port *)port2_context)

static
void
connect_request(
  void * port1_context,
  void * port2_context)
{
  log_info("connect request(%"PRIu64", %"PRIu64")", port1_ptr->id, port2_ptr->id);

  ASSERT(port1_ptr->graph_canvas == port2_ptr->graph_canvas);

  graph_proxy_connect_ports(port1_ptr->graph_canvas->graph, port1_ptr->id, port2_ptr->id);
}

void
disconnect_request(
  void * port1_context,
  void * port2_context)
{
  log_info("disconnect request(%"PRIu64", %"PRIu64")", port1_ptr->id, port2_ptr->id);

  ASSERT(port1_ptr->graph_canvas == port2_ptr->graph_canvas);

  graph_proxy_disconnect_ports(port1_ptr->graph_canvas->graph, port1_ptr->id, port2_ptr->id);
}

#undef port1_ptr
#undef port2_ptr

#define client_ptr ((struct client *)module_context)

void
module_location_changed(
  void * module_context,
  double x,
  double y)
{
  char x_str[100];
  char y_str[100];
  char * locale;

  log_info("module_location_changed(id = %3llu, x = %6.1f, y = %6.1f)", (unsigned long long)client_ptr->id, x, y);

  locale = strdup(setlocale(LC_NUMERIC, NULL));
  if (locale == NULL)
  {
    log_error("strdup() failed for locale string");
    return;
  }

  setlocale(LC_NUMERIC, "POSIX");
  sprintf(x_str, "%f", x);
  sprintf(y_str, "%f", y);
  setlocale(LC_NUMERIC, locale);
  free(locale);

  graph_proxy_dict_entry_set(
    client_ptr->owner_ptr->graph,
    GRAPH_DICT_OBJECT_TYPE_CLIENT,
    client_ptr->id,
    URI_CANVAS_X,
    x_str);

  graph_proxy_dict_entry_set(
    client_ptr->owner_ptr->graph,
    GRAPH_DICT_OBJECT_TYPE_CLIENT,
    client_ptr->id,
    URI_CANVAS_Y,
    y_str);
}

#undef client_ptr

bool
graph_canvas_create(
  int width,
  int height,
  graph_canvas_handle * graph_canvas_handle_ptr)
{
  struct graph_canvas * graph_canvas_ptr;

  graph_canvas_ptr = malloc(sizeof(struct graph_canvas));
  if (graph_canvas_ptr == NULL)
  {
    return false;
  }

  if (!canvas_create(width, height, connect_request, disconnect_request, module_location_changed, &graph_canvas_ptr->canvas))
  {
    free(graph_canvas_ptr);
    return false;
  }

  graph_canvas_ptr->graph = NULL;
  INIT_LIST_HEAD(&graph_canvas_ptr->clients);

  *graph_canvas_handle_ptr = (graph_canvas_handle)graph_canvas_ptr;

  return true;
}

#define graph_canvas_ptr ((struct graph_canvas *)graph_canvas)

static
void
clear(
  void * graph_canvas)
{
  log_info("canvas::clear()");
  canvas_clear(graph_canvas_ptr->canvas);
}

static
void
client_appeared(
  void * graph_canvas,
  uint64_t id,
  const char * name)
{
  struct client * client_ptr;
  char * x_str;
  char * y_str;
  double x;
  double y;
  char * locale;
  double width;
  double height;

  log_info("canvas::client_appeared(%"PRIu64", \"%s\")", id, name);

  canvas_get_size(graph_canvas_ptr->canvas, &width, &height);
  //log_debug("width %f, height %f", width, height);

  client_ptr = malloc(sizeof(struct client));
  if (client_ptr == NULL)
  {
    log_error("allocation of memory for struct client failed");
    return;
  }

  client_ptr->id = id;
  INIT_LIST_HEAD(&client_ptr->ports);
  client_ptr->owner_ptr = graph_canvas_ptr;

  x = 0;
  y = 0;

  if (!graph_proxy_dict_entry_get(
        client_ptr->owner_ptr->graph,
        GRAPH_DICT_OBJECT_TYPE_CLIENT,
        id,
        URI_CANVAS_X,
        &x_str))
  {
    x_str = NULL;
    x = width / 2 - 200 + rand() % 300;
  }

  if (!graph_proxy_dict_entry_get(
        client_ptr->owner_ptr->graph,
        GRAPH_DICT_OBJECT_TYPE_CLIENT,
        id,
        URI_CANVAS_Y,
        &y_str))
  {
    y_str = NULL;
    y = height / 2 - 200 + rand() % 300;
  }

  if (x_str != NULL || y_str != NULL)
  {
    locale = strdup(setlocale(LC_NUMERIC, NULL));
    if (locale == NULL)
    {
      log_error("strdup() failed for locale string");
    }
    else
    {
      setlocale(LC_NUMERIC, "POSIX");
      if (x_str != NULL)
      {
        //log_info("converting %s", x_str);
        sscanf(x_str, "%lf", &x);
      }
      if (y_str != NULL)
      {
        //log_info("converting %s", y_str);
        sscanf(y_str, "%lf", &y);
      }
      setlocale(LC_NUMERIC, locale);
      free(locale);
    }

    if (x_str != NULL)
    {
      free(x_str);
    }

    if (y_str != NULL)
    {
      free(y_str);
    }
  }

  if (!canvas_create_module(graph_canvas_ptr->canvas, name, x, y, true, true, client_ptr, &client_ptr->canvas_module))
  {
    log_error("canvas_create_module(\"%s\") failed", name);
    free(client_ptr);
    return;
  }

  if (x_str == NULL || y_str == NULL)
  { /* we have generated random value, store it */
    module_location_changed(client_ptr, x, y);
  }

  list_add_tail(&client_ptr->siblings, &graph_canvas_ptr->clients);
}

static
void
client_disappeared(
  void * graph_canvas,
  uint64_t id)
{
  struct client * client_ptr;

  log_info("canvas::client_disappeared(%"PRIu64")", id);

  client_ptr = find_client(graph_canvas_ptr, id);
  if (client_ptr == NULL)
  {
    log_error("cannot find disappearing client %"PRIu64"", id);
    return;
  }

  list_del(&client_ptr->siblings);
  canvas_destroy_module(graph_canvas_ptr->canvas, client_ptr->canvas_module);
  free(client_ptr);
}

static void client_renamed(void * graph_canvas, uint64_t id, const char * old_name, const char * new_name)
{
  struct client * client_ptr;

  log_info("canvas::client_renamed(%"PRIu64", '%s', '%s')", id, old_name, new_name);

  client_ptr = find_client(graph_canvas_ptr, id);
  if (client_ptr == NULL)
  {
    log_error("cannot find renamed client %"PRIu64, id);
    return;
  }

  canvas_set_module_name(client_ptr->canvas_module, new_name);
}

static
void
port_appeared(
  void * graph_canvas,
  uint64_t client_id,
  uint64_t port_id,
  const char * port_name,
  bool is_input,
  bool is_terminal,
  bool is_midi)
{
  int color;
  struct client * client_ptr;
  struct port * port_ptr;

  log_info("canvas::port_appeared(%"PRIu64", %"PRIu64", \"%s\")", client_id, port_id, port_name);

  client_ptr = find_client(graph_canvas_ptr, client_id);
  if (client_ptr == NULL)
  {
    log_error("cannot find client %"PRIu64" of appearing port %"PRIu64" \"%s\"", client_id, port_id, port_name);
    return;
  }

  port_ptr = malloc(sizeof(struct port));
  if (port_ptr == NULL)
  {
    log_error("allocation of memory for struct port failed");
    return;
  }

  port_ptr->id = port_id;
  port_ptr->graph_canvas = graph_canvas_ptr;

  // Darkest tango palette colour, with S -= 6, V -= 6, w/ transparency
  if (is_midi)
  {
    color = 0x960909C0;
  }
  else
  {
    color = 0x244678C0;
  }

  if (!canvas_create_port(graph_canvas_ptr->canvas, client_ptr->canvas_module, port_name, is_input, color, port_ptr, &port_ptr->canvas_port))
  {
    log_error("canvas_create_port(\"%s\") failed", port_name);
    free(client_ptr);
    return;
  }

  list_add_tail(&port_ptr->siblings, &client_ptr->ports);
}

static
void
port_disappeared(
  void * graph_canvas,
  uint64_t client_id,
  uint64_t port_id)
{
  struct client * client_ptr;
  struct port * port_ptr;

  log_info("canvas::port_disappeared(%"PRIu64", %"PRIu64")", client_id, port_id);

  client_ptr = find_client(graph_canvas_ptr, client_id);
  if (client_ptr == NULL)
  {
    log_error("cannot find client %"PRIu64" of disappearing port %"PRIu64"", client_id, port_id);
    return;
  }

  port_ptr = find_port(client_ptr, port_id);
  if (client_ptr == NULL)
  {
    log_error("cannot find disappearing port %"PRIu64" of client %"PRIu64"", port_id, client_id);
    return;
  }

  list_del(&port_ptr->siblings);
  canvas_destroy_port(graph_canvas_ptr->canvas, port_ptr->canvas_port);
  free(port_ptr);
}

static
void
port_renamed(
  void * graph_canvas,
  uint64_t client_id,
  uint64_t port_id,
  const char * old_port_name,
  const char * new_port_name)
{
  struct client * client_ptr;
  struct port * port_ptr;

  log_info("canvas::port_renamed(%"PRIu64", %"PRIu64", '%s', '%s')", client_id, port_id, old_port_name, new_port_name);

  client_ptr = find_client(graph_canvas_ptr, client_id);
  if (client_ptr == NULL)
  {
    log_error("cannot find client %"PRIu64" of renamed port %"PRIu64"", client_id, port_id);
    return;
  }

  port_ptr = find_port(client_ptr, port_id);
  if (client_ptr == NULL)
  {
    log_error("cannot find renamed port %"PRIu64" of client %"PRIu64"", port_id, client_id);
    return;
  }

  canvas_set_port_name(port_ptr->canvas_port, new_port_name);
}

static
void
ports_connected(
  void * graph_canvas,
  uint64_t client1_id,
  uint64_t port1_id,
  uint64_t client2_id,
  uint64_t port2_id)
{
  struct client * client1_ptr;
  struct port * port1_ptr;
  struct client * client2_ptr;
  struct port * port2_ptr;

  log_info("canvas::ports_connected(%"PRIu64", %"PRIu64", %"PRIu64", %"PRIu64")", client1_id, port1_id, client2_id, port2_id);

  client1_ptr = find_client(graph_canvas_ptr, client1_id);
  if (client1_ptr == NULL)
  {
    log_error("cannot find client %"PRIu64" of connected port %"PRIu64"", client1_id, port1_id);
    return;
  }

  port1_ptr = find_port(client1_ptr, port1_id);
  if (client1_ptr == NULL)
  {
    log_error("cannot find connected port %"PRIu64" of client %"PRIu64"", port1_id, client1_id);
    return;
  }

  client2_ptr = find_client(graph_canvas_ptr, client2_id);
  if (client2_ptr == NULL)
  {
    log_error("cannot find client %"PRIu64" of connected port %"PRIu64"", client2_id, port2_id);
    return;
  }

  port2_ptr = find_port(client2_ptr, port2_id);
  if (client2_ptr == NULL)
  {
    log_error("cannot find connected port %"PRIu64" of client %"PRIu64"", port2_id, client2_id);
    return;
  }

  canvas_add_connection(
    graph_canvas_ptr->canvas,
    port1_ptr->canvas_port,
    port2_ptr->canvas_port,
    canvas_get_port_color(port1_ptr->canvas_port) + 0x22222200);
}

static
void
ports_disconnected(
  void * graph_canvas,
  uint64_t client1_id,
  uint64_t port1_id,
  uint64_t client2_id,
  uint64_t port2_id)
{
  struct client * client1_ptr;
  struct port * port1_ptr;
  struct client * client2_ptr;
  struct port * port2_ptr;

  log_info("canvas::ports_disconnected(%"PRIu64", %"PRIu64", %"PRIu64", %"PRIu64")", client1_id, port1_id, client2_id, port2_id);

  client1_ptr = find_client(graph_canvas_ptr, client1_id);
  if (client1_ptr == NULL)
  {
    log_error("cannot find client %"PRIu64" of disconnected port %"PRIu64"", client1_id, port1_id);
    return;
  }

  port1_ptr = find_port(client1_ptr, port1_id);
  if (client1_ptr == NULL)
  {
    log_error("cannot find disconnected port %"PRIu64" of client %"PRIu64"", port1_id, client1_id);
    return;
  }

  client2_ptr = find_client(graph_canvas_ptr, client2_id);
  if (client2_ptr == NULL)
  {
    log_error("cannot find client %"PRIu64" of disconnected port %"PRIu64"", client2_id, port2_id);
    return;
  }

  port2_ptr = find_port(client2_ptr, port2_id);
  if (client2_ptr == NULL)
  {
    log_error("cannot find disconnected port %"PRIu64" of client %"PRIu64"", port2_id, client2_id);
    return;
  }

  canvas_remove_connection(
    graph_canvas_ptr->canvas,
    port1_ptr->canvas_port,
    port2_ptr->canvas_port);
}

void
graph_canvas_destroy(
  graph_canvas_handle graph_canvas)
{
  if (graph_canvas_ptr->graph != NULL)
  {
    graph_canvas_detach(graph_canvas);
  }

  free(graph_canvas_ptr);
}

bool
graph_canvas_attach(
  graph_canvas_handle graph_canvas,
  graph_proxy_handle graph)
{
  ASSERT(graph_canvas_ptr->graph == NULL);

  if (!graph_proxy_attach(
        graph,
        graph_canvas,
        clear,
        client_appeared,
        client_renamed,
        client_disappeared,
        port_appeared,
        port_renamed,
        port_disappeared,
        ports_connected,
        ports_disconnected))
  {
    return false;
  }

  graph_canvas_ptr->graph = graph;

  return true;
}

void
graph_canvas_detach(
  graph_canvas_handle graph_canvas)
{
  ASSERT(graph_canvas_ptr->graph != NULL);
  graph_proxy_detach(graph_canvas_ptr->graph, graph_canvas);
  graph_canvas_ptr->graph = NULL;
}

canvas_handle
graph_canvas_get_canvas(
  graph_canvas_handle graph_canvas)
{
  return graph_canvas_ptr->canvas;
}
