/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the "create room" command
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
#include "control.h"

struct ladish_command_create_room
{
  struct ladish_command command; /* must be the first member */
  char * room_name;
  char * template_name;
};

struct add_room_ports_context
{
  ladish_client_handle room_client;
  ladish_graph_handle room_graph;
};

#define add_room_ports_context_ptr ((struct add_room_ports_context *)context)

static
bool
ladish_studio_add_room_ports(
  void * context,
  ladish_port_handle port_handle,
  const char * port_name,
  uint32_t port_type,
  uint32_t port_flags)
{
  uuid_t uuid_in_studio;
  uuid_t uuid_in_room;
  char uuid_in_studio_str[37];
  char uuid_in_room_str[37];
  bool room_input;
  const char * input_port;
  const char * output_port;

  //log_info("Studio room port \"%s\"", port_name);

  if (JACKDBUS_PORT_IS_INPUT(port_flags))
  {
    JACKDBUS_PORT_CLEAR_INPUT(port_flags);
    JACKDBUS_PORT_SET_OUTPUT(port_flags);
    room_input = true;
  }
  else if (JACKDBUS_PORT_IS_OUTPUT(port_flags))
  {
    JACKDBUS_PORT_CLEAR_OUTPUT(port_flags);
    JACKDBUS_PORT_SET_INPUT(port_flags);
    room_input = false;
  }
  else
  {
    log_error("room link port with bad flags %"PRIu32, port_flags);
    return false;
  }

  if (!ladish_graph_add_port(g_studio.studio_graph, add_room_ports_context_ptr->room_client, port_handle, port_name, port_type, port_flags, true))
  {
    log_error("ladish_graph_add_port() failed to add link port to studio graph");
    return false;
  }

  ladish_graph_get_port_uuid(add_room_ports_context_ptr->room_graph, port_handle, uuid_in_room);
  ladish_graph_get_port_uuid(g_studio.studio_graph, port_handle, uuid_in_studio);

  uuid_unparse(uuid_in_room, uuid_in_room_str);
  uuid_unparse(uuid_in_studio, uuid_in_studio_str);

  if (room_input)
  {
    input_port = uuid_in_room_str;
    output_port = uuid_in_studio_str;
    log_info("room input port %s is linked to studio output port %s", input_port, output_port);
  }
  else
  {
    input_port = uuid_in_studio_str;
    output_port = uuid_in_room_str;
    log_info("studio input port %s is linked to room output port %s", input_port, output_port);
  }

  if (!jmcore_proxy_create_link(port_type == JACKDBUS_PORT_TYPE_MIDI, input_port, output_port))
  {
    log_error("jmcore_proxy_create_link() failed.");
    return false;
  }

  return true;
}

#undef add_room_ports_context_ptr
#define cmd_ptr ((struct ladish_command_create_room *)cmd_context)

static bool run(void * cmd_context)
{
  ladish_room_handle room;
  char room_dbus_name[1024];
  ladish_client_handle room_client;
  uuid_t room_uuid;
  ladish_graph_handle room_graph;
  struct add_room_ports_context context;

  ASSERT(cmd_ptr->command.state == LADISH_COMMAND_STATE_PENDING);

  log_info("Request to create new studio room \"%s\" from template \"%s\".", cmd_ptr->room_name, cmd_ptr->template_name);

  room = find_room_template_by_name(cmd_ptr->template_name);
  if (room == NULL)
  {
    log_error("Unknown room template \"%s\"",  cmd_ptr->template_name);
    goto fail;
  }

  g_studio.room_count++;

  sprintf(room_dbus_name, DBUS_BASE_PATH "/Room%u", g_studio.room_count);

  if (!ladish_room_create(NULL, cmd_ptr->room_name, room, room_dbus_name, &room))
  {
    log_error("ladish_room_create() failed.");
    goto fail_decrement_room_count;
  }

  room_graph = ladish_room_get_graph(room);
  if (g_studio.virtualizer != NULL)
  {
    ladish_virtualizer_set_graph_connection_handlers(g_studio.virtualizer, room_graph);
  }

  ladish_room_get_uuid(room, room_uuid);

  if (!ladish_client_create(room_uuid, &room_client))
  {
    log_error("ladish_client_create() failed.");
    goto fail_destroy_room;
  }

  if (!ladish_graph_add_client(g_studio.studio_graph, room_client, cmd_ptr->room_name, false))
  {
    log_error("ladish_graph_add_client() failed to add room client to studio graph.");
    goto fail_destroy_room_client;
  }

  context.room_client = room_client;
  context.room_graph = room_graph;

  if (!ladish_room_iterate_link_ports(room, &context, ladish_studio_add_room_ports))
  {
    log_error("Creation of studio room link ports failed.");
    goto fail_remove_room_client;
  }

  list_add_tail(ladish_room_get_list_node(room), &g_studio.rooms);

  ladish_studio_emit_room_appeared(room);

  cmd_ptr->command.state = LADISH_COMMAND_STATE_DONE;
  return true;

fail_remove_room_client:
  ladish_graph_remove_client(g_studio.studio_graph, room_client);
fail_destroy_room_client:
  ladish_client_destroy(room_client);
fail_destroy_room:
  ladish_room_destroy(room);
fail_decrement_room_count:
  g_studio.room_count--;
fail:
  return false;
}

static void destructor(void * cmd_context)
{
  log_info("create_room command destructor");
  free(cmd_ptr->room_name);
  free(cmd_ptr->template_name);
}

#undef cmd_ptr

bool ladish_command_create_room(void * call_ptr, struct ladish_cqueue * queue_ptr, const char * room_name, const char * template_name)
{
  struct ladish_command_create_room * cmd_ptr;
  char * room_name_dup;
  char * template_name_dup;

  room_name_dup = strdup(room_name);
  if (room_name_dup == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup('%s') failed.", room_name);
    goto fail;
  }

  template_name_dup = strdup(template_name);
  if (template_name_dup == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup('%s') failed.", template_name);
    goto fail_free_room_name;
  }

  cmd_ptr = ladish_command_new(sizeof(struct ladish_command_create_room));
  if (cmd_ptr == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "ladish_command_new() failed.");
    goto fail_free_template_name;
  }

  cmd_ptr->command.run = run;
  cmd_ptr->command.destructor = destructor;
  cmd_ptr->room_name = room_name_dup;
  cmd_ptr->template_name = template_name_dup;

  if (!ladish_cqueue_add_command(queue_ptr, &cmd_ptr->command))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "ladish_cqueue_add_command() failed.");
    goto fail_destroy_command;
  }

  return true;

fail_destroy_command:
  free(cmd_ptr);
fail_free_template_name:
  free(room_name_dup);
fail_free_room_name:
  free(room_name_dup);
fail:
  return false;
}
