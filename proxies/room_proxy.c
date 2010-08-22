/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
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
};

bool ladish_room_proxy_create(const char * service, const char * object, ladish_room_proxy_handle * handle_ptr)
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

  *handle_ptr = (ladish_room_proxy_handle)proxy_ptr;
  return true;

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
  free(proxy_ptr->object);
  free(proxy_ptr->service);
  free(proxy_ptr);
}

char * ladish_room_proxy_get_name(ladish_room_proxy_handle proxy)
{
  DBusMessage * reply_ptr;
  const char * name;
  char * name_buffer;

  if (!dbus_call(proxy_ptr->service, proxy_ptr->object, IFACE_ROOM, "GetName", "", NULL, &reply_ptr))
  {
    log_error("GetName() failed.");
    return NULL;
  }

  if (!dbus_message_get_args(
        reply_ptr,
        &g_dbus_error,
        DBUS_TYPE_STRING, &name,
        DBUS_TYPE_INVALID))
  {
    dbus_message_unref(reply_ptr);
    dbus_error_free(&g_dbus_error);
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
  if (!dbus_call(proxy_ptr->service, proxy_ptr->object, IFACE_ROOM, "LoadProject", "s", &project_dir, ""))
  {
    log_error("LoadProject() failed.");
    return false;
  }

  return true;
}

bool ladish_room_proxy_save_project(ladish_room_proxy_handle proxy, const char * project_dir, const char * project_name)
{
  if (!dbus_call(proxy_ptr->service, proxy_ptr->object, IFACE_ROOM, "SaveProject", "ss", &project_dir, &project_name, ""))
  {
    log_error("SaveProject() failed.");
    return false;
  }

  return true;
}

bool ladish_room_proxy_unload_project(ladish_room_proxy_handle proxy)
{
  if (!dbus_call(proxy_ptr->service, proxy_ptr->object, IFACE_ROOM, "UnloadProject", "", ""))
  {
    log_error("UnloadProject() failed.");
    return false;
  }

  return true;
}

bool ladish_room_proxy_get_project_properties(ladish_room_proxy_handle proxy, char ** project_dir, char ** project_name)
{
  DBusMessage * reply_ptr;
  DBusMessageIter iter;
  const char * name;
  const char * dir;
  char * name_buffer;
  char * dir_buffer;

  if (!dbus_call(proxy_ptr->service, proxy_ptr->object, IFACE_ROOM, "GetProjectProperties", "", NULL, &reply_ptr))
  {
    log_error("GetProjectProperties() failed.");
    return false;
  }

  dbus_message_iter_init(reply_ptr, &iter);

  if (!dbus_iter_get_dict_entry_string(&iter, "name", &name))
  {
    name = "";
  }

  if (!dbus_iter_get_dict_entry_string(&iter, "dir", &dir))
  {
    dir = "";
  }

  name_buffer = strdup(name);
  if (name_buffer == NULL)
  {
    log_error("strdup() failed for project name");
    dbus_message_unref(reply_ptr);
    return false;
  }

  dir_buffer = strdup(dir);
  if (dir_buffer == NULL)
  {
    log_error("strdup() failed for project dir");
    free(name_buffer);
    dbus_message_unref(reply_ptr);
    return false;
  }

  dbus_message_unref(reply_ptr);

  *project_name = name_buffer;
  *project_dir = dir_buffer;

  return true;
}

#undef proxy_ptr
