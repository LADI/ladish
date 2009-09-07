/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 **************************************************************************
 * This file contains code of the D-Bus helpers
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

#include <stdbool.h>
#include <dbus/dbus.h>
#include <string.h>
#include <assert.h>

#include "helpers.h"
#include "method.h"
#include "../common/debug.h"

DBusConnection * g_dbus_connection;
DBusError g_dbus_error;

bool dbus_iter_get_dict_entry(DBusMessageIter * iter, const char ** key_ptr, void * value_ptr, int * type_ptr, int * size_ptr)
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

/*
 * Append a variant type to a D-Bus message.
 * Return false if something fails, true otherwise.
 */
bool dbus_iter_append_variant(DBusMessageIter * iter, int type, const void * arg)
{
  DBusMessageIter sub_iter;
  char s[2];

  s[0] = (char)type;
  s[1] = '\0';

  /* Open a variant container. */
  if (!dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, (const char *)s, &sub_iter))
    return false;

  /* Append the supplied value. */
  if (!dbus_message_iter_append_basic(&sub_iter, type, arg))
  {
    dbus_message_iter_close_container(iter, &sub_iter);
    return false;
  }

  /* Close the container. */
  if (!dbus_message_iter_close_container(iter, &sub_iter))
    return false;

  return true;
}

static __inline__ bool dbus_iter_append_variant_raw(DBusMessageIter * iter, const void * buf, int len)
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

bool dbus_iter_append_dict_entry(DBusMessageIter * iter, int type, const char * key, const void * value, int length)
{
  DBusMessageIter dict_iter;

  if (!dbus_message_iter_open_container(iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_iter))
    return false;

  if (!dbus_message_iter_append_basic(&dict_iter, DBUS_TYPE_STRING, &key))
    goto fail;

  if (type == '-')
  {
    if (!dbus_iter_append_variant_raw(&dict_iter, value, length))
      goto fail;
  }
  else if (!dbus_iter_append_variant(&dict_iter, type, value))
  {
    goto fail;
  }

  if (!dbus_message_iter_close_container(iter, &dict_iter))
    return false;

  return true;

fail:
  dbus_message_iter_close_container(iter, &dict_iter);
  return false;
}

bool dbus_maybe_add_dict_entry_string(DBusMessageIter *dict_iter_ptr, const char * key, const char * value)
{
  DBusMessageIter dict_entry_iter;

  if (value == NULL)
  {
    return true;
  }

  if (!dbus_message_iter_open_container(dict_iter_ptr, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter))
  {
    return false;
  }

  if (!dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, (const void *) &key))
  {
    dbus_message_iter_close_container(dict_iter_ptr, &dict_entry_iter);
    return false;
  }

  dbus_iter_append_variant(&dict_entry_iter, DBUS_TYPE_STRING, &value);

  if (!dbus_message_iter_close_container(dict_iter_ptr, &dict_entry_iter))
  {
    return false;
  }

  return true;
}

bool dbus_add_dict_entry_uint32(DBusMessageIter * dict_iter_ptr, const char * key, dbus_uint32_t value)
{
  DBusMessageIter dict_entry_iter;

  if (!dbus_message_iter_open_container(dict_iter_ptr, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter))
  {
    return false;
  }

  if (!dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, (const void *) &key))
  {
    dbus_message_iter_close_container(dict_iter_ptr, &dict_entry_iter);
    return false;
  }

  dbus_iter_append_variant(&dict_entry_iter, DBUS_TYPE_UINT32, &value);

  if (!dbus_message_iter_close_container(dict_iter_ptr, &dict_entry_iter))
  {
    return false;
  }

  return true;
}

bool dbus_add_dict_entry_bool(DBusMessageIter * dict_iter_ptr, const char * key, dbus_bool_t value)
{
  DBusMessageIter dict_entry_iter;

  if (!dbus_message_iter_open_container(dict_iter_ptr, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter))
  {
    return false;
  }

  if (!dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, (const void *) &key))
  {
    dbus_message_iter_close_container(dict_iter_ptr, &dict_entry_iter);
    return false;
  }

  dbus_iter_append_variant(&dict_entry_iter, DBUS_TYPE_BOOLEAN, &value);

  if (!dbus_message_iter_close_container(dict_iter_ptr, &dict_entry_iter))
  {
    return false;
  }

  return true;
}

bool
dbus_call(
  const char * service,
  const char * object,
  const char * iface,
  const char * method,
  const char * input_signature,
  ...)
{
  DBusMessageIter iter;
  DBusMessage * request_ptr;
  DBusMessage * reply_ptr;
  const char * output_signature;
  const char * reply_signature;
  va_list ap;
  bool ret;
  void * parameter_ptr;
  int type;
  DBusSignatureIter sig_iter;

  //lash_info("dbus_call('%s', '%s', '%s', '%s')", service, object, iface, method);

  ret = false;
  va_start(ap, input_signature);

  if (input_signature != NULL)
  {
    if (!dbus_signature_validate(input_signature, NULL))
    {
      lash_error("input signature '%s' is invalid", input_signature);
      goto fail;
    }

    dbus_signature_iter_init(&sig_iter, input_signature);

    request_ptr = dbus_message_new_method_call(service, object, iface, method);
    if (request_ptr == NULL)
    {
      lash_error("dbus_message_new_method_call() failed.");
      goto fail;
    }

    dbus_message_iter_init_append(request_ptr, &iter);

    while (*input_signature != '\0')
    {
      type = dbus_signature_iter_get_current_type(&sig_iter);
      if (!dbus_type_is_basic(type))
      {
        lash_error("non-basic input parameter '%c' (%d)", *input_signature, type);
        goto fail;
      }

      parameter_ptr = va_arg(ap, void *);

      if (!dbus_message_iter_append_basic(&iter, type, parameter_ptr))
      {
        lash_error("dbus_message_iter_append_basic() failed.");
        goto fail;
      }

      dbus_signature_iter_next(&sig_iter);
      input_signature++;
    }
  }
  else
  {
    request_ptr = va_arg(ap, DBusMessage *);
  }

  output_signature = va_arg(ap, const char *);

  reply_ptr = dbus_connection_send_with_reply_and_block(
    g_dbus_connection,
    request_ptr,
    DBUS_CALL_DEFAULT_TIMEOUT,
    &g_dbus_error);

  if (input_signature != NULL)
  {
    dbus_message_unref(request_ptr);
  }

  if (reply_ptr == NULL)
  {
    lash_error("calling method '%s' failed, error is '%s'", method, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    goto fail;
  }

  if (output_signature != NULL)
  {
    reply_signature = dbus_message_get_signature(reply_ptr);

    if (strcmp(reply_signature, output_signature) != 0)
    {
      lash_error("reply signature is '%s' but expected signature is '%s'", reply_signature, output_signature);
    }

    dbus_message_iter_init(reply_ptr, &iter);

    while (*output_signature++ != '\0')
    {
      assert(dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_INVALID); /* we've checked the signature, this should not happen */
      parameter_ptr = va_arg(ap, void *);
      dbus_message_iter_get_basic(&iter, parameter_ptr);
      dbus_message_iter_next(&iter);
    }

    assert(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_INVALID); /* we've checked the signature, this should not happen */
    dbus_message_unref(reply_ptr);
  }
  else
  {
    parameter_ptr = va_arg(ap, DBusMessage **);
    *(DBusMessage **)parameter_ptr = reply_ptr;
  }

  ret = true;

fail:
  va_end(ap);
  return ret;
}

static
const char *
compose_signal_match(
  const char * service,
  const char * object,
  const char * iface,
  const char * signal)
{
  static char rule[1024];
  snprintf(rule, sizeof(rule), "type='signal',sender='%s',path='%s',interface='%s',member='%s'", service, object, iface, signal);
  return rule;
}

bool
dbus_register_object_signal_handler(
  DBusConnection * connection,
  const char * service,
  const char * object,
  const char * iface,
  const char * const * signals,
  DBusHandleMessageFunction handler,
  void * handler_data)
{
  const char * const * signal;

  for (signal = signals; *signal != NULL; signal++)
  {
    dbus_bus_add_match(connection, compose_signal_match(service, object, iface, *signal), &g_dbus_error);
    if (dbus_error_is_set(&g_dbus_error))
    {
      lash_error("Failed to add D-Bus match rule: %s", g_dbus_error.message);
      dbus_error_free(&g_dbus_error);
      return false;
    }
  }

  dbus_connection_add_filter(g_dbus_connection, handler, handler_data, NULL);

  return true;
}

bool
dbus_unregister_object_signal_handler(
  DBusConnection * connection,
  const char * service,
  const char * object,
  const char * iface,
  const char * const * signals,
  DBusHandleMessageFunction handler,
  void * handler_data)
{
  const char * const * signal;

  for (signal = signals; *signal != NULL; signal++)
  {
    dbus_bus_remove_match(connection, compose_signal_match(service, object, iface, *signal), &g_dbus_error);
    if (dbus_error_is_set(&g_dbus_error))
    {
      lash_error("Failed to add D-Bus match rule: %s", g_dbus_error.message);
      dbus_error_free(&g_dbus_error);
      return false;
    }
  }

  dbus_connection_remove_filter(g_dbus_connection, handler, handler_data);

  return true;
}
