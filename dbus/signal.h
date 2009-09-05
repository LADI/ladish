/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 **************************************************************************
 * This file contains interface to D-Bus signal helpers
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

#ifndef __LASH_DBUS_SIGNAL_H__
#define __LASH_DBUS_SIGNAL_H__

struct dbus_signal_arg_descriptor
{
  const char * name;
  const char * type;
};

struct dbus_signal_descriptor
{
  const char * name;
  const struct dbus_signal_arg_descriptor * args;
};

void
dbus_signal_emit(
  DBusConnection * connection_ptr,
  const char * path,
  const char * interface,
  const char * name,
  const char * signature,
  ...);

#define SIGNAL_ARGS_BEGIN(signal_name, descr) \
static const struct dbus_signal_arg_descriptor signal_name ## _args_dtor[] = \
{

#define SIGNAL_ARG_DESCRIBE(arg_name, arg_type, descr)         \
        {                                                      \
                .name = arg_name,                              \
                .type = arg_type                               \
        },

#define SIGNAL_ARGS_END                                        \
        {                                                      \
                .name = NULL,                                  \
                .type = NULL                                   \
        }                                                      \
};

#define SIGNALS_BEGIN                                          \
static const struct dbus_signal_descriptor signals_dtor[] =    \
{

#define SIGNAL_DESCRIBE(signal_name)                           \
        {                                                      \
                .name = # signal_name,                         \
                .args = signal_name ## _args_dtor              \
        },

#define SIGNALS_END                                            \
        {                                                      \
                .name = NULL,                                  \
                .args = NULL                                   \
        }                                                      \
};

#endif /* __LASH_DBUS_SIGNAL_H__ */
