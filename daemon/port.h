/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the interface of the port objects
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

#ifndef PORT_H__62F81E7C_91FA_44AB_94A9_E0E2D226ED58__INCLUDED
#define PORT_H__62F81E7C_91FA_44AB_94A9_E0E2D226ED58__INCLUDED

#include "dict.h"

typedef struct ladish_port_tag { int unused; } * ladish_port_handle;

bool ladish_port_create(const uuid_t uuid_ptr, ladish_port_handle * port_handle_ptr);
bool ladish_port_create_copy(ladish_port_handle port_handle, ladish_port_handle * port_handle_ptr);
void ladish_port_destroy(ladish_port_handle port_handle);
ladish_dict_handle ladish_port_get_dict(ladish_port_handle port_handle);
void ladish_port_get_uuid(ladish_port_handle port_handle, uuid_t uuid);
void ladish_port_set_jack_id(ladish_port_handle port_handle, uint64_t jack_id);
uint64_t ladish_port_get_jack_id(ladish_port_handle port_handle);

void ladish_port_add_ref(ladish_port_handle port_handle);
void ladish_port_del_ref(ladish_port_handle port_handle);

#endif /* #ifndef PORT_H__62F81E7C_91FA_44AB_94A9_E0E2D226ED58__INCLUDED */
