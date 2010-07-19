/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface of the room object
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

#ifndef ROOM_H__9A1CF253_0A17_402A_BDF8_9BD72B467118__INCLUDED
#define ROOM_H__9A1CF253_0A17_402A_BDF8_9BD72B467118__INCLUDED

#include "common.h"
#include "graph.h"
#include "app_supervisor.h"
#include "virtualizer.h"

typedef struct ladish_room_tag { int unused; } * ladish_room_handle;

bool
ladish_room_create_template(
  const uuid_t uuid_ptr,
  const char * name,
  ladish_room_handle * room_handle_ptr);

bool
ladish_room_create(
  const uuid_t uuid_ptr,
  const char * name,
  ladish_room_handle template,
  ladish_graph_handle owner,
  ladish_room_handle * room_handle_ptr);

void
ladish_room_destroy(
  ladish_room_handle room_handle);

struct list_head * ladish_room_get_list_node(ladish_room_handle room_handle);
ladish_room_handle ladish_room_from_list_node(struct list_head * node_ptr);

const char * ladish_room_get_name(ladish_room_handle room_handle);
const char * ladish_room_get_opath(ladish_room_handle room_handle);
bool ladish_room_get_template_uuid(ladish_room_handle room_handle, uuid_t uuid_ptr);
void ladish_room_get_uuid(ladish_room_handle room_handle, uuid_t uuid_ptr);
ladish_graph_handle ladish_room_get_graph(ladish_room_handle room_handle);
ladish_app_supervisor_handle ladish_room_get_app_supervisor(ladish_room_handle room_handle);

bool
ladish_room_iterate_link_ports(
  ladish_room_handle room_handle,
  void * callback_context,
  bool
  (* callback)(
    void * context,
    ladish_port_handle port_handle,
    const char * port_name,
    uint32_t port_type,
    uint32_t port_flags));

bool ladish_room_start(ladish_room_handle room_handle, ladish_virtualizer_handle virtualizer);
void ladish_room_initiate_stop(ladish_room_handle room_handle, bool clear_persist);
bool ladish_room_stopped(ladish_room_handle room_handle);

#endif /* #ifndef ROOM_H__9A1CF253_0A17_402A_BDF8_9BD72B467118__INCLUDED */
