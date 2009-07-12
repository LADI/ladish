/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *   Copyright (C) 2008 Nedko Arnaudov
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __LASH_DBUS_ERROR_H__
#define __LASH_DBUS_ERROR_H__

#include "types.h"

#define LASH_DBUS_ERROR_UNKNOWN_METHOD      "org.nongnu.LASH.Error.UnknownMethod"
#define LASH_DBUS_ERROR_SERVER_NOT_RUNNING  "org.nongnu.LASH.Error.ServerNotRunning"
#define LASH_DBUS_ERROR_INVALID_ARGS        "org.nongnu.LASH.Error.InvalidArgs"
#define LASH_DBUS_ERROR_GENERIC             "org.nongnu.LASH.Error.Generic"
#define LASH_DBUS_ERROR_FATAL               "org.nongnu.LASH.Error.Fatal"
#define LASH_DBUS_ERROR_UNKNOWN_PROJECT     "org.nongnu.LASH.Error.UnknownProject"
#define LASH_DBUS_ERROR_UNKNOWN_CLIENT      "org.nongnu.LASH.Error.UnknownClient"
#define LASH_DBUS_ERROR_INVALID_CLIENT_ID   "org.nongnu.LASH.Error.InvalidClientId"
#define LASH_DBUS_ERROR_UNFINISHED_TASK     "org.nongnu.LASH.Error.UnfinishedTask"
#define LASH_DBUS_ERROR_INVALID_TASK        "org.nongnu.LASH.Error.InvalidTask"

void
lash_dbus_error(method_call_t *call_ptr,
                const char    *err_name,
                const char    *format,
                               ...);

#endif /* __LASH_DBUS_ERROR_H__ */
