/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010,2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of code that interfaces
 * ladishd room objects through D-Bus
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

#include "room_proxy.h"

struct ladish_room_proxy
{
  char * service;
  char * object;

  void * project_properties_changed_context;
  void (* project_properties_changed)(
    void * project_properties_changed_context,
    const char * project_dir,
    const char * project_name,
    const char * project_description,
    const char * project_notes);

  uint64_t project_properties_version;
  char * project_name;
  char * project_dir;
  char * project_description;
  char * project_notes;
};

static bool update_project_properties(struct ladish_room_proxy * proxy_ptr, DBusMessage * message_ptr, const char * context)
{
  const char * signature;
  DBusMessageIter iter;
  dbus_uint64_t version;
  const char * name;
  const char * dir;
  const char * description;
  const char * notes;
  char * name_buffer;
  char * dir_buffer;
  char * description_buffer;
  char * notes_buffer;

  signature = dbus_message_get_signature(message_ptr);
  if (strcmp(signature, "ta{sv}") != 0)
  {
    log_error("%s signature mismatch. '%s'", context, signature);
    goto fail;
  }

  dbus_message_iter_init(message_ptr, &iter);

  dbus_message_iter_get_basic(&iter, &version);
  dbus_message_iter_next(&iter);

  if (version == 0)
  {
    log_error("%s contains project properties version 0", context);
    goto fail;
  }

  if (proxy_ptr->project_properties_version >= version)
  {
    goto fail;
  }

  if (!dbus_iter_get_dict_entry_string(&iter, "name", &name))
  {
    name = "";
  }

  if (!dbus_iter_get_dict_entry_string(&iter, "dir", &dir))
  {
    dir = "";
  }

  if (!dbus_iter_get_dict_entry_string(&iter, "description", &description))
  {
    description = "";
  }

  if (!dbus_iter_get_dict_entry_string(&iter, "notes", &notes))
  {
    notes = "";
  }

  name_buffer = strdup(name);
  if (name_buffer == NULL)
  {
    log_error("strdup() failed for project name");
    goto fail;
  }

  dir_buffer = strdup(dir);
  if (dir_buffer == NULL)
  {
    log_error("strdup() failed for project dir");
    goto fail_free_name_buffer;
  }

  description_buffer = strdup(description);
  if (description_buffer == NULL)
  {
    log_error("strdup() failed for project description");
    goto fail_free_dir_buffer;
  }

  notes_buffer = strdup(notes);
  if (notes_buffer == NULL)
  {
    log_error("strdup() failed for project notes");
    goto fail_free_description_buffer;
  }

  proxy_ptr->project_properties_version = version;

  if (proxy_ptr->project_name != NULL)
  {
    free(proxy_ptr->project_name);
  }
  proxy_ptr->project_name = name_buffer;

  if (proxy_ptr->project_dir != NULL)
  {
    free(proxy_ptr->project_dir);
  }
  proxy_ptr->project_dir = dir_buffer;

  if (proxy_ptr->project_description != NULL)
  {
    free(proxy_ptr->project_description);
  }
  proxy_ptr->project_description = description_buffer;

  if (proxy_ptr->project_notes != NULL)
  {
    free(proxy_ptr->project_notes);
  }
  proxy_ptr->project_notes = notes_buffer;

  /* log_info("Room '%s' project properties changed:", proxy_ptr->object); /\* TODO: cache project name *\/ */
  /* log_info("  Properties version: %"PRIu64, proxy_ptr->project_properties_version); */
  /* log_info("  Project name: '%s'", proxy_ptr->project_name); */
  /* log_info("  Project dir: '%s'", proxy_ptr->project_dir); */
  /* log_info("  Project description: '%s'", proxy_ptr->project_description); */
  /* log_info("  Project notes: '%s'", proxy_ptr->project_notes); */

  proxy_ptr->project_properties_changed(
    proxy_ptr->project_properties_changed_context,
    proxy_ptr->project_dir,
    proxy_ptr->project_name,
    proxy_ptr->project_description,
    proxy_ptr->project_notes);

  return true;

fail_free_description_buffer:
  free(description_buffer);
fail_free_dir_buffer:
  free(dir_buffer);
fail_free_name_buffer:
  free(name_buffer);
fail:
  return false;
}

bool ladish_room_proxy_get_project_properties_internal(struct ladish_room_proxy * proxy_ptr)
{
  DBusMessage * reply_ptr;

  if (!dbus_call(0, proxy_ptr->service, proxy_ptr->object, IFACE_ROOM, "GetProjectProperties", "", NULL, &reply_ptr))
  {
    log_error("GetProjectProperties() failed.");
    return false;
  }

  if (!update_project_properties(proxy_ptr, reply_ptr, "GetProjectProperties() reply"))
  {
    dbus_message_unref(reply_ptr);
    return false;
  }

  dbus_message_unref(reply_ptr);

  return true;
}

#define proxy_ptr ((struct ladish_room_proxy *)context)

static void on_project_properties_changed(void * context, DBusMessage * message_ptr)
{
  log_info("ProjectPropertiesChanged signal received.");
  update_project_properties(proxy_ptr, message_ptr, "ProjectPropertiesChanged() signal");
}

#undef proxy_ptr

/* this must be static because it is referenced by the
 * dbus helper layer when hooks are active */
static struct dbus_signal_hook g_signal_hooks[] =
{
  {"ProjectPropertiesChanged", on_project_properties_changed},
  {NULL, NULL}
};

bool
ladish_room_proxy_create(
  const char * service,
  const char * object,
  void * project_properties_changed_context,
  void (* project_properties_changed)(
    void * project_properties_changed_context,
    const char * project_dir,
    const char * project_name,
    const char * project_description,
    const char * project_notes),
  ladish_room_proxy_handle * handle_ptr)
{
  struct ladish_room_proxy * proxy_ptr;

  proxy_ptr = malloc(sizeof(struct ladish_room_proxy));
  if (proxy_ptr == NULL)
  {
    log_error("malloc() failed to allocate room proxy struct");
    goto fail;
  }

  proxy_ptr->service = strdup(service);
  if (proxy_ptr->service == NULL)
  {
    log_error("strdup() failed too duplicate service name '%s'", service);
    goto free_proxy;
  }

  proxy_ptr->object = strdup(object);
  if (proxy_ptr->object == NULL)
  {
    log_error("strdup() failed too duplicate object name '%s'", object);
    goto free_service;
  }

  proxy_ptr->project_properties_version = 0;
  proxy_ptr->project_name = NULL;
  proxy_ptr->project_dir = NULL;
  proxy_ptr->project_description = NULL;
  proxy_ptr->project_notes = NULL;

  proxy_ptr->project_properties_changed_context = project_properties_changed_context;
  proxy_ptr->project_properties_changed = project_properties_changed;

  if (!dbus_register_object_signal_hooks(
        cdbus_g_dbus_connection,
        proxy_ptr->service,
        proxy_ptr->object,
        IFACE_ROOM,
        proxy_ptr,
        g_signal_hooks))
  {
    log_error("dbus_register_object_signal_hooks() failed for room");
    goto free_object;
  }

  if (!ladish_room_proxy_get_project_properties_internal(proxy_ptr))
  {
    goto unregister_signal_hooks;
  }

  *handle_ptr = (ladish_room_proxy_handle)proxy_ptr;
  return true;

unregister_signal_hooks:
  dbus_unregister_object_signal_hooks(cdbus_g_dbus_connection, proxy_ptr->service, proxy_ptr->object, IFACE_ROOM);
free_object:
  free(proxy_ptr->object);
free_service:
  free(proxy_ptr->service);
free_proxy:
  free(proxy_ptr);
fail:
  return false;
}

#define proxy_ptr ((struct ladish_room_proxy *)proxy)

void ladish_room_proxy_destroy(ladish_room_proxy_handle proxy)
{
  dbus_unregister_object_signal_hooks(cdbus_g_dbus_connection, proxy_ptr->service, proxy_ptr->object, IFACE_ROOM);

  if (proxy_ptr->project_name != NULL)
  {
    free(proxy_ptr->project_name);
  }

  if (proxy_ptr->project_dir != NULL)
  {
    free(proxy_ptr->project_dir);
  }

  if (proxy_ptr->project_description != NULL)
  {
    free(proxy_ptr->project_description);
  }

  if (proxy_ptr->project_notes != NULL)
  {
    free(proxy_ptr->project_notes);
  }

  free(proxy_ptr->object);
  free(proxy_ptr->service);
  free(proxy_ptr);
}

char * ladish_room_proxy_get_name(ladish_room_proxy_handle proxy)
{
  DBusMessage * reply_ptr;
  const char * name;
  char * name_buffer;

  if (!dbus_call(0, proxy_ptr->service, proxy_ptr->object, IFACE_ROOM, "GetName", "", NULL, &reply_ptr))
  {
    log_error("GetName() failed.");
    return NULL;
  }

  if (!dbus_message_get_args(
        reply_ptr,
        &cdbus_g_dbus_error,
        DBUS_TYPE_STRING, &name,
        DBUS_TYPE_INVALID))
  {
    dbus_message_unref(reply_ptr);
    dbus_error_free(&cdbus_g_dbus_error);
    log_error("decoding reply of GetName failed.");
    return NULL;
  }

  name_buffer = strdup(name);
  dbus_message_unref(reply_ptr);
  if (name_buffer == NULL)
  {
    log_error("strdup() for app name failed.");
  }
  return name_buffer;
}

bool ladish_room_proxy_load_project(ladish_room_proxy_handle proxy, const char * project_dir)
{
  if (!dbus_call(0, proxy_ptr->service, proxy_ptr->object, IFACE_ROOM, "LoadProject", "s", &project_dir, ""))
  {
    log_error("LoadProject() failed.");
    return false;
  }

  return true;
}

bool ladish_room_proxy_save_project(ladish_room_proxy_handle proxy, const char * project_dir, const char * project_name)
{
  if (!dbus_call(0, proxy_ptr->service, proxy_ptr->object, IFACE_ROOM, "SaveProject", "ss", &project_dir, &project_name, ""))
  {
    log_error("SaveProject() failed.");
    return false;
  }

  return true;
}

bool ladish_room_proxy_unload_project(ladish_room_proxy_handle proxy)
{
  if (!dbus_call(0, proxy_ptr->service, proxy_ptr->object, IFACE_ROOM, "UnloadProject", "", ""))
  {
    log_error("UnloadProject() failed.");
    return false;
  }

  return true;
}

void
ladish_room_proxy_get_project_properties(
  ladish_room_proxy_handle proxy,
  const char ** project_dir,
  const char ** project_name,
  const char ** project_description,
  const char ** project_notes)
{
  ASSERT(proxy_ptr->project_properties_version > 0);

  if (project_dir != NULL)
  {
    *project_dir = proxy_ptr->project_dir;
  }

  if (project_name != NULL)
  {
    *project_name = proxy_ptr->project_name;
  }

  if (project_description != NULL)
  {
    *project_description = proxy_ptr->project_description;
  }

  if (project_notes != NULL)
  {
    *project_notes = proxy_ptr->project_notes;
  }
}

bool
ladish_room_proxy_set_project_description(
  ladish_room_proxy_handle proxy,
  const char * description)
{
  uint64_t new_version;

  if (!dbus_call(0, proxy_ptr->service, proxy_ptr->object, IFACE_ROOM, "SetProjectDescription", "s", &description, "t", &new_version))
  {
    log_error("SetProjectDescription() failed.");
    return false;
  }

  return true;
}

bool
ladish_room_proxy_set_project_notes(
  ladish_room_proxy_handle proxy,
  const char * notes)
{
  uint64_t new_version;

  if (!dbus_call(0, proxy_ptr->service, proxy_ptr->object, IFACE_ROOM, "SetProjectNotes", "s", &notes, "t", &new_version))
  {
    log_error("SetProjectNotes(%s) failed.", notes);
    return false;
  }

  return true;
}

bool
ladish_room_proxy_get_recent_projects(
  ladish_room_proxy_handle proxy,
  uint16_t max_items,
  void (* callback)(
    void * context,
    const char * project_name,
    const char * project_dir),
  void * context)
{
  DBusMessage * reply_ptr;
  const char * reply_signature;
  DBusMessageIter top_iter;
  DBusMessageIter struct_iter;
  DBusMessageIter array_iter;
  const char * project_dir;
  const char * project_name;

  if (!dbus_call(0, proxy_ptr->service, proxy_ptr->object, IFACE_RECENT_ITEMS, "get", "q", &max_items, NULL, &reply_ptr))
  {
    log_error("GetStudioList() failed.");
    return false;
  }

  reply_signature = dbus_message_get_signature(reply_ptr);
  if (strcmp(reply_signature, "a(sa{sv})") != 0)
  {
    log_error("GetStudioList() reply signature mismatch. '%s'", reply_signature);
    dbus_message_unref(reply_ptr);
    return false;
  }

  dbus_message_iter_init(reply_ptr, &top_iter);
  for (dbus_message_iter_recurse(&top_iter, &array_iter);
       dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_INVALID;
       dbus_message_iter_next(&array_iter))
  {
    dbus_message_iter_recurse(&array_iter, &struct_iter);
    dbus_message_iter_get_basic(&struct_iter, &project_dir);
    dbus_message_iter_next(&struct_iter);

    if (!dbus_iter_get_dict_entry_string(&struct_iter, "name", &project_name))
    {
      project_name = NULL;
    }

    callback(context, project_name != NULL ? project_name : project_dir, project_dir);

    dbus_message_iter_next(&struct_iter);
  }

  dbus_message_unref(reply_ptr);
  return true;
}

#undef proxy_ptr
