/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to graph canvas object
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

#ifndef GRAPH_CANVAS_H__F145C6FA_633C_4E64_9117_ED301618B587__INCLUDED
#define GRAPH_CANVAS_H__F145C6FA_633C_4E64_9117_ED301618B587__INCLUDED

#include "../graph_proxy.h"
#include "canvas.h"

typedef struct graph_canvas_tag { int unused; } * graph_canvas_handle;

#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* Adjust editor indent */
#endif

bool
graph_canvas_create(
  int width,
  int height,
  graph_canvas_handle * graph_canvas_ptr);

void
graph_canvas_destroy(
  graph_canvas_handle graph_canvas);

bool
graph_canvas_attach(
  graph_canvas_handle graph_canvas,
  graph_handle graph);

void
graph_canvas_detach(
  graph_canvas_handle graph_canvas);

canvas_handle
graph_canvas_get_canvas(
  graph_canvas_handle graph_canvas);

#if 0
{ /* Adjust editor indent */
#endif
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* #ifndef GRAPH_CANVAS_H__F145C6FA_633C_4E64_9117_ED301618B587__INCLUDED */
