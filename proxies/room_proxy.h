/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to room object that is backed through D-Bus
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

#ifndef ROOM_PROXY_H__0FDD1790_EF07_4C6C_8C95_0F75E29A3E81__INCLUDED
#define ROOM_PROXY_H__0FDD1790_EF07_4C6C_8C95_0F75E29A3E81__INCLUDED

#include "common.h"

typedef struct ladish_room_proxy_tag { int unused; } * ladish_room_proxy_handle;

bool
ladish_room_proxy_create(
  const char * service,
  const char * object,
  void * project_properties_changed_context,
  void (* project_properties_changed)(
    void * project_properties_changed_context,
    const char * project_dir,
    const char * project_name,
    const char * project_description,
    const char * project_notes),
  ladish_room_proxy_handle * proxy_ptr);

void ladish_room_proxy_destroy(ladish_room_proxy_handle proxy);
char * ladish_room_proxy_get_name(ladish_room_proxy_handle proxy);
bool ladish_room_proxy_load_project(ladish_room_proxy_handle proxy, const char * project_dir);
bool ladish_room_proxy_save_project(ladish_room_proxy_handle proxy, const char * project_dir, const char * project_name);
bool ladish_room_proxy_unload_project(ladish_room_proxy_handle proxy);

void
ladish_room_proxy_get_project_properties(
  ladish_room_proxy_handle proxy,
  const char ** project_dir,
  const char ** project_name,
  const char ** project_description,
  const char ** project_notes);

bool
ladish_room_proxy_set_project_description(
  ladish_room_proxy_handle proxy,
  const char * description);

bool
ladish_room_proxy_set_project_notes(
  ladish_room_proxy_handle proxy,
  const char * notes);

bool
ladish_room_proxy_get_recent_projects(
  ladish_room_proxy_handle proxy,
  uint16_t max_items,
  void (* callback)(
    void * context,
    const char * project_name,
    const char * project_dir),
  void * context);

#endif /* #ifndef ROOM_PROXY_H__0FDD1790_EF07_4C6C_8C95_0F75E29A3E81__INCLUDED */
