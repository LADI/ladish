/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to recent items store
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

#ifndef RECENT_STORE_H__52816145_014A_4CDA_B713_D7F39AAB4E73__INCLUDED
#define RECENT_STORE_H__52816145_014A_4CDA_B713_D7F39AAB4E73__INCLUDED

#include "common.h"

typedef struct ladish_recent_store_tag { int unused; } * ladish_recent_store_handle;

bool
ladish_recent_store_create(
  const char * file_path,
  unsigned int max_items,
  ladish_recent_store_handle * store_ptr);

void
ladish_recent_store_destroy(
  ladish_recent_store_handle store);

void
ladish_recent_store_use_item(
  ladish_recent_store_handle store,
  const char * item);

bool
ladish_recent_store_check_known(
  ladish_recent_store_handle store,
  const char * item);

void
ladish_recent_store_iterate_items(
  ladish_recent_store_handle store,
  void * callback_context,
  bool (* callback)(void * callback_context, const char * item));

#endif /* #ifndef RECENT_STORE_H__52816145_014A_4CDA_B713_D7F39AAB4E73__INCLUDED */
