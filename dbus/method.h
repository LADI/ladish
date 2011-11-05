/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008,2009,2011 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 **************************************************************************
 * This file contains interface to D-Bus methods helpers
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

#ifndef __CDBUS_METHOD_H__
#define __CDBUS_METHOD_H__

struct cdbus_method_call
{
  DBusConnection * connection;
  const char * method_name;
  DBusMessage * message;
  DBusMessage * reply;
  const struct cdbus_interface_descriptor * iface;
  void * iface_context;
};

struct cdbus_method_arg_descriptor
{
  const char * name;
  const char * type;
  const bool direction_in;      /* false == out, true == in */
};

typedef void (* cdbus_method_handler)(struct cdbus_method_call * call_ptr);

struct cdbus_method_descriptor
{
  const char * name;
  const cdbus_method_handler handler;
  const struct cdbus_method_arg_descriptor * args;
};

void cdbus_error(struct cdbus_method_call * call_ptr, const char * err_name, const char * format, ...);

void cdbus_method_return_new_void(struct cdbus_method_call * call_ptr);
void cdbus_method_return_new_single(struct cdbus_method_call * call_ptr, int type, const void * arg);
void cdbus_method_return_new_valist(struct cdbus_method_call * call_ptr, int type, ...);
bool cdbus_method_return_verify(DBusMessage * msg, const char ** str);
void cdbus_method_return_send(struct cdbus_method_call * call_ptr);
void cdbus_method_default_handler(DBusPendingCall * pending, void * data);

#define METHOD_ARGS_BEGIN(method_name, descr) \
static const struct cdbus_method_arg_descriptor method_name ## _args_dtor[] = \
{

#define METHOD_ARG_DESCRIBE_IN(arg_name, arg_type, descr)       \
        {                                                       \
                .name = arg_name,                               \
                .type = arg_type,                               \
                .direction_in = true                            \
        },

#define METHOD_ARG_DESCRIBE_OUT(arg_name, arg_type, descr)      \
        {                                                       \
                .name = arg_name,                               \
                .type = arg_type,                               \
                .direction_in = false                           \
        },

#define METHOD_ARGS_END                                         \
        {                                                       \
                .name = NULL,                                   \
        }                                                       \
};

#define METHODS_BEGIN                                           \
static const struct cdbus_method_descriptor methods_dtor[] =    \
{

#define METHOD_DESCRIBE(method_name, handler_name)              \
        {                                                       \
                .name = # method_name,                          \
                .handler = handler_name,                        \
                .args = method_name ## _args_dtor               \
        },

#define METHODS_END                                             \
        {                                                       \
                .name = NULL,                                   \
                .handler = NULL,                                \
                .args = NULL                                    \
        }                                                       \
};

#endif /* __CDBUS_METHOD_H__ */
