/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009,2010,2011,2012 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the interface of the client objects
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

#ifndef CLIENT_H__2160B4BA_D6D1_464D_9DC5_ECA50B0958AD__INCLUDED
#define CLIENT_H__2160B4BA_D6D1_464D_9DC5_ECA50B0958AD__INCLUDED

#include "dict.h"

typedef struct ladish_client_tag { int unused; } * ladish_client_handle;

bool
ladish_client_create(
  const uuid_t uuid_ptr,
  ladish_client_handle * client_handle_ptr);

bool
ladish_client_create_copy(
  ladish_client_handle client_handle,
  ladish_client_handle * client_handle_ptr);

ladish_dict_handle ladish_client_get_dict(ladish_client_handle client_handle);

void ladish_client_get_uuid(ladish_client_handle client_handle, uuid_t uuid);

void ladish_client_set_jack_id(ladish_client_handle client_handle, uint64_t jack_id);
uint64_t ladish_client_get_jack_id(ladish_client_handle client_handle);

void ladish_client_set_jack_name(ladish_client_handle client_handle, const char * jack_name);
const char * ladish_client_get_jack_name(ladish_client_handle client_handle);

void ladish_client_set_pid(ladish_client_handle client_handle, pid_t pid);
pid_t ladish_client_get_pid(ladish_client_handle client_handle);

void ladish_client_set_vgraph(ladish_client_handle client_handle, void * vgraph);
void * ladish_client_get_vgraph(ladish_client_handle client_handle);

void ladish_client_interlink(ladish_client_handle client1_handle, ladish_client_handle client2_handle);
void ladish_client_interlink_copy(ladish_client_handle dst_client_handle, ladish_client_handle src_client_handle);
bool ladish_client_get_interlink(ladish_client_handle client_handle, uuid_t uuid);
void ladish_client_clear_interlink(ladish_client_handle client_handle);

void ladish_client_set_app(ladish_client_handle client_handle, const uuid_t uuid);
bool ladish_client_get_app(ladish_client_handle client_handle, uuid_t uuid);
bool ladish_client_is_app(ladish_client_handle client_handle, uuid_t uuid);
bool ladish_client_has_app(ladish_client_handle client_handle);
void ladish_client_copy_app(ladish_client_handle dst_client_handle, ladish_client_handle src_client_handle);

void ladish_client_set_js(ladish_client_handle client_handle, bool has_js_callback);
bool ladish_client_is_js(ladish_client_handle client_handle);

#endif /* #ifndef CLIENT_H__2160B4BA_D6D1_464D_9DC5_ECA50B0958AD__INCLUDED */
