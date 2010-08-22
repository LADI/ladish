/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains internal declarations used by the room object implementation
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

#ifndef ROOM_INTERNAL_H__FAF5B68F_E419_442A_8F9B_C729BAC00422__INCLUDED
#define ROOM_INTERNAL_H__FAF5B68F_E419_442A_8F9B_C729BAC00422__INCLUDED

#include "room.h"

#define LADISH_PROJECT_FILENAME "/ladish-project.xml"

struct ladish_room
{
  struct list_head siblings;
  uuid_t uuid;
  char * name;
  bool template;

  /* these are not valid for templates */
  uuid_t template_uuid;
  ladish_graph_handle owner;
  unsigned int index;
  char * object_path;
  dbus_object_path dbus_object;
  ladish_graph_handle graph;
  ladish_app_supervisor_handle app_supervisor;
  ladish_client_handle client;
  bool started;

  bool project_unloading;
  uuid_t project_uuid;
  char * project_dir;
  char * project_name;
};

void ladish_room_emit_project_properties_changed(struct ladish_room * room_ptr);

#endif /* #ifndef ROOM_INTERNAL_H__FAF5B68F_E419_442A_8F9B_C729BAC00422__INCLUDED */
