/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
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

#include "../dbus/error.h"
#include "control.h"
#include "../dbus_constants.h"
#include "studio_internal.h"
#include "room.h"
#include "../lib/wkports.h"

#define INTERFACE_NAME IFACE_CONTROL

/* 805e485f-65e4-4c37-a959-2a3b60b3c270 */
UUID_DEFINE(empty_room,0x80,0x5E,0x48,0x5F,0x65,0xE4,0x4C,0x37,0xA9,0x59,0x2A,0x3B,0x60,0xB3,0xC2,0x70);

/* c603f2a0-d96a-493e-a8cf-55581d950aa9 */
UUID_DEFINE(basic_room,0xC6,0x03,0xF2,0xA0,0xD9,0x6A,0x49,0x3E,0xA8,0xCF,0x55,0x58,0x1D,0x95,0x0A,0xA9);

static struct list_head g_rooms;

bool create_builtin_rooms(void)
{
  ladish_room_handle room;

  if (!ladish_room_create(empty_room, "Empty", NULL, NULL, &room))
  {
    log_error("ladish_room_create() failed.");
    return false;
  }

  list_add_tail(ladish_room_get_list_node(room), &g_rooms);

  if (!ladish_room_create(basic_room, "Basic", NULL, NULL, &room))
  {
    log_error("ladish_room_create() failed.");
    return false;
  }

  list_add_tail(ladish_room_get_list_node(room), &g_rooms);

  return true;
}

bool create_rooms(void)
{
  if (!create_builtin_rooms())
  {
    return false;
  }

  return true;
}

void maybe_create_rooms(void)
{
  if (list_empty(&g_rooms))
  {
    create_rooms();
  }
}

bool rooms_init(void)
{
  INIT_LIST_HEAD(&g_rooms);

  return true;
}

void rooms_uninit(void)
{
  struct list_head * node_ptr;
  ladish_room_handle room;

  while (!list_empty(&g_rooms))
  {
    node_ptr = g_rooms.next;
    list_del(node_ptr);
    room = ladish_room_from_list_node(node_ptr);
    ladish_room_destroy(room);
  }
}

bool rooms_enum(void * context, bool (* callback)(void * context, ladish_room_handle room))
{
  struct list_head * node_ptr;

  maybe_create_rooms();

  list_for_each(node_ptr, &g_rooms)
  {
    if (!callback(context, ladish_room_from_list_node(node_ptr)))
    {
      return false;
    }
  }

  return true;
}

ladish_room_handle find_room_template_by_name(const char * name)
{
  ladish_room_handle room;
  struct list_head * node_ptr;

  maybe_create_rooms();

  list_for_each(node_ptr, &g_rooms)
  {
    room = ladish_room_from_list_node(node_ptr);
    if (strcmp(ladish_room_get_name(room), name) == 0)
    {
      return room;
    }
  }

  return NULL;
}

ladish_room_handle find_room_template_by_uuid(const uuid_t uuid_ptr)
{
  ladish_room_handle room;
  struct list_head * node_ptr;
  uuid_t uuid;

  maybe_create_rooms();

  list_for_each(node_ptr, &g_rooms)
  {
    room = ladish_room_from_list_node(node_ptr);
    ladish_room_get_uuid(room, uuid);
    if (uuid_compare(uuid, uuid_ptr) == 0)
    {
      return room;
    }
  }

  return NULL;
}

static void ladish_is_studio_loaded(struct dbus_method_call * call_ptr)
{
  DBusMessageIter iter;
  dbus_bool_t is_loaded;

  is_loaded = studio_is_loaded();

  call_ptr->reply = dbus_message_new_method_return(call_ptr->message);
  if (call_ptr->reply == NULL)
  {
    goto fail;
  }

  dbus_message_iter_init_append(call_ptr->reply, &iter);

  if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_BOOLEAN, &is_loaded))
  {
    goto fail_unref;
  }

  return;

fail_unref:
  dbus_message_unref(call_ptr->reply);
  call_ptr->reply = NULL;

fail:
  log_error("Ran out of memory trying to construct method return");
}

#define array_iter_ptr ((DBusMessageIter *)context)

static bool get_studio_list_callback(void * call_ptr, void * context, const char * studio, uint32_t modtime)
{
  DBusMessageIter struct_iter;
  DBusMessageIter dict_iter;
  bool ret;

  ret = false;

  if (!dbus_message_iter_open_container(array_iter_ptr, DBUS_TYPE_STRUCT, NULL, &struct_iter))
    goto exit;

  if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &studio))
    goto close_struct;

  if (!dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter))
    goto close_struct;

/*   if (!maybe_add_dict_entry_string(&dict_iter, "Description", xxx)) */
/*     goto close_dict; */

  if (!dbus_add_dict_entry_uint32(&dict_iter, "Modification Time", modtime))
    goto close_dict;

  ret = true;

close_dict:
  if (!dbus_message_iter_close_container(&struct_iter, &dict_iter))
    ret = false;

close_struct:
  if (!dbus_message_iter_close_container(array_iter_ptr, &struct_iter))
    ret = false;

exit:
  return ret;
}

static void ladish_get_studio_list(struct dbus_method_call * call_ptr)
{
  DBusMessageIter iter, array_iter;

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

  if (!studios_iterate(call_ptr, &array_iter, get_studio_list_callback))
  {
    dbus_message_iter_close_container(&iter, &array_iter);
    if (call_ptr->reply == NULL)
      goto fail_unref;

    /* studios_iterate or get_studio_list_callback() composed error reply */
    return;
  }

  if (!dbus_message_iter_close_container(&iter, &array_iter))
  {
    goto fail_unref;
  }

  return;

fail_unref:
  dbus_message_unref(call_ptr->reply);
  call_ptr->reply = NULL;

fail:
  log_error("Ran out of memory trying to construct method return");
}

static void ladish_load_studio(struct dbus_method_call * call_ptr)
{
  const char * name;

  dbus_error_init(&g_dbus_error);

  if (!dbus_message_get_args(call_ptr->message, &g_dbus_error, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  log_info("Load studio request (%s)", name);

  if (ladish_command_load_studio(call_ptr, &g_studio.cmd_queue, name))
  {
    method_return_new_void(call_ptr);
  }
}

static void ladish_delete_studio(struct dbus_method_call * call_ptr)
{
  const char * name;

  dbus_error_init(&g_dbus_error);

  if (!dbus_message_get_args(call_ptr->message, &g_dbus_error, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  if (studio_delete(call_ptr, name))
  {
    method_return_new_void(call_ptr);
  }
}

static void ladish_new_studio(struct dbus_method_call * call_ptr)
{
  const char * name;

  dbus_error_init(&g_dbus_error);

  if (!dbus_message_get_args(call_ptr->message, &g_dbus_error, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  log_info("New studio request (%s)", name);

  if (ladish_command_new_studio(call_ptr, &g_studio.cmd_queue, name))
  {
    method_return_new_void(call_ptr);
  }
}

static void ladish_get_application_list(struct dbus_method_call * call_ptr)
{
  DBusMessageIter iter;
  DBusMessageIter array_iter;
#if 0
  DBusMessageIter struct_iter;
  DBusMessageIter dict_iter;
  struct list_head * node_ptr;
  struct lash_appdb_entry * entry_ptr;
#endif

  log_info("Getting applications list");

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

#define array_iter_ptr ((DBusMessageIter *)context)

bool room_template_list_filler(void * context, ladish_room_handle room)
{
  DBusMessageIter struct_iter;
  DBusMessageIter dict_iter;
  const char * name;

  name = ladish_room_get_name(room);

  if (!dbus_message_iter_open_container(array_iter_ptr, DBUS_TYPE_STRUCT, NULL, &struct_iter))
    return false;

  if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &name))
    return false;

  if (!dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter))
    return false;

  if (!dbus_message_iter_close_container(&struct_iter, &dict_iter))
    return false;

  if (!dbus_message_iter_close_container(array_iter_ptr, &struct_iter))
    return false;

  return true;
}

#undef array_iter_ptr

static void ladish_get_room_template_list(struct dbus_method_call * call_ptr)
{
  DBusMessageIter iter, array_iter;

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

  if (!rooms_enum(&array_iter, room_template_list_filler))
  {
    goto fail_unref;
  }

  if (!dbus_message_iter_close_container(&iter, &array_iter))
  {
    goto fail_unref;
  }

  return;

fail_unref:
  dbus_message_unref(call_ptr->reply);
  call_ptr->reply = NULL;

fail:
  log_error("Ran out of memory trying to construct method return");
}

static void ladish_delete_room_template(struct dbus_method_call * call_ptr)
{
  const char * name;

  dbus_error_init(&g_dbus_error);

  if (!dbus_message_get_args(call_ptr->message, &g_dbus_error, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  log_info("Delete room request (%s)", name);

  {
    method_return_new_void(call_ptr);
  }
}

static void ladish_create_room_template(struct dbus_method_call * call_ptr)
{
  const char * name;

  dbus_error_init(&g_dbus_error);

  if (!dbus_message_get_args(call_ptr->message, &g_dbus_error, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  log_info("New room request (%s)", name);

  {
    method_return_new_void(call_ptr);
  }
}

static void ladish_exit(struct dbus_method_call * call_ptr)
{
  log_info("Exit command received through D-Bus");

  if (!ladish_command_exit(NULL, &g_studio.cmd_queue))
  { /* if queuing of command failed, force exit anyway,
       JACK server will be left started,
       but refusing exit command is worse */
    g_quit = true;
  }

  method_return_new_void(call_ptr);
}

void emit_studio_appeared(void)
{
  dbus_signal_emit(g_dbus_connection, CONTROL_OBJECT_PATH, INTERFACE_NAME, "StudioAppeared", "");
}

void emit_studio_disappeared(void)
{
  dbus_signal_emit(g_dbus_connection, CONTROL_OBJECT_PATH, INTERFACE_NAME, "StudioDisappeared", "");
}

void emit_clean_exit(void)
{
  dbus_signal_emit(g_dbus_connection, CONTROL_OBJECT_PATH, INTERFACE_NAME, "CleanExit", "");
}

METHOD_ARGS_BEGIN(IsStudioLoaded, "Check whether studio D-Bus object is present")
  METHOD_ARG_DESCRIBE_OUT("present", "b", "Whether studio D-Bus object is present")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(GetStudioList, "Get list of studios")
  METHOD_ARG_DESCRIBE_OUT("studio_list", "a(sa{sv})", "List of studios, name and properties")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(NewStudio, "New studio")
  METHOD_ARG_DESCRIBE_IN("studio_name", "s", "Name of studio, if empty name will be generated")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(LoadStudio, "Load studio")
  METHOD_ARG_DESCRIBE_IN("studio_name", "s", "Name of studio to load")
  METHOD_ARG_DESCRIBE_IN("options", "a{sv}", "Load options")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(DeleteStudio, "Delete studio")
  METHOD_ARG_DESCRIBE_IN("studio_name", "s", "Name of studio to delete")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(GetApplicationList, "Get list of applications that can be launched")
  METHOD_ARG_DESCRIBE_OUT("applications", "a(sa{sv})", "List of applications, name and properties")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(GetRoomTemplateList, "Get list of room templates")
  METHOD_ARG_DESCRIBE_OUT("room_template_list", "a(sa{sv})", "List of room templates (name and properties)")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(CreateRoomTemplate, "New room template")
  METHOD_ARG_DESCRIBE_IN("room_template name", "s", "Name of the room template")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(DeleteRoomTemplate, "Delete room template")
  METHOD_ARG_DESCRIBE_IN("room_template_name", "s", "Name of room template to delete")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Exit, "Tell ladish D-Bus service to exit")
METHOD_ARGS_END

METHODS_BEGIN
  METHOD_DESCRIBE(IsStudioLoaded, ladish_is_studio_loaded)
  METHOD_DESCRIBE(GetStudioList, ladish_get_studio_list)
  METHOD_DESCRIBE(NewStudio, ladish_new_studio)
  METHOD_DESCRIBE(LoadStudio, ladish_load_studio)
  METHOD_DESCRIBE(DeleteStudio, ladish_delete_studio)
  METHOD_DESCRIBE(GetApplicationList, ladish_get_application_list)
  METHOD_DESCRIBE(GetRoomTemplateList, ladish_get_room_template_list)
  METHOD_DESCRIBE(CreateRoomTemplate, ladish_create_room_template)
  METHOD_DESCRIBE(DeleteRoomTemplate, ladish_delete_room_template)
  METHOD_DESCRIBE(Exit, ladish_exit)
METHODS_END

SIGNAL_ARGS_BEGIN(StudioAppeared, "Studio D-Bus object appeared")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(StudioDisappeared, "Studio D-Bus object disappeared")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(CleanExit, "Exit was requested")
SIGNAL_ARGS_END

SIGNALS_BEGIN
  SIGNAL_DESCRIBE(StudioAppeared)
  SIGNAL_DESCRIBE(StudioDisappeared)
  SIGNAL_DESCRIBE(CleanExit)
SIGNALS_END

/*
 * Interface description.
 */

INTERFACE_BEGIN(g_lashd_interface_control, INTERFACE_NAME)
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
  INTERFACE_EXPOSE_SIGNALS
INTERFACE_END
