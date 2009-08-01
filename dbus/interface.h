/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 **************************************************************************
 * This file contains declaration of macros used to define D-Bus interfaces
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

#ifndef __LASH_DBUS_INTERFACE_H__
#define __LASH_DBUS_INTERFACE_H__

#include <stdbool.h>

#include "types.h"
#include "method.h"
#include "signal.h"

struct _interface
{
  const char                *name;
  const interface_handler_t  handler;
  const method_t            *methods;
  const signal_t            *signals;
};

bool
interface_default_handler(const interface_t *interface,
                          method_call_t     *call);

#define INTERFACE_BEGIN(iface_var, iface_name) \
const struct _interface iface_var =            \
{                                              \
        .name = iface_name,

#define INTERFACE_DEFAULT_HANDLER              \
        .handler = interface_default_handler,

#define INTERFACE_HANDLER(handler_func)        \
        .handler = handler_func,

#define INTERFACE_EXPOSE_METHODS               \
        .methods = methods_dtor,

#define INTERFACE_EXPOSE_SIGNALS               \
        .signals = signals_dtor,

#define INTERFACE_END                          \
};

#endif /* __LASH_DBUS_INTERFACE_H__ */
