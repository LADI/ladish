/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 **************************************************************************
 * This file contains code of the D-Bus control interface helpers
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

#include "common.h"

#include "../dbus/interface.h"
#include "../dbus/error.h"

#define INTERFACE_NAME DBUS_NAME_BASE ".Control"

static void ladish_get_studio_list(method_call_t * call_ptr)
{
  DBusMessageIter iter, array_iter;
  //DBusMessageIter struct_iter, dict_iter;
  //struct list_head * node_ptr;
  //project_t * project_ptr;

  call_ptr->reply = dbus_message_new_method_return(call_ptr->message);
  if (call_ptr->reply == NULL)
  {
    goto fail;
  }

  dbus_message_iter_init_append(call_ptr->reply, &iter);

  if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(sa{sv})", &array_iter))
  {
    goto fail_unref;
  }

#if 0
  list_for_each (node_ptr, &g_server->all_projects)
  {
    project_ptr = list_entry(node_ptr, project_t, siblings_all);

    if (!dbus_message_iter_open_container(&array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter))
      goto fail_unref;

    if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &project_ptr->name))
    {
      dbus_message_iter_close_container(&iter, &array_iter);
      goto fail_unref;
    }

    if (!dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter))
      goto fail_unref;

    if (!maybe_add_dict_entry_string(&dict_iter, "Description", project_ptr->description))
      goto fail_unref;

    if (!add_dict_entry_uint32(&dict_iter, "Modification Time", project_ptr->last_modify_time))
      goto fail_unref;

    if (!dbus_message_iter_close_container(&struct_iter, &dict_iter))
      goto fail_unref;

    if (!dbus_message_iter_close_container(&array_iter, &struct_iter))
      goto fail_unref;
  }
#endif

  if (!dbus_message_iter_close_container(&iter, &array_iter))
  {
    goto fail_unref;
  }

  return;

fail_unref:
  dbus_message_unref(call_ptr->reply);
  call_ptr->reply = NULL;

fail:
  lash_error("Ran out of memory trying to construct method return");
}

static void ladish_load_studio(method_call_t * call_ptr)
{
  const char * name;

  dbus_error_init(&g_dbus_error);

  if (!dbus_message_get_args(call_ptr->message, &g_dbus_error, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  lash_info("Loading studio '%s'", name);
}

static void ladish_get_application_list(method_call_t * call_ptr)
{
  DBusMessageIter iter;
  DBusMessageIter array_iter;
#if 0
  DBusMessageIter struct_iter;
  DBusMessageIter dict_iter;
  struct list_head * node_ptr;
  struct lash_appdb_entry * entry_ptr;
#endif

  lash_info("Getting applications list");

  call_ptr->reply = dbus_message_new_method_return(call_ptr->message);
  if (call_ptr->reply == NULL)
  {
    goto fail;
  }

  dbus_message_iter_init_append(call_ptr->reply, &iter);

  if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(sa{sv})", &array_iter))
  {
    goto fail_unref;
  }

#if 0
  list_for_each(node_ptr, &g_server->appdb)
  {
    entry_ptr = list_entry(node_ptr, struct lash_appdb_entry, siblings);

    if (!dbus_message_iter_open_container(&array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter))
      goto fail_unref;

    if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, (const void *) &entry_ptr->name))
    {
      dbus_message_iter_close_container(&iter, &array_iter);
      goto fail_unref;
    }

    if (!dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter))
      goto fail_unref;

    if (!maybe_add_dict_entry_string(&dict_iter, "GenericName", entry_ptr->generic_name))
      goto fail_unref;

    if (!maybe_add_dict_entry_string(&dict_iter, "Comment", entry_ptr->comment))
      goto fail_unref;

    if (!maybe_add_dict_entry_string(&dict_iter, "Icon", entry_ptr->icon))
      goto fail_unref;

    if (!dbus_message_iter_close_container(&struct_iter, &dict_iter))
      goto fail_unref;

    if (!dbus_message_iter_close_container(&array_iter, &struct_iter))
      goto fail_unref;
  }
#endif

  if (!dbus_message_iter_close_container(&iter, &array_iter))
  {
    goto fail_unref;
  }

  return;

fail_unref:
  dbus_message_unref(call_ptr->reply);
  call_ptr->reply = NULL;
fail:
  return;
}

static void ladish_exit(method_call_t * call_ptr)
{
  lash_info("Exit command received through D-Bus");
  g_quit = true;
}

METHOD_ARGS_BEGIN(GetStudioList)
  METHOD_ARG_DESCRIBE("studio_list", "a(sa{sv})", DIRECTION_OUT)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(LoadStudio)
  METHOD_ARG_DESCRIBE("studio_name", "s", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("options", "a{sv}", DIRECTION_IN)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(GetApplicationList)
  METHOD_ARG_DESCRIBE("applications", "a(sa{sv})", DIRECTION_OUT)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Exit)
METHOD_ARGS_END

METHODS_BEGIN
  METHOD_DESCRIBE(GetStudioList, ladish_get_studio_list)
  METHOD_DESCRIBE(LoadStudio, ladish_load_studio)
  METHOD_DESCRIBE(GetApplicationList, ladish_get_application_list)
  METHOD_DESCRIBE(Exit, ladish_exit)
METHODS_END

SIGNALS_BEGIN
SIGNALS_END

/*
 * Interface description.
 */

INTERFACE_BEGIN(g_lashd_interface_control, INTERFACE_NAME)
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
  INTERFACE_EXPOSE_SIGNALS
INTERFACE_END
