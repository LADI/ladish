/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 **************************************************************************
 * This file contains typdefs for internal structs (obsolete)
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

#ifndef __LASH_DBUS_TYPES_H__
#define __LASH_DBUS_TYPES_H__

#include <stdbool.h>

/* signal types */
typedef struct _signal_msg signal_msg_t;
typedef struct _signal_arg signal_arg_t;
typedef struct _signal     signal_t;

/* method types */
typedef struct _method_msg  method_msg_t;
typedef struct _method_call method_call_t;
typedef struct _method_arg  method_arg_t;
typedef struct _method      method_t;
typedef void (*method_handler_t) (method_call_t *call);

/* interface types */
typedef struct _interface interface_t;
typedef bool (*interface_handler_t) (const interface_t *, method_call_t *);

/* service types */
typedef struct _service service_t;

#endif /* __LASH_DBUS_TYPES_H__ */
