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

#ifndef __LASH_DBUS_METHOD_H__
#define __LASH_DBUS_METHOD_H__

#include <stdbool.h>
#include <dbus/dbus.h>

#include "dbus/types.h"

#define DIRECTION_OUT (0)
#define DIRECTION_IN  (1)

struct _method_msg
{
	const service_t               *service;
	DBusMessage                   *message;
	void                          *context;
	DBusFreeFunction               context_free_func;
	DBusPendingCallNotifyFunction  return_handler;
};

struct _method_call
{
	DBusConnection    *connection;
	const char        *method_name;
	DBusMessage       *message;
	DBusMessage       *reply;
	const interface_t *interface;
	void              *context;
};

struct _method_arg
{
	const char *name;
	const char *type;
	const int   direction;  /* 0 == out, 1 == in */
};

struct _method
{
	const char             *name;
	const method_handler_t  handler;
	const method_arg_t     *args;
};

void
method_return_new_void(method_call_t *call);

void
method_return_new_single(method_call_t *call,
                         int            type,
                         const void    *arg);

void
method_return_new_valist(method_call_t *call,
                         int            type,
                                        ...);

bool
method_return_verify(DBusMessage  *msg,
                     const char  **str);

bool
method_iter_append_variant(DBusMessageIter *iter,
                           int              type,
                           const void      *arg);

bool
method_iter_append_dict_entry(DBusMessageIter *iter,
                              int              type,
                              const char      *key,
                              const void      *value,
                              int              length);

bool
method_send(method_msg_t *call,
            bool          will_block);

void
method_return_send(method_call_t *call);

void
method_default_handler(DBusPendingCall *pending,
                       void            *data);

bool
method_call_init(method_msg_t                  *call,
                 service_t                     *service,
                 void                          *return_context,
                 DBusPendingCallNotifyFunction  return_handler,
                 const char                    *destination,
                 const char                    *path,
                 const char                    *interface,
                 const char                    *method);

bool
method_call_new_void(service_t                     *service,
                     void                          *return_context,
                     DBusPendingCallNotifyFunction  return_handler,
                     bool                           will_block,
                     const char                    *destination,
                     const char                    *path,
                     const char                    *interface,
                     const char                    *method);

bool
method_call_new_single(service_t                     *service,
                       void                          *return_context,
                       DBusPendingCallNotifyFunction  return_handler,
                       bool                           will_block,
                       const char                    *destination,
                       const char                    *path,
                       const char                    *interface,
                       const char                    *method,
                       int                            type,
                       const void                    *arg);

bool
method_call_new_valist(service_t                     *service,
                       void                          *return_context,
                       DBusPendingCallNotifyFunction  return_handler,
                       bool                           will_block,
                       const char                    *destination,
                       const char                    *path,
                       const char                    *interface,
                       const char                    *method,
                       int                            type,
                                                       ...);

bool
method_iter_get_args(DBusMessageIter *iter,
                                      ...);

bool
method_iter_get_dict_entry(DBusMessageIter  *iter,
                           const char      **key_ptr,
                           void             *value_ptr,
                           int              *type_ptr,
                           int              *size_ptr);

#define METHOD_ARGS_BEGIN(method_name)                          \
static const struct _method_arg method_name ## _args_dtor[] =   \
{

#define METHOD_ARG_DESCRIBE(arg_name, arg_type, arg_direction)  \
        {                                                       \
                .name = arg_name,                               \
                .type = arg_type,                               \
                .direction = arg_direction                      \
        },

#define METHOD_ARGS_END                                         \
        {                                                       \
                .name = NULL,                                   \
                .type = NULL,                                   \
                .direction = 0                                  \
        }                                                       \
};

#define METHODS_BEGIN                                           \
static const struct _method methods_dtor[] =                    \
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

#endif /* __LASH_DBUS_METHOD_H__ */
