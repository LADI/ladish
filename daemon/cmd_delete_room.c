/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the "delete room" command
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

#include "cmd.h"
#include "studio_internal.h"
#include "../dbus/error.h"
#include "../proxies/jmcore_proxy.h"

struct ladish_command_delete_room
{
  struct ladish_command command; /* must be the first member */
  char * name;
};

static
bool
uninit_room_ports(
  void * context,
  void * client_iteration_context_ptr,
  ladish_client_handle client_handle,
  const char * client_name,
  ladish_port_handle port_handle,
  const char * port_name,
  uint32_t port_type,
  uint32_t port_flags)
{
  uuid_t uuid_in_room;
  char uuid_in_room_str[37];

  if (ladish_port_is_link(port_handle))
  {
    log_info("link port %s", port_name);

    ladish_graph_get_port_uuid(ladish_room_get_graph(context), port_handle, uuid_in_room);
    uuid_unparse(uuid_in_room, uuid_in_room_str);
    jmcore_proxy_destroy_link(uuid_in_room_str);
  }
  else
  {
    log_info("jack port %s", port_name);
  }

  return true;
}

static void remove_port_callback(ladish_port_handle port)
{
  ladish_client_handle jack_client;

  jack_client = ladish_graph_remove_port(g_studio.jack_graph, port);
  ASSERT(jack_client != NULL);  /* room app port not found in jack graph */
  if (ladish_graph_client_is_empty(g_studio.jack_graph, jack_client))
  {
    ladish_graph_remove_client(g_studio.jack_graph, jack_client);
  }
}

#define cmd_ptr ((struct ladish_command_delete_room *)context)

static bool run(void * context)
{
  struct list_head * node_ptr;
  ladish_room_handle room;
  uuid_t room_uuid;
  ladish_client_handle room_client;
  unsigned int running_app_count;
  ladish_app_supervisor_handle supervisor;
  ladish_graph_handle graph;

  if (cmd_ptr->command.state == LADISH_COMMAND_STATE_PENDING)
  {
    log_info("Delete studio room request (%s)", cmd_ptr->name);
  }

  list_for_each(node_ptr, &g_studio.rooms)
  {
    room = ladish_room_from_list_node(node_ptr);
    if (strcmp(ladish_room_get_name(room), cmd_ptr->name) == 0)
    {
      goto found;
    }
  }

  log_error("Cannot delete room with name \"%s\" because it is unknown", cmd_ptr->name);
  return false;

found:
  supervisor = ladish_room_get_app_supervisor(room);
  graph = ladish_room_get_graph(room);
  ladish_room_get_uuid(room, room_uuid);
  room_client = ladish_graph_find_client_by_uuid(g_studio.studio_graph, room_uuid);
  ASSERT(room_client != NULL);

  if (cmd_ptr->command.state == LADISH_COMMAND_STATE_PENDING)
  {
    ladish_graph_clear_persist(graph);
    ladish_graph_iterate_nodes(ladish_room_get_graph(room), false, room, NULL, uninit_room_ports, NULL);
    ladish_app_supervisor_stop(supervisor);

    cmd_ptr->command.state = LADISH_COMMAND_STATE_WAITING;
    return true;
  }

  ASSERT(cmd_ptr->command.state == LADISH_COMMAND_STATE_WAITING);

  running_app_count = ladish_app_supervisor_get_running_app_count(supervisor);
  if (running_app_count != 0)
  {
    log_info("there are %u running app(s) in room \"%s\"", running_app_count, cmd_ptr->name);
    return true;
  }

  if (!ladish_graph_looks_empty(graph))
  {
    log_info("the room \"%s\" graph is still not empty", cmd_ptr->name);
    return true;
  }

  if (!ladish_graph_client_looks_empty(g_studio.studio_graph, room_client))
  {
    log_info("the room \"%s\" studio client still does not look empty", cmd_ptr->name);
    return true;
  }

  /* ladish_graph_dump(graph); */
  /* ladish_graph_dump(g_studio.studio_graph); */
  /* ladish_graph_dump(g_studio.jack_graph); */

  ladish_graph_clear(graph, remove_port_callback);

  list_del(node_ptr);
  ladish_studio_emit_room_disappeared(room);

  ladish_graph_remove_client(g_studio.studio_graph, room_client);
  ladish_client_destroy(room_client);

  ladish_room_destroy(room);

  ladish_graph_dump(g_studio.studio_graph);
  ladish_graph_dump(g_studio.jack_graph);

  cmd_ptr->command.state = LADISH_COMMAND_STATE_DONE;
  return true;
}

static void destructor(void * context)
{
  log_info("delete_room command destructor");
  free(cmd_ptr->name);
}

#undef cmd_ptr

bool ladish_command_delete_room(void * call_ptr, struct ladish_cqueue * queue_ptr, const char * room_name)
{
  struct ladish_command_delete_room * cmd_ptr;
  char * room_name_dup;

  room_name_dup = strdup(room_name);
  if (room_name_dup == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup('%s') failed.", room_name);
    goto fail;
  }

  cmd_ptr = ladish_command_new(sizeof(struct ladish_command_delete_room));
  if (cmd_ptr == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "ladish_command_new() failed.");
    goto fail_free_name;
  }

  cmd_ptr->command.run = run;
  cmd_ptr->command.destructor = destructor;
  cmd_ptr->name = room_name_dup;

  if (!ladish_cqueue_add_command(queue_ptr, &cmd_ptr->command))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "ladish_cqueue_add_command() failed.");
    goto fail_destroy_command;
  }

  return true;

fail_destroy_command:
  free(cmd_ptr);
fail_free_name:
  free(room_name_dup);
fail:
  return false;
}
