/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to the helper functionality for accessing
 * Studio object through D-Bus
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

#ifndef STUDIO_PROXY_H__2CEC623F_C998_4618_A947_D1A0016DF978__INCLUDED
#define STUDIO_PROXY_H__2CEC623F_C998_4618_A947_D1A0016DF978__INCLUDED

#include "common.h"

bool studio_proxy_init(void);
void studio_proxy_uninit(void);
bool studio_proxy_get_name(char ** name);
bool studio_proxy_rename(const char * name);
bool studio_proxy_save(void);
bool studio_proxy_save_as(const char * name);
bool studio_proxy_unload(void);
void studio_proxy_set_renamed_callback(void (* callback)(const char * new_studio_name));

void studio_proxy_set_startstop_callbacks(void (* started_callback)(void), void (* stopped_callback)(void), void (* crashed_callback)(void));
bool studio_proxy_start(void);
bool studio_proxy_stop(void);
bool studio_proxy_is_started(bool * is_started_ptr);

#endif /* #ifndef STUDIO_PROXY_H__2CEC623F_C998_4618_A947_D1A0016DF978__INCLUDED */
