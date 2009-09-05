/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 **************************************************************************
 * This file contains declaration of macros used to define D-Bus interfaces
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

#ifndef __LASH_DBUS_INTERFACE_H__
#define __LASH_DBUS_INTERFACE_H__

typedef bool (* dbus_interface_handler)(const struct dbus_interface_descriptor *, struct dbus_method_call *);

struct dbus_interface_descriptor
{
  const char * name;
  dbus_interface_handler handler;
  const struct dbus_method_descriptor * methods;
  const struct dbus_signal_descriptor * signals;
};

bool dbus_interface_default_handler(const struct dbus_interface_descriptor * interface, struct dbus_method_call * call_ptr);

#define INTERFACE_BEGIN(iface_var, iface_name)     \
const struct dbus_interface_descriptor iface_var = \
{                                                  \
  .name = iface_name,

#define INTERFACE_DEFAULT_HANDLER               \
  .handler = dbus_interface_default_handler,

#define INTERFACE_HANDLER(handler_func)         \
  .handler = handler_func,

#define INTERFACE_EXPOSE_METHODS                \
  .methods = methods_dtor,

#define INTERFACE_EXPOSE_SIGNALS                \
  .signals = signals_dtor,

#define INTERFACE_END                           \
};

#endif /* __LASH_DBUS_INTERFACE_H__ */
