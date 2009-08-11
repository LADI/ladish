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

#include "graph_canvas.h"

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
  return false;
}

void
graph_canvas_detach(
  graph_canvas_handle graph_canvas)
{
}

canvas_handle
graph_canvas_get_canvas(
  graph_canvas_handle graph_canvas)
{
  return graph_canvas_ptr->canvas;
}
