/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface of the graph view object
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

#ifndef GRAPH_VIEW_H__05B5CE46_5239_43F1_9F31_79F13EBF0DFA__INCLUDED
#define GRAPH_VIEW_H__05B5CE46_5239_43F1_9F31_79F13EBF0DFA__INCLUDED

#include "graph_canvas.h"

typedef struct graph_view_tag { int unused; } * graph_view_handle;

void view_init(void);

bool
create_view(
  const char * name,
  const char * service,
  const char * object,
  bool graph_dict_supported,
  bool force_activate,
  graph_view_handle * handle_ptr);

void destroy_view(graph_view_handle view);
void activate_view(graph_view_handle view);
const char * get_view_name(graph_view_handle view);
bool set_view_name(graph_view_handle view, const char * name);
canvas_handle get_current_canvas();

/* not very good place for this prototype, because it is not implemented in graph_view.c */
void set_main_window_title(graph_view_handle view);

#endif /* #ifndef GRAPH_VIEW_H__05B5CE46_5239_43F1_9F31_79F13EBF0DFA__INCLUDED */
