/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to menu related code
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

#ifndef MENU_H__37ACA2FE_C43D_4FA8_B7CF_6DD345F17CD1__INCLUDED
#define MENU_H__37ACA2FE_C43D_4FA8_B7CF_6DD345F17CD1__INCLUDED

void menu_init(void);
void menu_studio_state_changed(unsigned int studio_state);
void menu_set_jack_latency_items_sensivity(bool sensitive);
bool menu_set_jack_latency(uint32_t buffer_size, bool force, bool * changed_ptr);
void menu_view_activated(bool room);

void menu_request_daemon_exit(void);
void menu_request_jack_configure(void);
void menu_request_save_studio(void);
void menu_request_save_as_studio(void);
void menu_request_new_studio(void);
void menu_request_start_app(void);
void menu_request_start_studio(void);
void menu_request_stop_studio(void);
void menu_request_unload_studio(void);
void menu_request_rename_studio(void);
void menu_request_create_room(void);
void menu_request_destroy_room(void);
void menu_request_jack_latency_change(uint32_t buffer_size);

void menu_request_toggle_toolbar(bool visible);
void menu_request_toggle_raw_jack(bool visible);

#endif /* #ifndef MENU_H__37ACA2FE_C43D_4FA8_B7CF_6DD345F17CD1__INCLUDED */
