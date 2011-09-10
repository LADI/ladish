/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008,2009,2011 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 **************************************************************************
 * This file contains interface to dbus error helpers
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

#ifndef __LASH_DBUS_ERROR_H__
#define __LASH_DBUS_ERROR_H__

#define LASH_DBUS_ERROR_UNKNOWN_METHOD      DBUS_NAME_BASE ".Error.UnknownMethod"
#define LASH_DBUS_ERROR_SERVER_NOT_RUNNING  DBUS_NAME_BASE ".Error.ServerNotRunning"
#define LASH_DBUS_ERROR_INVALID_ARGS        DBUS_NAME_BASE ".Error.InvalidArgs"
#define LASH_DBUS_ERROR_GENERIC             DBUS_NAME_BASE ".Error.Generic"
#define LASH_DBUS_ERROR_FATAL               DBUS_NAME_BASE ".Error.Fatal"
#define LASH_DBUS_ERROR_UNKNOWN_PROJECT     DBUS_NAME_BASE ".Error.UnknownProject"
#define LASH_DBUS_ERROR_UNKNOWN_CLIENT      DBUS_NAME_BASE ".Error.UnknownClient"
#define LASH_DBUS_ERROR_INVALID_CLIENT_ID   DBUS_NAME_BASE ".Error.InvalidClientId"
#define LASH_DBUS_ERROR_UNFINISHED_TASK     DBUS_NAME_BASE ".Error.UnfinishedTask"
#define LASH_DBUS_ERROR_INVALID_TASK        DBUS_NAME_BASE ".Error.InvalidTask"
#define LASH_DBUS_ERROR_KEY_NOT_FOUND       DBUS_NAME_BASE ".Error.KeyNotFound"

void lash_dbus_error(struct cdbus_method_call * call_ptr, const char * err_name, const char * format, ...);

#endif /* __LASH_DBUS_ERROR_H__ */
