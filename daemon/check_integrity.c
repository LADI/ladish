/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the code that checks data integrity
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

#include <unistd.h>             /* usleep() */
#include "studio.h"
#include "../proxies/notify_proxy.h"

struct ladish_check_vgraph_integrity_context
{
  ladish_graph_handle jack_graph;
};

static void ladish_check_integrity_fail(const char * message)
{
  log_error("Integirity check failed: %s", message);
  ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "ladish daemon integrity check failed", LADISH_CHECK_LOG_TEXT);
  usleep(3 * 1000000);
  ASSERT_NO_PASS;
}

#define ctx_ptr ((struct ladish_check_vgraph_integrity_context *)context)

bool
ladish_check_vgraph_integrity_client_begin_callback(
  void * context,
  ladish_graph_handle graph_handle,
  ladish_client_handle client_handle,
  const char * client_name,
  void ** client_iteration_context_ptr_ptr)
{
  return true;
}

bool
ladish_check_vgraph_integrity_port_callback(
  void * context,
  ladish_graph_handle vgraph,
  void * client_iteration_context_ptr,
  ladish_client_handle client_handle,
  const char * client_name,
  ladish_port_handle vport,
  const char * port_name,
  uint32_t port_type,
  uint32_t port_flags)
{
  uuid_t uuid;
  ladish_port_handle jport;
  char uuid_str[37];
  bool link;

  ladish_port_get_uuid(vport, uuid);
  uuid_unparse(uuid, uuid_str);

  link = ladish_port_is_link(vport);
  if (link)
  {
    return true;
  }

  jport = ladish_graph_find_port_by_uuid(ctx_ptr->jack_graph, uuid, false, vgraph);
  if (jport == NULL)
  {
    log_error("vgraph: %s", ladish_graph_get_description(vgraph));
    log_error("client name: %s", client_name);
    log_error("port name: %s", port_name);
    log_error("port uuid: %s", uuid_str);
    log_error("port ptr: %p", vport);
    log_error("port type: 0x%"PRIX32, port_type);
    log_error("port flags: 0x%"PRIX32, port_flags);
    ladish_check_integrity_fail("vgraph port not found in JACK graph.");
  }

  return true;
}

bool
ladish_check_vgraph_integrity_client_end_callback(
    void * context,
    ladish_graph_handle graph_handle,
    ladish_client_handle client_handle,
    const char * client_name,
    void * client_iteration_context_ptr)
{
  return true;
}

#undef ctx_ptr

bool ladish_check_vgraph_integrity(void * context, ladish_graph_handle graph, ladish_app_supervisor_handle app_supervisor)
{
  ladish_graph_iterate_nodes(
    graph,
    true,
    context,
    ladish_check_vgraph_integrity_client_begin_callback,
    ladish_check_vgraph_integrity_port_callback,
    ladish_check_vgraph_integrity_client_end_callback);

  return true;
}

void ladish_check_integrity(void)
{
  struct ladish_check_vgraph_integrity_context ctx;

  //ladish_check_integrity_fail("test");

  if (!ladish_studio_is_loaded())
  {
    return;
  }

  ctx.jack_graph = ladish_studio_get_jack_graph();

  ladish_studio_iterate_virtual_graphs(&ctx, ladish_check_vgraph_integrity);
}
