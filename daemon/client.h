/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
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

typedef struct ladish_client_tag { int unused; } * ladish_client_handle;

bool
ladish_client_create(
  uuid_t uuid_ptr,
  bool virtual,
  bool link,
  bool system,
  ladish_client_handle * client_handle_ptr);

void
ladish_client_destroy(
  ladish_client_handle client_handle);

#endif /* #ifndef CLIENT_H__2160B4BA_D6D1_464D_9DC5_ECA50B0958AD__INCLUDED */
