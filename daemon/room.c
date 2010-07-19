/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the room object
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

#include "room.h"
#include "../dbus_constants.h"
#include "graph_dict.h"
#include "../lib/wkports.h"
#include "studio.h"
#include "../proxies/jmcore_proxy.h"

struct ladish_room
{
  struct list_head siblings;
  uuid_t uuid;
  char * name;
  uuid_t template_uuid;

  /* these are not valid for templates */
  ladish_graph_handle owner;
  unsigned int index;
  char * object_path;
  dbus_object_path dbus_object;
  ladish_graph_handle graph;
  ladish_app_supervisor_handle app_supervisor;
  ladish_client_handle client;
  bool started;
};

extern const struct dbus_interface_descriptor g_interface_room;

/* implemented in studio.c */
void ladish_on_app_renamed(void * context, const char * old_name, const char * new_app_name);

static bool get_port_direction(uint32_t port_flags, bool * room_input_ptr)
{
  if (JACKDBUS_PORT_IS_INPUT(port_flags))
  {
    *room_input_ptr = true;
    return true;
  }

  if (JACKDBUS_PORT_IS_OUTPUT(port_flags))
  {
    *room_input_ptr = false;
    return true;
  }

  log_error("room link port with bad flags %"PRIu32, port_flags);
  return false;
}

struct ladish_room * ladish_room_create_internal(const uuid_t uuid_ptr, const char * name, const char * object_path)
{
  struct ladish_room * room_ptr;

  room_ptr = malloc(sizeof(struct ladish_room));
  if (room_ptr == NULL)
  {
    log_error("malloc() failed to allocate struct ladish_room");
    goto fail;
  }

  if (uuid_ptr != NULL)
  {
    uuid_copy(room_ptr->uuid, uuid_ptr);
  }
  else
  {
    uuid_generate(room_ptr->uuid);
  }

  room_ptr->name = strdup(name);
  if (room_ptr->name == NULL)
  {
    log_error("strdup() failed for room name");
    goto free_room;
  }

  if (!ladish_graph_create(&room_ptr->graph, object_path))
  {
    goto free_name;
  }

  return room_ptr;

free_name:
  free(room_ptr->name);
free_room:
  free(room_ptr);
fail:
  return NULL;
}

bool
ladish_room_create_template(
  const uuid_t uuid_ptr,
  const char * name,
  ladish_room_handle * room_handle_ptr)
{
  struct ladish_room * room_ptr;

  room_ptr = ladish_room_create_internal(uuid_ptr, name, NULL);
  if (room_ptr == NULL)
  {
    return false;
  }

  uuid_clear(room_ptr->template_uuid);

  *room_handle_ptr = (ladish_room_handle)room_ptr;
  return true;
}

#define room_ptr ((struct ladish_room *)context)

static
bool
create_shadow_port(
  void * context,
  ladish_port_handle port_handle,
  const char * port_name,
  uint32_t port_type,
  uint32_t port_flags)
{
  bool room_input;

  //log_info("Studio room port \"%s\"", port_name);

  if (!get_port_direction(port_flags, &room_input))
  {
    return false;
  }

  if (room_input)
  {
    JACKDBUS_PORT_CLEAR_INPUT(port_flags);
    JACKDBUS_PORT_SET_OUTPUT(port_flags);
  }
  else
  {
    JACKDBUS_PORT_CLEAR_OUTPUT(port_flags);
    JACKDBUS_PORT_SET_INPUT(port_flags);
  }

  if (!ladish_graph_add_port(room_ptr->owner, room_ptr->client, port_handle, port_name, port_type, port_flags, true))
  {
    log_error("ladish_graph_add_port() failed to add link port to room owner graph");
    return false;
  }

  return true;
}

static
bool
create_port_link(
  void * context,
  ladish_port_handle port_handle,
  const char * port_name,
  uint32_t port_type,
  uint32_t port_flags)
{
  uuid_t uuid_in_owner;
  uuid_t uuid_in_room;
  char uuid_in_owner_str[37];
  char uuid_in_room_str[37];
  bool room_input;
  const char * input_port;
  const char * output_port;

  //log_info("Room port \"%s\"", port_name);

  if (!get_port_direction(port_flags, &room_input))
  {
    return false;
  }

  ladish_graph_get_port_uuid(room_ptr->graph, port_handle, uuid_in_room);
  ladish_graph_get_port_uuid(room_ptr->owner, port_handle, uuid_in_owner);

  uuid_unparse(uuid_in_room, uuid_in_room_str);
  uuid_unparse(uuid_in_owner, uuid_in_owner_str);

  if (room_input)
  {
    input_port = uuid_in_room_str;
    output_port = uuid_in_owner_str;
    log_info("room input port %s is linked to owner graph output port %s", input_port, output_port);
  }
  else
  {
    input_port = uuid_in_owner_str;
    output_port = uuid_in_room_str;
    log_info("owner graph input port %s is linked to room output port %s", input_port, output_port);
  }

  if (!jmcore_proxy_create_link(port_type == JACKDBUS_PORT_TYPE_MIDI, input_port, output_port))
  {
    log_error("jmcore_proxy_create_link() failed.");
    return false;
  }

  return true;
}

static
bool
destroy_port_link(
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

#undef room_ptr

static void remove_port_callback(ladish_port_handle port)
{
  ladish_graph_handle jack_graph;
  ladish_client_handle jack_client;

  jack_graph = ladish_studio_get_jack_graph();

  jack_client = ladish_graph_remove_port(jack_graph, port);
  if (jack_client == NULL)
  { /* room app port not found in jack graph */
    /* this can happen if the port is hidden in the vgraph  */
    return;
  }

  if (ladish_graph_client_is_empty(jack_graph, jack_client))
  {
    ladish_graph_remove_client(jack_graph, jack_client);
  }
}

bool
ladish_room_create(
  const uuid_t uuid_ptr,
  const char * name,
  ladish_room_handle template,
  ladish_graph_handle owner,
  ladish_room_handle * room_handle_ptr)
{
  struct ladish_room * room_ptr;
  char object_path[1024];
  unsigned int index;

  index = ladish_studio_get_room_index();
  sprintf(object_path, DBUS_BASE_PATH "/Room%u", index);

  room_ptr = ladish_room_create_internal(uuid_ptr, name, object_path);
  if (room_ptr == NULL)
  {
    goto release_index;
  }

  room_ptr->index = index;
  ladish_room_get_uuid(template, room_ptr->template_uuid);
  room_ptr->owner = owner;
  room_ptr->started = false;

  if (!ladish_graph_copy(ladish_room_get_graph(template), room_ptr->graph, false))
  {
    goto destroy;
  }

  if (!ladish_app_supervisor_create(&room_ptr->app_supervisor, object_path, room_ptr->name, room_ptr->graph, ladish_on_app_renamed))
  {
    log_error("ladish_app_supervisor_create() failed.");
    goto destroy;
  }

  room_ptr->dbus_object = dbus_object_path_new(
    object_path,
    &g_interface_room, room_ptr,
    &g_interface_patchbay, ladish_graph_get_dbus_context(room_ptr->graph),
    &g_iface_graph_dict, room_ptr->graph,
    &g_iface_app_supervisor, room_ptr->app_supervisor,
    NULL);
  if (room_ptr->dbus_object == NULL)
  {
    log_error("dbus_object_path_new() failed");
    goto destroy_app_supervisor;
  }

  if (!dbus_object_path_register(g_dbus_connection, room_ptr->dbus_object))
  {
    log_error("object_path_register() failed");
    goto destroy_dbus_object;
  }

  log_info("D-Bus object \"%s\" created for room \"%s\".", object_path, room_ptr->name);

  if (!ladish_client_create(room_ptr->uuid, &room_ptr->client))
  {
    log_error("ladish_client_create() failed.");
    goto unregister_dbus_object;
  }

  if (!ladish_graph_add_client(owner, room_ptr->client, room_ptr->name, true))
  {
    log_error("ladish_graph_add_client() failed to add room client to owner graph.");
    goto destroy_client;
  }

  if (!ladish_room_iterate_link_ports((ladish_room_handle)room_ptr, room_ptr, create_shadow_port))
  {
    log_error("Creation of studio room link ports failed.");
    goto remove_client;
  }

  ladish_studio_room_appeared((ladish_room_handle)room_ptr);

  *room_handle_ptr = (ladish_room_handle)room_ptr;
  return true;

remove_client:
  ladish_graph_remove_client(owner, room_ptr->client);
destroy_client:
  ladish_client_destroy(room_ptr->client);
unregister_dbus_object:
  dbus_object_path_unregister(g_dbus_connection, room_ptr->dbus_object);
destroy_dbus_object:
  dbus_object_path_destroy(g_dbus_connection, room_ptr->dbus_object);
destroy_app_supervisor:
  ladish_app_supervisor_destroy(room_ptr->app_supervisor);
destroy:
  ladish_graph_destroy(room_ptr->graph);
  free(room_ptr->name);
  free(room_ptr);
release_index:
  ladish_studio_release_room_index(index);
  return false;
}

#define room_ptr ((struct ladish_room *)room_handle)

void ladish_room_destroy(ladish_room_handle room_handle)
{
  if (!uuid_is_null(room_ptr->template_uuid))
  {
    ASSERT(!room_ptr->started); /* attempt to destroy not stopped room */

    /* ladish_graph_dump(graph); */

    if (ladish_studio_is_started())
    {
      ladish_graph_clear(room_ptr->graph, remove_port_callback);
    }

    dbus_object_path_destroy(g_dbus_connection, room_ptr->dbus_object);
    ladish_app_supervisor_destroy(room_ptr->app_supervisor);

    ladish_graph_remove_client(room_ptr->owner, room_ptr->client);
    ladish_client_destroy(room_ptr->client);

    ladish_studio_room_disappeared((ladish_room_handle)room_ptr);
    ladish_studio_release_room_index(room_ptr->index);
  }

  ladish_graph_destroy(room_ptr->graph);
  free(room_ptr->name);
  free(room_ptr);
}

struct list_head * ladish_room_get_list_node(ladish_room_handle room_handle)
{
  return &room_ptr->siblings;
}

const char * ladish_room_get_name(ladish_room_handle room_handle)
{
  return room_ptr->name;
}

const char * ladish_room_get_opath(ladish_room_handle room_handle)
{
  return ladish_graph_get_opath(room_ptr->graph);
}

bool ladish_room_get_template_uuid(ladish_room_handle room_handle, uuid_t uuid_ptr)
{
  if (uuid_is_null(room_ptr->template_uuid))
  {
    return false;
  }

  uuid_copy(uuid_ptr, room_ptr->template_uuid);
  return true;
}

void ladish_room_get_uuid(ladish_room_handle room_handle, uuid_t uuid_ptr)
{
  uuid_copy(uuid_ptr, room_ptr->uuid);
}

ladish_graph_handle ladish_room_get_graph(ladish_room_handle room_handle)
{
  return room_ptr->graph;
}

ladish_app_supervisor_handle ladish_room_get_app_supervisor(ladish_room_handle room_handle)
{
  return room_ptr->app_supervisor;
}

struct ladish_room_iterate_link_ports_context
{
  void * context;
  bool
  (* callback)(
    void * context,
    ladish_port_handle port_handle,
    const char * port_name,
    uint32_t port_type,
    uint32_t port_flags);
};

#define context_ptr ((struct ladish_room_iterate_link_ports_context *)context)

static
bool
ladish_room_iterate_link_ports_client_callback(
  void * context,
  ladish_client_handle client_handle,
  const char * client_name,
  void ** client_iteration_context_ptr_ptr)
{
  uuid_t uuid;

  ladish_client_get_uuid(client_handle, uuid);

  if (uuid_compare(uuid, ladish_wkclient_capture) == 0 ||
      uuid_compare(uuid, ladish_wkclient_playback) == 0)
  {
    *client_iteration_context_ptr_ptr = (void *)1;
  }
  else
  {
    *client_iteration_context_ptr_ptr = (void *)0;
  }

  return true;
}

static
bool
ladish_room_iterate_link_ports_port_callback(
  void * context,
  void * client_iteration_context_ptr,
  ladish_client_handle client_handle,
  const char * client_name,
  ladish_port_handle port_handle,
  const char * port_name,
  uint32_t port_type,
  uint32_t port_flags)
{
  if (client_iteration_context_ptr == (void *)0)
  {
    /* port of non-link client */
    return true;
  }

  return context_ptr->callback(context_ptr->context, port_handle, port_name, port_type, port_flags);
}

#undef context_ptr

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
    uint32_t port_flags))
{
  struct ladish_room_iterate_link_ports_context context;

  context.context = callback_context;
  context.callback = callback;

  return ladish_graph_iterate_nodes(
    room_ptr->graph,
    false,
    NULL,
    &context,
    ladish_room_iterate_link_ports_client_callback,
    ladish_room_iterate_link_ports_port_callback,
    NULL);
}

bool ladish_room_start(ladish_room_handle room_handle, ladish_virtualizer_handle virtualizer)
{
  if (!ladish_room_iterate_link_ports(room_handle, room_ptr, create_port_link))
  {
    log_error("Creation of room port links failed.");
    return false;
  }

  ladish_virtualizer_set_graph_connection_handlers(virtualizer, room_ptr->graph);
  room_ptr->started = true;

  return true;
}

void ladish_room_initiate_stop(ladish_room_handle room_handle, bool clear_persist)
{
  if (!room_ptr->started)
  {
    return;
  }

  ladish_graph_set_connection_handlers(room_ptr->graph, NULL, NULL, NULL);

  if (clear_persist)
  {
    ladish_graph_clear_persist(room_ptr->graph);
  }

  ladish_graph_iterate_nodes(room_ptr->graph, false, NULL, room_ptr, NULL, destroy_port_link, NULL);
  ladish_app_supervisor_stop(room_ptr->app_supervisor);
}

bool ladish_room_stopped(ladish_room_handle room_handle)
{
  unsigned int running_app_count;

  if (!room_ptr->started)
  {
    return true;
  }

  running_app_count = ladish_app_supervisor_get_running_app_count(room_ptr->app_supervisor);
  if (running_app_count != 0)
  {
    log_info("there are %u running app(s) in room \"%s\"", running_app_count, room_ptr->name);
    return false;
  }

  if (!ladish_graph_looks_empty(room_ptr->graph))
  {
    log_info("the room \"%s\" graph is still not empty", room_ptr->name);
    return false;
  }

  if (!ladish_graph_client_looks_empty(room_ptr->owner, room_ptr->client))
  {
    log_info("the room \"%s\" client in owner still does not look empty", room_ptr->name);
    return false;
  }

  room_ptr->started = false;
  return true;
}

#undef room_ptr

ladish_room_handle ladish_room_from_list_node(struct list_head * node_ptr)
{
  return (ladish_room_handle)list_entry(node_ptr, struct ladish_room, siblings);
}

#define room_ptr ((struct ladish_room *)call_ptr->iface_context)

static void ladish_get_room_name(struct dbus_method_call * call_ptr)
{
  method_return_new_single(call_ptr, DBUS_TYPE_STRING, &room_ptr->name);
}

#undef room_ptr

METHOD_ARGS_BEGIN(GetName, "Get room name")
  METHOD_ARG_DESCRIBE_OUT("room_name", "s", "Name of room")
METHOD_ARGS_END

METHODS_BEGIN
  METHOD_DESCRIBE(GetName, ladish_get_room_name)
METHODS_END

SIGNALS_BEGIN
SIGNALS_END

INTERFACE_BEGIN(g_interface_room, IFACE_ROOM)
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
  INTERFACE_EXPOSE_SIGNALS
INTERFACE_END
