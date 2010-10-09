/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the core parts of room object implementation
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

#include "room_internal.h"
#include "../dbus_constants.h"
#include "graph_dict.h"
#include "../lib/wkports.h"
#include "studio.h"
#include "../proxies/jmcore_proxy.h"
#include "cmd.h"
#include "../dbus/error.h"
#include "recent_projects.h"

extern const struct dbus_interface_descriptor g_interface_room;

static bool port_is_input(uint32_t flags)
{
  bool playback;

  playback = JACKDBUS_PORT_IS_INPUT(flags);
  ASSERT(playback || JACKDBUS_PORT_IS_OUTPUT(flags)); /* playback or capture */
  ASSERT(!(playback && JACKDBUS_PORT_IS_OUTPUT(flags))); /* but not both */

  return playback;
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

  if (object_path != NULL)
  {
    room_ptr->object_path = strdup(object_path);
    if (room_ptr->object_path == NULL)
    {
      log_error("strdup() failed for room name");
      goto free_name;
    }
  }

  if (!ladish_graph_create(&room_ptr->graph, object_path))
  {
    goto free_opath;
  }

  return room_ptr;

free_opath:
  if (object_path != NULL)
  {
    free(room_ptr->object_path);
  }
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

  room_ptr->template = true;

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
  //log_info("Studio room port \"%s\"", port_name);

  if (port_is_input(port_flags))
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
  const char * input_port;
  const char * output_port;

  //log_info("Room port \"%s\"", port_name);

  ladish_graph_get_port_uuid(room_ptr->graph, port_handle, uuid_in_room);
  ladish_graph_get_port_uuid(room_ptr->owner, port_handle, uuid_in_owner);

  uuid_unparse(uuid_in_room, uuid_in_room_str);
  uuid_unparse(uuid_in_owner, uuid_in_owner_str);

  if (port_is_input(port_flags))
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
  ladish_graph_handle graph_handle,
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

  room_ptr->template = false;
  room_ptr->index = index;
  room_ptr->owner = owner;
  room_ptr->started = false;
  room_ptr->version = 1;

  room_ptr->project_name = NULL;
  room_ptr->project_dir = NULL;
  room_ptr->project_unloading = false;

  if (template != NULL)
  {
    ladish_room_get_uuid(template, room_ptr->template_uuid);
    if (!ladish_graph_copy(ladish_room_get_graph(template), room_ptr->graph, false))
    {
      goto destroy;
    }
  }
  else
  {
    uuid_clear(room_ptr->template_uuid);
  }

  if (!ladish_app_supervisor_create(&room_ptr->app_supervisor, object_path, room_ptr->name, room_ptr->graph, ladish_virtualizer_rename_app))
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
    &g_iface_recent_items, NULL,
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
  /* project has either both name and dir no none of them */
  ASSERT((room_ptr->project_dir == NULL && room_ptr->project_name == NULL) || (room_ptr->project_dir != NULL && room_ptr->project_name != NULL));
  if (room_ptr->project_dir == NULL)
  {
    free(room_ptr->project_dir);
  }
  if (room_ptr->project_name == NULL)
  {
    free(room_ptr->project_name);
  }

  if (!room_ptr->template)
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
  ladish_graph_handle graph_handle,
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
  ladish_graph_handle graph_handle,
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

  ladish_app_supervisor_autorun(room_ptr->app_supervisor);

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

static
bool
ladish_room_app_is_stopped(
  void * context,
  const char * name,
  bool running,
  const char * command,
  bool terminal,
  uint8_t level,
  pid_t pid,
  const uuid_t uuid)
{
  if (pid != 0)
  {
    log_info("App '%s' is still running pid=%u", name, (unsigned int)pid);
    return false;
  }

  if (!ladish_virtualizer_is_hidden_app(ladish_studio_get_jack_graph(), uuid, name))
  {
    log_info("App '%s' is still visible in the jack graph", name);
    return false;
  }

  return true;
}

static
bool
ladish_remove_room_app(
  void * context,
  const char * name,
  bool running,
  const char * command,
  bool terminal,
  uint8_t level,
  pid_t pid,
  const uuid_t uuid)
{
  ladish_virtualizer_remove_app(ladish_studio_get_jack_graph(), uuid, name);
  return true;
}

bool ladish_room_unload_project(ladish_room_handle room_handle)
{
  if (!room_ptr->project_unloading)
  {
    if (!ladish_app_supervisor_has_apps(room_ptr->app_supervisor))
    {
      goto done;
    }

    log_info("Stopping room apps...");
    ladish_graph_dump(room_ptr->graph);
    room_ptr->project_unloading = true;
    //ladish_graph_clear_persist(room_ptr->graph);
    ladish_app_supervisor_stop(room_ptr->app_supervisor);
    return false;
  }

  if (!ladish_app_supervisor_enum(room_ptr->app_supervisor, room_ptr, ladish_room_app_is_stopped))
  {
    return false;
  }

  /* remove app clients, ports and connections */
  ladish_app_supervisor_enum(room_ptr->app_supervisor, room_ptr, ladish_remove_room_app);

  ladish_app_supervisor_clear(room_ptr->app_supervisor);
  ASSERT(!ladish_app_supervisor_has_apps(room_ptr->app_supervisor));

  //ladish_graph_set_persist(room_ptr->graph);

  log_info("Room apps stopped.");
  ladish_graph_dump(room_ptr->graph);
  room_ptr->project_unloading = false;

done:
  if (room_ptr->project_name != NULL)
  {
    free(room_ptr->project_name);
    room_ptr->project_name = NULL;
  }
  if (room_ptr->project_dir != NULL)
  {
    free(room_ptr->project_dir);
    room_ptr->project_dir = NULL;
  }

  ladish_room_emit_project_properties_changed(room_ptr);

  return true;
}

ladish_port_handle
ladish_room_add_port(
  ladish_room_handle room_handle,
  const uuid_t uuid_ptr,
  const char * name,
  uint32_t type,
  uint32_t flags)
{
  ladish_port_handle port;
  bool playback;
  ladish_client_handle client;
  const char * client_name;
  uuid_t client_uuid;
  bool new_client;

  playback = port_is_input(flags);

  ASSERT(!uuid_is_null(uuid_ptr));
  if (!ladish_port_create(uuid_ptr, true, &port))
  {
    log_error("Creation of room port \"%s\" failed.", name);
    goto fail;
  }

  client_name = playback ? "Playback" : "Capture";
  uuid_copy(client_uuid, playback ? ladish_wkclient_playback : ladish_wkclient_capture);

  /* if client is not found, create it and add it to graph */
  client = ladish_graph_find_client_by_uuid(room_ptr->graph, client_uuid);
  new_client = client == NULL;
  if (new_client)
  {
    if (!ladish_client_create(client_uuid, &client))
    {
      log_error("ladish_client_create() failed to create %s room client.", playback ? "playback" : "capture");
      goto fail_destroy_port;
    }

    if (!ladish_graph_add_client(room_ptr->graph, client, client_name, true))
    {
      log_error("ladish_graph_add_client() failed to add %s room client to room graph.", playback ? "playback" : "capture");
      goto fail_destroy_client;
    }
  }

  if (!ladish_graph_add_port(room_ptr->graph, client, port, name, type, flags, true))
  {
    log_error("ladish_graph_add_port() failed to add %s room port \"%s\" to room graph.", playback ? "playback" : "capture", name);
    goto fail_destroy_client;
  }

  if (!create_shadow_port(room_ptr, port, name, type, flags))
  {
    log_error("ladish_graph_add_port() failed to add port \"%s\" to room owner graph.", name);
    goto fail_remove_port;
  }

  return port;

fail_remove_port:
  ASSERT(client != NULL);
  if (ladish_graph_remove_port(room_ptr->graph, port) != client)
  {
    ASSERT_NO_PASS;
  }
fail_destroy_client:
  if (new_client)
  {
    ladish_client_destroy(client);
  }
fail_destroy_port:
  ladish_port_destroy(port);
fail:
  return NULL;
}

#undef room_ptr

ladish_room_handle ladish_room_from_list_node(struct list_head * node_ptr)
{
  return (ladish_room_handle)list_entry(node_ptr, struct ladish_room, siblings);
}

static bool ladish_room_fill_project_properties(DBusMessageIter * iter_ptr, struct ladish_room * room_ptr)
{
  DBusMessageIter dict_iter;

  if (!dbus_message_iter_append_basic(iter_ptr, DBUS_TYPE_UINT64, &room_ptr->version))
  {
    log_error("dbus_message_iter_append_basic() failed.");
    return false;
  }

  if (!dbus_message_iter_open_container(iter_ptr, DBUS_TYPE_ARRAY, "{sv}", &dict_iter))
  {
    log_error("dbus_message_iter_open_container() failed.");
    return false;
  }

  if (!dbus_maybe_add_dict_entry_string(&dict_iter, "name", room_ptr->project_name))
  {
    log_error("dbus_maybe_add_dict_entry_string() failed.");
    return false;
  }

  if (!dbus_maybe_add_dict_entry_string(&dict_iter, "dir", room_ptr->project_dir))
  {
    log_error("dbus_maybe_add_dict_entry_string() failed.");
    return false;
  }

  if (!dbus_message_iter_close_container(iter_ptr, &dict_iter))
  {
    log_error("dbus_message_iter_close_container() failed.");
    return false;
  }

  return true;
}

void ladish_room_emit_project_properties_changed(struct ladish_room * room_ptr)
{
  DBusMessage * message_ptr;
  DBusMessageIter iter;

  room_ptr->version++;

  message_ptr = dbus_message_new_signal(room_ptr->object_path, IFACE_ROOM, "ProjectPropertiesChanged");
  if (message_ptr == NULL)
  {
    log_error("dbus_message_new_signal() failed.");
    return;
  }

  dbus_message_iter_init_append(message_ptr, &iter);

  if (ladish_room_fill_project_properties(&iter, room_ptr))
  {
    dbus_signal_send(g_dbus_connection, message_ptr);
  }

  dbus_message_unref(message_ptr);
}

/**********************************************************************************/
/*                                D-Bus methods                                   */
/**********************************************************************************/

#define room_ptr ((struct ladish_room *)call_ptr->iface_context)

static void ladish_room_dbus_get_name(struct dbus_method_call * call_ptr)
{
  method_return_new_single(call_ptr, DBUS_TYPE_STRING, &room_ptr->name);
}

static void ladish_room_dbus_save_project(struct dbus_method_call * call_ptr)
{
  const char * dir;
  const char * name;

  log_info("Save project request");

  if (!dbus_message_get_args(call_ptr->message, &g_dbus_error, DBUS_TYPE_STRING, &dir, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  if (ladish_command_save_project(call_ptr, ladish_studio_get_cmd_queue(), room_ptr->uuid, dir, name))
  {
    method_return_new_void(call_ptr);
  }
}

static void ladish_room_dbus_unload_project(struct dbus_method_call * call_ptr)
{
  log_info("Unload project request");

  if (ladish_command_unload_project(call_ptr, ladish_studio_get_cmd_queue(), room_ptr->uuid))
  {
    method_return_new_void(call_ptr);
  }
}

static void ladish_room_dbus_load_project(struct dbus_method_call * call_ptr)
{
  const char * dir;

  log_info("Load project request");

  if (!dbus_message_get_args(call_ptr->message, &g_dbus_error, DBUS_TYPE_STRING, &dir, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  if (ladish_command_load_project(call_ptr, ladish_studio_get_cmd_queue(), room_ptr->uuid, dir))
  {
    method_return_new_void(call_ptr);
  }
}

static void ladish_room_dbus_get_project_properties(struct dbus_method_call * call_ptr)
{
  DBusMessageIter iter;

  call_ptr->reply = dbus_message_new_method_return(call_ptr->message);
  if (call_ptr->reply == NULL)
  {
    goto fail;
  }

  dbus_message_iter_init_append(call_ptr->reply, &iter);

  if (!ladish_room_fill_project_properties(&iter, room_ptr))
  {
    goto fail_unref;
  }

  return;

fail_unref:
  dbus_message_unref(call_ptr->reply);
  call_ptr->reply = NULL;

fail:
  log_error("Ran out of memory trying to construct method return");
}

#undef room_ptr

METHOD_ARGS_BEGIN(GetName, "Get room name")
  METHOD_ARG_DESCRIBE_OUT("room_name", "s", "Name of room")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(SaveProject, "Save the current project")
  METHOD_ARG_DESCRIBE_IN("project_dir", "s", "Project directory. Can be an empty string if project has a path associated already")
  METHOD_ARG_DESCRIBE_IN("project_name", "s", "Name of the project. Can be an empty string if project has a name associated already")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(UnloadProject, "Unload project, if any")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(LoadProject, "Load project")
  METHOD_ARG_DESCRIBE_IN("project_dir", "s", "Project directory")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(GetProjectProperties, "Get project properties")
  METHOD_ARG_DESCRIBE_OUT("new_version", DBUS_TYPE_UINT64_AS_STRING, "New version of the project properties")
  METHOD_ARG_DESCRIBE_OUT("properties", "a{sv}", "project properties")
METHOD_ARGS_END

METHODS_BEGIN
  METHOD_DESCRIBE(GetName, ladish_room_dbus_get_name) /* sync */
  METHOD_DESCRIBE(SaveProject, ladish_room_dbus_save_project) /* async */
  METHOD_DESCRIBE(UnloadProject, ladish_room_dbus_unload_project) /* async */
  METHOD_DESCRIBE(LoadProject, ladish_room_dbus_load_project) /* async */
  METHOD_DESCRIBE(GetProjectProperties, ladish_room_dbus_get_project_properties) /* sync */
METHODS_END

SIGNAL_ARGS_BEGIN(ProjectPropertiesChanged, "Project properties changed")
  SIGNAL_ARG_DESCRIBE("new_version", DBUS_TYPE_UINT64_AS_STRING, "New version of the project properties")
  SIGNAL_ARG_DESCRIBE("properties", "a{sv}", "project properties")
SIGNAL_ARGS_END

SIGNALS_BEGIN
  SIGNAL_DESCRIBE(ProjectPropertiesChanged)
SIGNALS_END

INTERFACE_BEGIN(g_interface_room, IFACE_ROOM)
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
  INTERFACE_EXPOSE_SIGNALS
INTERFACE_END
