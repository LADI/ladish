/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to the D-Bus patchbay interface helpers
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

#ifndef PATCHBAY_H__30334B9A_8847_4E8C_AFF9_73DB13406C8E__INCLUDED
#define PATCHBAY_H__30334B9A_8847_4E8C_AFF9_73DB13406C8E__INCLUDED

#include "client.h"
#include "port.h"

typedef struct ladish_graph_tag { int unused; } * ladish_graph_handle;

bool ladish_graph_create(ladish_graph_handle * graph_handle_ptr, const char * opath);
void ladish_graph_destroy(ladish_graph_handle graph_handle);
void ladish_graph_clear(ladish_graph_handle graph_handle);
void * ladish_graph_get_dbus_context(ladish_graph_handle graph_handle);
ladish_dict_handle ladish_graph_get_dict(ladish_graph_handle graph_handle);
bool ladish_graph_add_client(ladish_graph_handle graph_handle, ladish_client_handle client_handle, const char * name);

void
ladish_graph_remove_client(
  ladish_graph_handle graph_handle,
  ladish_client_handle client_handle,
  bool destroy_ports);

bool
ladish_graph_add_port(
  ladish_graph_handle graph_handle,
  ladish_client_handle client_handle,
  ladish_port_handle port_handle,
  const char * name,
  uint32_t type,
  uint32_t flags);

void
ladish_graph_remove_port(
  ladish_graph_handle graph_handle,
  ladish_client_handle client_handle,
  ladish_port_handle port_handle);

ladish_client_handle ladish_graph_find_client_by_id(ladish_graph_handle graph_handle, uint64_t client_id);
ladish_port_handle ladish_graph_find_port_by_id(ladish_graph_handle graph_handle, uint64_t port_id);

bool
ladish_graph_iterate_nodes(
  ladish_graph_handle graph_handle,
  void * callback_context,
  bool
  (* client_callback)(
    void * context,
    ladish_client_handle client_handle,
    const char * client_name,
    void ** client_iteration_context_ptr_ptr),
  bool
  (* port_callback)(
    void * context,
    void * client_iteration_context_ptr,
    ladish_client_handle client_handle,
    const char * client_name,
    ladish_port_handle port_handle,
    const char * port_name,
    uint32_t port_type,
    uint32_t port_flags));

extern const struct dbus_interface_descriptor g_interface_patchbay;

#endif /* #ifndef PATCHBAY_H__30334B9A_8847_4E8C_AFF9_73DB13406C8E__INCLUDED */
