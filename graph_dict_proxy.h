/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to graph dictionary D-Bus helpers
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

#ifndef GRAPH_DICT_PROXY_H__83E681EB_798E_4A25_9653_F9A9DBEB4D82__INCLUDED
#define GRAPH_DICT_PROXY_H__83E681EB_798E_4A25_9653_F9A9DBEB4D82__INCLUDED

#include "common.h"

bool
lash_graph_dict_proxy_set(
  const char * service,
  const char * object,
  uint32_t object_type,
  uint64_t object_id,
  const char * key,
  const char * value);

bool
lash_graph_dict_proxy_get(
  const char * service,
  const char * object,
  uint32_t object_type,
  uint64_t object_id,
  const char * key,
  char ** value);

bool
lash_graph_dict_proxy_drop(
  const char * service,
  const char * object,
  uint32_t object_type,
  uint64_t object_id,
  const char * key);

#endif /* #ifndef GRAPH_DICT_PROXY_H__83E681EB_798E_4A25_9653_F9A9DBEB4D82__INCLUDED */
