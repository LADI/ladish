/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the graph dictionary D-Bus helpers
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

#include "graph_dict_proxy.h"
#include "dbus/helpers.h"
#include "dbus_constants.h"

bool
lash_graph_dict_proxy_set(
  const char * service,
  const char * object,
  uint32_t object_type,
  uint64_t object_id,
  const char * key,
  const char * value)
{
  if (!dbus_call(service, object, IFACE_GRAPH_DICT, "Set", "utss", &object_type, &object_id, &key, &value, ""))
  {
    lash_error(IFACE_GRAPH_DICT ".Set() failed.");
    return false;
  }
  return true;
}

bool
lash_graph_dict_proxy_get(
  const char * service,
  const char * object,
  uint32_t object_type,
  uint64_t object_id,
  const char * key,
  char ** value_ptr_ptr)
{
  DBusMessage * reply_ptr;
  const char * reply_signature;
  DBusMessageIter iter;
  const char * cvalue_ptr;
  char * value_ptr;

  if (!dbus_call(service, object, IFACE_GRAPH_DICT, "Get", "uts", &object_type, &object_id, &key, NULL, &reply_ptr))
  {
    lash_error(IFACE_GRAPH_DICT ".Get() failed.");
    return false;
  }

  reply_signature = dbus_message_get_signature(reply_ptr);

  if (strcmp(reply_signature, "s") != 0)
  {
    lash_error("reply signature is '%s' but expected signature is 's'", reply_signature);
    dbus_message_unref(reply_ptr);
    return false;
  }

  dbus_message_iter_init(reply_ptr, &iter);
  dbus_message_iter_get_basic(&iter, &cvalue_ptr);
  value_ptr = strdup(cvalue_ptr);
  dbus_message_unref(reply_ptr);
  if (value_ptr == NULL)
  {
    lash_error("strdup() failed for dict value");
    return false;
  }
  *value_ptr_ptr = value_ptr;
  return true;
}

bool
lash_graph_dict_proxy_drop(
  const char * service,
  const char * object,
  uint32_t object_type,
  uint64_t object_id,
  const char * key)
{
  if (!dbus_call(service, object, IFACE_GRAPH_DICT, "Drop", "uts", &object_type, &object_id, &key, ""))
  {
    lash_error(IFACE_GRAPH_DICT ".Drop() failed.");
    return false;
  }
  return true;
}
