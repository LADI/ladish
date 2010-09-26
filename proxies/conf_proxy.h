/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to code that interfaces ladiconfd through D-Bus
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

#ifndef CONF_PROXY_H__D45503B2_D49C_46BF_86F7_9A531B819B6B__INCLUDED
#define CONF_PROXY_H__D45503B2_D49C_46BF_86F7_9A531B819B6B__INCLUDED

#include "common.h"

bool conf_proxy_init(void);
void conf_proxy_uninit(void);

bool
conf_register(
  const char * key,
  void (* callback)(void * context, const char * key, const char * value),
  void * callback_context);

bool conf_set(const char * key, const char * value);
bool conf_get(const char * key, const char ** value_ptr);

bool conf_string2bool(const char * value);
const char * conf_bool2string(bool value);

bool conf_set_bool(const char * key, bool value);
bool conf_get_bool(const char * key, bool * value_ptr);

#endif /* #ifndef CONF_PROXY_H__D45503B2_D49C_46BF_86F7_9A531B819B6B__INCLUDED */
