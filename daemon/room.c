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
#include "graph.h"
#include "graph_dict.h"

struct ladish_room
{
  struct list_head siblings;
  uuid_t uuid;
  char * name;
  uuid_t template_uuid;
  dbus_object_path dbus_object;
  ladish_graph_handle graph;
};

extern const struct dbus_interface_descriptor g_interface_room;

bool
ladish_room_create(
  const uuid_t uuid_ptr,
  const char * name,
  ladish_room_handle template,
  const char * object_path,
  ladish_room_handle * room_handle_ptr)
{
  struct ladish_room * room_ptr;

  room_ptr = malloc(sizeof(struct ladish_room));
  if (room_ptr == NULL)
  {
    log_error("malloc() failed to allocate struct ladish_room");
    return false;
  }

  if (uuid_ptr == NULL)
  {
    if (template == NULL)
    {
      uuid_generate(room_ptr->uuid);
    }
    else
    {
      ladish_room_get_uuid(template, room_ptr->uuid);
    }
  }
  else
  {
    uuid_copy(room_ptr->uuid, uuid_ptr);
  }

  if (template != NULL)
  {
    ladish_room_get_uuid(template, room_ptr->template_uuid);
  }
  else
  {
    uuid_clear(room_ptr->template_uuid);
  }

  room_ptr->name = strdup(name);
  if (room_ptr->name == NULL)
  {
    log_error("strdup() failed for room name");
    free(room_ptr);
    return false;
  }

  if (object_path)
  {
    if (!ladish_graph_create(&room_ptr->graph, object_path))
    {
      free(room_ptr);
      return false;
    }

    room_ptr->dbus_object = dbus_object_path_new(
      object_path,
      &g_interface_room, room_ptr,
      &g_interface_patchbay, ladish_graph_get_dbus_context(room_ptr->graph),
      &g_iface_graph_dict, room_ptr->graph,
      //&g_iface_app_supervisor, room_ptr->app_supervisor,
      NULL);
    if (room_ptr->dbus_object == NULL)
    {
      log_error("dbus_object_path_new() failed");
      ladish_graph_destroy(room_ptr->graph, true);
      free(room_ptr);
      return false;
    }

    if (!dbus_object_path_register(g_dbus_connection, room_ptr->dbus_object))
    {
      log_error("object_path_register() failed");
      dbus_object_path_destroy(g_dbus_connection, room_ptr->dbus_object);
      ladish_graph_destroy(room_ptr->graph, true);
      free(room_ptr);
      return false;
    }

    log_info("D-Bus object \"%s\" created for room \"%s\".", object_path, room_ptr->name);
  }
  else
  {
    room_ptr->dbus_object = NULL;
    room_ptr->graph = NULL;
  }

  *room_handle_ptr = (ladish_room_handle)room_ptr;
  return true;
}

#define room_ptr ((struct ladish_room *)room_handle)

void
ladish_room_destroy(
  ladish_room_handle room_handle)
{
  if (room_ptr->dbus_object != NULL)
  {
    dbus_object_path_destroy(g_dbus_connection, room_ptr->dbus_object);
    ladish_graph_destroy(room_ptr->graph, true);
  }

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
  if (room_ptr->graph == NULL)
  {
    return NULL;
  }

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
