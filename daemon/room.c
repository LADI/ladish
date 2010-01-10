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

struct ladish_room
{
  uuid_t uuid;
  char * name;
};

bool
ladish_room_create(
  const uuid_t uuid_ptr,
  const char * name,
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
    uuid_generate(room_ptr->uuid);
  }
  else
  {
    uuid_copy(room_ptr->uuid, uuid_ptr);
  }

  room_ptr->name = strdup(name);
  if (room_ptr->name == NULL)
  {
    log_error("strdup() failed for room name");
    free(room_ptr);
    return false;
  }

  *room_handle_ptr = (ladish_room_handle)room_ptr;
  return true;
}

#define room_ptr ((struct ladish_room *)room_handle)

void
ladish_room_destroy(
  ladish_room_handle room_handle)
{
  free(room_ptr->name);
  free(room_ptr);
}

#undef room_ptr
