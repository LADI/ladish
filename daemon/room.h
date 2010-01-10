/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface of the room object
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

#ifndef ROOM_H__9A1CF253_0A17_402A_BDF8_9BD72B467118__INCLUDED
#define ROOM_H__9A1CF253_0A17_402A_BDF8_9BD72B467118__INCLUDED

#include "common.h"

typedef struct ladish_room_tag { int unused; } * ladish_room_handle;

bool
ladish_room_create(
  const uuid_t uuid_ptr,
  const char * name,
  ladish_room_handle * room_handle_ptr);

void
ladish_room_destroy(
  ladish_room_handle room_handle);

#endif /* #ifndef ROOM_H__9A1CF253_0A17_402A_BDF8_9BD72B467118__INCLUDED */
