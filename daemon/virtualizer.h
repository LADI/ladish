/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to graph dispatcher object
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

#ifndef JACK_DISPATCH_H__C7566B66_081D_4D00_A702_7C18F7CC0735__INCLUDED
#define JACK_DISPATCH_H__C7566B66_081D_4D00_A702_7C18F7CC0735__INCLUDED

#include "common.h"
#include "../proxies/graph_proxy.h"
#include "graph.h"

typedef struct ladish_virtualizer { int unused; } * ladish_virtualizer_handle;

bool
ladish_virtualizer_create(
  graph_proxy_handle jack_graph_proxy,
  ladish_graph_handle jack_graph,
  ladish_graph_handle studio_graph,
  ladish_virtualizer_handle * handle_ptr);

unsigned int
ladish_virtualizer_get_our_clients_count(
  ladish_virtualizer_handle handle);

void
ladish_virtualizer_destroy(
  ladish_virtualizer_handle handle);

#endif /* #ifndef JACK_DISPATCH_H__C7566B66_081D_4D00_A702_7C18F7CC0735__INCLUDED */
