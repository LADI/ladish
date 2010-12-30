/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the interface of the dictionary objects
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

#ifndef DICT_H__12A321F8_A361_482B_9255_66CCD4D3C31F__INCLUDED
#define DICT_H__12A321F8_A361_482B_9255_66CCD4D3C31F__INCLUDED

#include "common.h"

typedef struct ladish_dict_tag { int unused; } * ladish_dict_handle;

bool ladish_dict_create(ladish_dict_handle * dict_handle_ptr);
bool ladish_dict_dup(ladish_dict_handle src_dict_handle, ladish_dict_handle * dst_dict_handle_ptr);
void ladish_dict_destroy(ladish_dict_handle dict_handle);
bool ladish_dict_set(ladish_dict_handle dict_handle, const char * key, const char * value);
const char * ladish_dict_get(ladish_dict_handle dict_handle, const char * key);
void ladish_dict_drop(ladish_dict_handle dict_handle, const char * key);
void ladish_dict_clear(ladish_dict_handle dict_handle);
bool ladish_dict_iterate(ladish_dict_handle dict_handle, void * context, bool (* callback)(void * context, const char * key, const char * value));
bool ladish_dict_is_empty(ladish_dict_handle dict_handle);

#endif /* #ifndef DICT_H__12A321F8_A361_482B_9255_66CCD4D3C31F__INCLUDED */
