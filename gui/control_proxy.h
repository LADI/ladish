/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to code that interfaces
 * the ladishd Control object through D-Bus
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

#ifndef CONTROL_PROXY_H__8BC89E98_FE1B_4831_8B89_1A48F676E019__INCLUDED
#define CONTROL_PROXY_H__8BC89E98_FE1B_4831_8B89_1A48F676E019__INCLUDED

#include "common.h"
#include "../common/klist.h"

bool control_proxy_init(void);
void control_proxy_uninit(void);
void control_proxy_on_studio_appeared(void);
void control_proxy_on_studio_disappeared(void);
bool control_proxy_get_studio_list(void (* callback)(void * context, const char * studio_name), void * context);
bool control_proxy_load_studio(const char * studio_name);
bool control_proxy_delete_studio(const char * studio_name);
bool control_proxy_exit(void);

#endif /* #ifndef CONTROL_PROXY_H__8BC89E98_FE1B_4831_8B89_1A48F676E019__INCLUDED */
