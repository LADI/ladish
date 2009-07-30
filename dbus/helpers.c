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

  method_iter_append_variant(&dict_entry_iter, DBUS_TYPE_STRING, &value);

  if (!dbus_message_iter_close_container(dict_iter_ptr, &dict_entry_iter))
  {
    return false;
  }

  return true;
}

bool dbus_add_dict_entry_uint32(DBusMessageIter * dict_iter_ptr, const char * key, dbus_uint32_t value)
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

  method_iter_append_variant(&dict_entry_iter, DBUS_TYPE_UINT32, &value);

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

  method_iter_append_variant(&dict_entry_iter, DBUS_TYPE_BOOLEAN, &value);

  if (!dbus_message_iter_close_container(dict_iter_ptr, &dict_entry_iter))
  {
    return false;
  }

  return true;
}
