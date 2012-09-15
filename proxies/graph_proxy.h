/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009,2010,2011,2012 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to graph object that is backed through D-Bus
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

#ifndef GRAPH_PROXY_H__61D1ED56_E33B_4F50_B45B_F520979E8AA7__INCLUDED
#define GRAPH_PROXY_H__61D1ED56_E33B_4F50_B45B_F520979E8AA7__INCLUDED

#include "common.h"

typedef struct graph_proxy_tag { int unused; } * graph_proxy_handle;

#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /* Adjust editor indent */
#endif

bool
graph_proxy_create(
  const char * service,
  const char * object,
  bool graph_dict_supported,
  bool graph_manager_supported,
  graph_proxy_handle * graph_proxy_ptr);

void
graph_proxy_destroy(
  graph_proxy_handle graph);

const char * graph_proxy_get_service(graph_proxy_handle graph);
const char * graph_proxy_get_object(graph_proxy_handle graph);

bool
graph_proxy_activate(
  graph_proxy_handle graph);

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
  void (* ports_disconnected)(void * context, uint64_t client1_id, uint64_t port1_id, uint64_t client2_id, uint64_t port2_id));

void
graph_proxy_detach(
  graph_proxy_handle graph,
  void * context);

bool
graph_proxy_connect_ports(
  graph_proxy_handle graph,
  uint64_t port1_id,
  uint64_t port2_id);

bool
graph_proxy_disconnect_ports(
  graph_proxy_handle graph,
  uint64_t port1_id,
  uint64_t port2_id);

bool
graph_proxy_dict_entry_set(
  graph_proxy_handle graph,
  uint32_t object_type,
  uint64_t object_id,
  const char * key,
  const char * value);

bool
graph_proxy_dict_entry_get(
  graph_proxy_handle graph,
  uint32_t object_type,
  uint64_t object_id,
  const char * key,
  char ** value);

bool
graph_proxy_dict_entry_drop(
  graph_proxy_handle graph,
  uint32_t object_type,
  uint64_t object_id,
  const char * key);

bool graph_proxy_get_client_pid(graph_proxy_handle graph, uint64_t client_id, pid_t * pid_ptr);
bool graph_proxy_get_client_original_name(graph_proxy_handle graph, uint64_t client_id, char ** name);

bool
graph_proxy_split(
  graph_proxy_handle graph,
  uint64_t client_id);

bool
graph_proxy_join(
  graph_proxy_handle graph,
  uint64_t client1_id,
  uint64_t client2_id);

bool
graph_proxy_rename_client(
  graph_proxy_handle graph,
  uint64_t client_id,
  const char * newname);

bool
graph_proxy_rename_port(
  graph_proxy_handle graph,
  uint64_t port_id,
  const char * newname);

bool
graph_proxy_move_port(
  graph_proxy_handle graph,
  uint64_t port_id,
  uint64_t client_id);

bool
graph_proxy_new_client(
  graph_proxy_handle graph,
  const char * name,
  uint64_t * client_id_ptr);

bool
graph_proxy_remove_client(
  graph_proxy_handle graph,
  uint64_t client_id);

#if 0
{ /* Adjust editor indent */
#endif
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* #ifndef GRAPH_PROXY_H__61D1ED56_E33B_4F50_B45B_F520979E8AA7__INCLUDED */
