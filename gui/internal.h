/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010,2014 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains declarations of internal stuff used to glue gui modules together
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

#ifndef INTERNAL_H__725DFCCC_50F8_437A_9CD7_8B59125C6A11__INCLUDED
#define INTERNAL_H__725DFCCC_50F8_437A_9CD7_8B59125C6A11__INCLUDED

#include "common.h"

/* dbus.c */
bool dbus_init(void);
void dbus_uninit(void);

/* control.c */
void on_load_studio(const char * studio_name);
void on_delete_studio(const char * studio_name);

/* studio_list.c */
bool create_studio_lists(void);
void destroy_studio_lists(void);

void set_room_callbacks(void);

/* dialogs.c */
void init_dialogs(void);
bool name_dialog(const char * title, const char * object, const char * old_name, char ** new_name);

extern GtkWidget * g_main_win;

#endif /* #ifndef INTERNAL_H__725DFCCC_50F8_437A_9CD7_8B59125C6A11__INCLUDED */
