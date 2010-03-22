/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 **************************************************************************
 * This file contains interface to the D-Bus control interface helpers
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

#ifndef __LASHD_DBUS_IFACE_CONTROL_H__
#define __LASHD_DBUS_IFACE_CONTROL_H__

#include "room.h"

extern const struct dbus_interface_descriptor g_lashd_interface_control;

void emit_studio_appeared(void);
void emit_studio_disappeared(void);
void emit_clean_exit(void);

bool room_templates_init(void);
void room_templates_uninit(void);
ladish_room_handle find_room_template_by_name(const char * name);
ladish_room_handle find_room_template_by_uuid(const uuid_t uuid_ptr);

#endif /* __LASHD_DBUS_IFACE_CONTROL_H__ */
