/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to "create room" dialog
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

#ifndef CREATE_ROOM_DIALOG_H__4CB9233D_A861_4DEF_959E_D560E6118452__INCLUDED
#define CREATE_ROOM_DIALOG_H__4CB9233D_A861_4DEF_959E_D560E6118452__INCLUDED

#include "common.h"

void create_room_dialog_init(void);
void create_room_dialog_uninit(void);
bool create_room_dialog_run(char ** name_ptr_ptr, char ** template_ptr_ptr);

#endif /* #ifndef CREATE_ROOM_DIALOG_H__4CB9233D_A861_4DEF_959E_D560E6118452__INCLUDED */
