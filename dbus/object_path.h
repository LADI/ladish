/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008,2009,2010,2011 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 **************************************************************************
 * This file contains interface to D-Bus object path helpers
 **************************************************************************
 *
 * Licensed under the Academic Free License version 2.1
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

#ifndef __CDBUS_OBJECT_PATH_H__
#define __CDBUS_OBJECT_PATH_H__

typedef struct cdbus_object_path_tag { int unused; } * cdbus_object_path;

cdbus_object_path cdbus_object_path_new(const char * name, const struct cdbus_interface_descriptor * iface, ...);
bool cdbus_object_path_register(DBusConnection * connection_ptr, cdbus_object_path opath);
void cdbus_object_path_unregister(DBusConnection * connection_ptr, cdbus_object_path opath);
void cdbus_object_path_destroy(DBusConnection * connection_ptr, cdbus_object_path opath);

#endif /* __CDBUS_OBJECT_PATH_H__ */
