/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 **************************************************************************
 * This file contains D-Bus methods helpers
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

#include "../common.h"
#include "../common/safety.h"
#include "helpers.h"
#include "method.h"

/*
 * Construct a void method return.
 *
 * The operation can only fail due to lack of memory, in which case
 * there's no sense in trying to construct an error return. Instead,
 * call_ptr->reply will be set to NULL and handled in send_method_return().
 */
void
method_return_new_void(struct dbus_method_call * call_ptr)
{
  if (!(call_ptr->reply = dbus_message_new_method_return(call_ptr->message))) {
    lash_error("Ran out of memory trying to construct method return");
  }
}

/*
 * Construct a method return which holds a single argument or, if
 * the type parameter is DBUS_TYPE_INVALID, no arguments at all
 * (a void message).
 *
 * The operation can only fail due to lack of memory, in which case
 * there's no sense in trying to construct an error return. Instead,
 * call_ptr->reply will be set to NULL and handled in send_method_return().
 */
void
method_return_new_single(struct dbus_method_call * call_ptr,
                         int            type,
                         const void    *arg)
{
  if (!call_ptr || !arg) {
    lash_error("Invalid arguments");
    return;
  }

  call_ptr->reply = dbus_message_new_method_return(call_ptr->message);

  if (!call_ptr->reply)
    goto fail_no_mem;

  /* Void method return requested by caller. */
  // TODO: do we really need this?
  if (type == DBUS_TYPE_INVALID)
    return;

  /* Prevent crash on NULL input string. */
  if (type == DBUS_TYPE_STRING && !(*((const char **) arg)))
    *((const char **) arg) = "";

  DBusMessageIter iter;

  dbus_message_iter_init_append(call_ptr->reply, &iter);

  if (dbus_message_iter_append_basic(&iter, type, arg))
    return;

  dbus_message_unref(call_ptr->reply);
  call_ptr->reply = NULL;

fail_no_mem:
  lash_error("Ran out of memory trying to construct method return");
}

void
method_return_new_valist(struct dbus_method_call * call_ptr,
                         int            type,
                                        ...)
{
  if (!call_ptr) {
    lash_error("Call pointer is NULL");
    return;
  }

  if (type == DBUS_TYPE_INVALID) {
    lash_error("No argument(s) supplied");
    return;
  }

  va_list argp;

  call_ptr->reply = dbus_message_new_method_return(call_ptr->message);
  if (!call_ptr->reply)
    goto fail_no_mem;

  va_start(argp, type);

  if (dbus_message_append_args_valist(call_ptr->reply, type, argp)) {
    va_end(argp);
    return;
  }

  va_end(argp);

  dbus_message_unref(call_ptr->reply);
  call_ptr->reply = NULL;

fail_no_mem:
  lash_error("Ran out of memory trying to construct method return");
}

/*
 * Send a method return.
 *
 * If call_ptr->reply is NULL, i.e. a previous attempt to construct
 * a return has failed, attempt to send a void return.
 */
void
method_return_send(struct dbus_method_call * call_ptr)
{
  if (call_ptr->reply) {
  retry_send:
    if (!dbus_connection_send(call_ptr->connection, call_ptr->reply, NULL))
      lash_error("Ran out of memory trying to queue "
                 "method return");
    else
      dbus_connection_flush(call_ptr->connection);

    dbus_message_unref(call_ptr->reply);
    call_ptr->reply = NULL;
  } else {
    lash_debug("Message was NULL, trying to construct a void return");

    if ((call_ptr->reply = dbus_message_new_method_return(call_ptr->message))) {
      lash_debug("Constructed a void return, trying to queue it");
      goto retry_send;
    } else {
      lash_error("Failed to construct method return!");
    }
  }
}

bool
method_return_verify(DBusMessage  *msg,
                     const char  **str)
{
  if (!msg || dbus_message_get_type(msg) != DBUS_MESSAGE_TYPE_ERROR)
    return true;

  const char *ptr;

  if (!dbus_message_get_args(msg, &g_dbus_error,
                             DBUS_TYPE_STRING, &ptr,
                             DBUS_TYPE_INVALID)) {
    lash_error("Cannot read description from D-Bus error message: %s ",
               g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    ptr = NULL;
  }

  if (str)
    *str = ptr;

  return false;
}

/*
 * Append a variant type to a D-Bus message.
 * Return false if something fails, true otherwise.
 */
bool
method_iter_append_variant(DBusMessageIter *iter,
                           int              type,
                           const void      *arg)
{
  DBusMessageIter sub_iter;
  char s[2];

  s[0] = (char) type;
  s[1] = '\0';

  /* Open a variant container. */
  if (!dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT,
                                        (const char *) s, &sub_iter))
    return false;

  /* Append the supplied value. */
  if (!dbus_message_iter_append_basic(&sub_iter, type, arg)) {
    dbus_message_iter_close_container(iter, &sub_iter);
    return false;
  }

  /* Close the container. */
  if (!dbus_message_iter_close_container(iter, &sub_iter))
    return false;

  return true;
}

static __inline__ bool
method_iter_append_variant_raw(DBusMessageIter *iter,
                               const void      *buf,
                               int              len)
{
  DBusMessageIter variant_iter, array_iter;

  /* Open a variant container. */
  if (!dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT,
                                        "ay", &variant_iter))
    return false;

  /* Open an array container. */
  if (!dbus_message_iter_open_container(&variant_iter, DBUS_TYPE_ARRAY,
                                        "y", &array_iter))
    goto fail;

  /* Append the supplied data. */
  if (!dbus_message_iter_append_fixed_array(&array_iter, DBUS_TYPE_BYTE, buf, len)) {
    dbus_message_iter_close_container(&variant_iter, &array_iter);
    goto fail;
  }

  /* Close the containers. */
  if (!dbus_message_iter_close_container(&variant_iter, &array_iter))
    goto fail;
  else if (!dbus_message_iter_close_container(iter, &variant_iter))
    return false;

  return true;

fail:
  dbus_message_iter_close_container(iter, &variant_iter);
  return false;
}

bool
method_iter_append_dict_entry(DBusMessageIter *iter,
                              int              type,
                              const char      *key,
                              const void      *value,
                              int              length)
{
  DBusMessageIter dict_iter;

  if (!dbus_message_iter_open_container(iter, DBUS_TYPE_DICT_ENTRY,
                                        NULL, &dict_iter))
    return false;

  if (!dbus_message_iter_append_basic(&dict_iter, DBUS_TYPE_STRING, &key))
    goto fail;

  if (type == '-') {
    if (!method_iter_append_variant_raw(&dict_iter, value, length))
      goto fail;
  } else if (!method_iter_append_variant(&dict_iter, type, value))
    goto fail;

  if (!dbus_message_iter_close_container(iter, &dict_iter))
    return false;

  return true;

fail:
  dbus_message_iter_close_container(iter, &dict_iter);
  return false;
}

bool
method_iter_get_dict_entry(DBusMessageIter  *iter,
                           const char      **key_ptr,
                           void             *value_ptr,
                           int              *type_ptr,
                           int              *size_ptr)
{
  if (!iter || !key_ptr || !value_ptr || !type_ptr) {
    lash_error("Invalid arguments");
    return false;
  }

  DBusMessageIter dict_iter, variant_iter;

  if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_DICT_ENTRY) {
    lash_error("Iterator does not point to a dict entry container");
    return false;
  }

  dbus_message_iter_recurse(iter, &dict_iter);

  if (dbus_message_iter_get_arg_type(&dict_iter) != DBUS_TYPE_STRING) {
    lash_error("Cannot find key in dict entry container");
    return false;
  }

  dbus_message_iter_get_basic(&dict_iter, key_ptr);

  if (!dbus_message_iter_next(&dict_iter)
      || dbus_message_iter_get_arg_type(&dict_iter) != DBUS_TYPE_VARIANT) {
    lash_error("Cannot find variant container in dict entry");
    return false;
  }

  dbus_message_iter_recurse(&dict_iter, &variant_iter);

  *type_ptr = dbus_message_iter_get_arg_type(&variant_iter);
  if (*type_ptr == DBUS_TYPE_INVALID) {
    lash_error("Cannot find value in variant container");
    return false;
  }

  if (*type_ptr == DBUS_TYPE_ARRAY) {
    DBusMessageIter array_iter;
    int n;

    if (dbus_message_iter_get_element_type(&variant_iter)
        != DBUS_TYPE_BYTE) {
      lash_error("Dict entry value is a non-byte array");
      return false;
    }
    *type_ptr = '-';

    dbus_message_iter_recurse(&variant_iter, &array_iter);
    dbus_message_iter_get_fixed_array(&array_iter, value_ptr, &n);

    if (size_ptr)
      *size_ptr = n;
  } else
    dbus_message_iter_get_basic(&variant_iter, value_ptr);

  return true;
}

/* EOF */
