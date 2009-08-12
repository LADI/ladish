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

#include <stdlib.h>
#include <inttypes.h>

#include "graph_canvas.h"
#include "../common/debug.h"

struct graph_canvas
{
  graph_handle graph;
  canvas_handle canvas;
};

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

  if (!canvas_create(width, height, &graph_canvas_ptr->canvas))
  {
    free(graph_canvas_ptr);
    return false;
  }

  graph_canvas_ptr->graph = NULL;

  *graph_canvas_handle_ptr = (graph_canvas_handle)graph_canvas_ptr;

  return true;
}

#define graph_canvas_ptr ((struct graph_canvas *)graph_canvas)

static
void
clear(
  void * graph_canvas)
{
  lash_info("canvas::clear()");
}

static
void
client_appeared(
  void * graph_canvas,
  uint64_t id,
  const char * name)
{
  lash_info("canvas::client_appeared(%"PRIu64", \"%s\")", id, name);
}

static
void
client_disappeared(
  void * graph_canvas,
  uint64_t id)
{
  lash_info("canvas::client_disappeared(%"PRIu64")", id);
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
  lash_info("canvas::port_appeared(%"PRIu64", %"PRIu64", \"%s\")", client_id, port_id, port_name);
}

static
void
port_disappeared(
  void * graph_canvas,
  uint64_t client_id,
  uint64_t port_id)
{
  lash_info("canvas::port_disappeared(%"PRIu64", %"PRIu64")", client_id, port_id);
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
  lash_info("canvas::ports_connected(%"PRIu64", %"PRIu64", %"PRIu64", %"PRIu64")", client1_id, port1_id, client2_id, port2_id);
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
  lash_info("canvas::ports_disconnected(%"PRIu64", %"PRIu64", %"PRIu64", %"PRIu64")", client1_id, port1_id, client2_id, port2_id);
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
  graph_handle graph)
{
  assert(graph_canvas_ptr->graph == NULL);

  if (!graph_attach(
        graph,
        graph_canvas,
        clear,
        client_appeared,
        client_disappeared,
        port_appeared,
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
  assert(graph_canvas_ptr->graph != NULL);
  graph_detach(graph_canvas_ptr->graph, graph_canvas);
  graph_canvas_ptr->graph = NULL;
}

canvas_handle
graph_canvas_get_canvas(
  graph_canvas_handle graph_canvas)
{
  return graph_canvas_ptr->canvas;
}
