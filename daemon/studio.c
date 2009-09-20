/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains part of the studio singleton object implementation
 * Other parts are in the other studio*.c files in same directory.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "studio_internal.h"
#include "../dbus_constants.h"
#include "control.h"
#include "../catdup.h"
#include "dirhelpers.h"
#include "graph_dict.h"
#include "escape.h"

#define STUDIOS_DIR "/studios/"
char * g_studios_dir;

struct studio g_studio;

#define EVENT_JACK_START   0
#define EVENT_JACK_STOP    1

struct event
{
  struct list_head siblings;
  unsigned int type;
};

bool studio_name_generate(char ** name_ptr)
{
  time_t now;
  char timestamp_str[26];
  char * name;

  time(&now);
  //ctime_r(&now, timestamp_str);
  //timestamp_str[24] = 0;
  snprintf(timestamp_str, sizeof(timestamp_str), "%llu", (unsigned long long)now);

  name = catdup("Studio ", timestamp_str);
  if (name == NULL)
  {
    log_error("catdup failed to create studio name");
    return false;
  }

  *name_ptr = name;
  return true;
}

bool
studio_publish(void)
{
  dbus_object_path object;

  assert(g_studio.name != NULL);

  object = dbus_object_path_new(
    STUDIO_OBJECT_PATH,
    &g_interface_studio, &g_studio,
    &g_interface_patchbay, ladish_graph_get_dbus_context(g_studio.studio_graph),
    &g_iface_graph_dict, g_studio.studio_graph,
    NULL);
  if (object == NULL)
  {
    log_error("dbus_object_path_new() failed");
    return false;
  }

  if (!dbus_object_path_register(g_dbus_connection, object))
  {
    log_error("object_path_register() failed");
    dbus_object_path_destroy(g_dbus_connection, object);
    return false;
  }

  log_info("Studio D-Bus object created. \"%s\"", g_studio.name);

  g_studio.dbus_object = object;

  emit_studio_appeared();

  return true;
}

static void emit_studio_started()
{
  dbus_signal_emit(g_dbus_connection, STUDIO_OBJECT_PATH, IFACE_STUDIO, "StudioStarted", "");
}

static void emit_studio_stopped()
{
  dbus_signal_emit(g_dbus_connection, STUDIO_OBJECT_PATH, IFACE_STUDIO, "StudioStopped", "");
}

bool studio_start(void)
{
  if (!g_studio.jack_running)
  {
    log_info("Starting JACK server.");

    if (!jack_proxy_start_server())
    {
      log_error("jack_proxy_start_server() failed.");
      return false;
    }
  }

  emit_studio_started();

  return true;
}

static bool studio_stop(void)
{
  if (g_studio.jack_running)
  {
    log_info("Stopping JACK server...");

    g_studio.automatic = false;   /* even if it was automatic, it is not anymore because user knows about it */

    if (jack_proxy_stop_server())
    {
      g_studio.jack_running = false;
      emit_studio_stopped();
    }
    else
    {
      log_error("Stopping JACK server failed.");
      return false;
    }
  }

  return true;
}

void
studio_clear(void)
{
  jack_conf_clear();
  studio_stop();

  g_studio.modified = false;
  g_studio.persisted = false;
  g_studio.automatic = false;

  if (g_studio.dbus_object != NULL)
  {
    dbus_object_path_destroy(g_dbus_connection, g_studio.dbus_object);
    g_studio.dbus_object = NULL;
    emit_studio_disappeared();
  }

  if (g_studio.name != NULL)
  {
    free(g_studio.name);
    g_studio.name = NULL;
  }

  if (g_studio.filename != NULL)
  {
    free(g_studio.filename);
    g_studio.filename = NULL;
  }
}

void
studio_clear_if_automatic(void)
{
  if (g_studio.automatic)
  {
    log_info("Unloading automatic studio.");
    studio_clear();
    return;
  }
}

void on_event_jack_started(void)
{
  if (g_studio.dbus_object == NULL)
  {
    assert(g_studio.name == NULL);
    if (!studio_name_generate(&g_studio.name))
    {
      log_error("studio_name_generate() failed.");
      return;
    }

    g_studio.automatic = true;

    studio_publish();
  }

  if (!studio_fetch_jack_settings())
  {
    log_error("studio_fetch_jack_settings() failed.");

    studio_clear_if_automatic();
    return;
  }

  log_info("jack conf successfully retrieved");
  g_studio.jack_conf_valid = true;
  g_studio.jack_running = true;

  if (!graph_proxy_create(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, false, &g_studio.jack_graph_proxy))
  {
    log_error("graph_proxy_create() failed for jackdbus");
  }
  else
  {
    if (!ladish_jack_dispatcher_create(g_studio.jack_graph_proxy, g_studio.jack_graph, g_studio.studio_graph, &g_studio.jack_dispatcher))
    {
      log_error("ladish_jack_dispatcher_create() failed.");
    }

    if (!graph_proxy_activate(g_studio.jack_graph_proxy))
    {
      log_error("graph_proxy_activate() failed.");
    }
  }
}

void on_event_jack_stopped(void)
{
  studio_clear_if_automatic();

  g_studio.jack_running = false;

  if (g_studio.jack_dispatcher)
  {
    ladish_jack_dispatcher_destroy(g_studio.jack_dispatcher);
    g_studio.jack_dispatcher = NULL;
  }

  ladish_graph_clear(g_studio.jack_graph);
  ladish_graph_clear(g_studio.studio_graph);

  if (g_studio.jack_graph_proxy)
  {
    graph_proxy_destroy(g_studio.jack_graph_proxy);
    g_studio.jack_graph_proxy = NULL;
  }

  /* TODO: if user wants, restart jack server and reconnect all jack apps to it */
}

void studio_run(void)
{
  struct event * event_ptr;

  while (!list_empty(&g_studio.event_queue))
  {
    event_ptr = list_entry(g_studio.event_queue.next, struct event, siblings);
    list_del(g_studio.event_queue.next);

    switch (event_ptr->type)
    {
    case EVENT_JACK_START:
      on_event_jack_started();
      break;
    case EVENT_JACK_STOP:
      on_event_jack_stopped();
      break;
    }

    free(event_ptr);
  }
}

static void on_jack_server_started(void)
{
  struct event * event_ptr;

  log_info("JACK server start detected.");

  event_ptr = malloc(sizeof(struct event));
  if (event_ptr == NULL)
  {
    log_error("malloc() failed to allocate struct event. Ignoring JACK start.");
    return;
  }

  event_ptr->type = EVENT_JACK_START;
  list_add_tail(&event_ptr->siblings, &g_studio.event_queue);
}

static void on_jack_server_stopped(void)
{
  struct event * event_ptr;

  log_info("JACK server stop detected.");

  event_ptr = malloc(sizeof(struct event));
  if (event_ptr == NULL)
  {
    log_error("malloc() failed to allocate struct event. Ignoring JACK stop.");
    return;
  }

  event_ptr->type = EVENT_JACK_STOP;
  list_add_tail(&event_ptr->siblings, &g_studio.event_queue);
}

static void on_jack_server_appeared(void)
{
  log_info("JACK controller appeared.");
}

static void on_jack_server_disappeared(void)
{
  log_info("JACK controller disappeared.");
}

bool studio_init(void)
{
  log_info("studio object construct");

  g_studios_dir = catdup(g_base_dir, STUDIOS_DIR);
  if (g_studios_dir == NULL)
  {
    log_error("catdup failed for '%s' and '%s'", g_base_dir, STUDIOS_DIR);
    goto fail;
  }

  if (!ensure_dir_exist(g_studios_dir, 0700))
  {
    goto free_studios_dir;
  }

  INIT_LIST_HEAD(&g_studio.all_connections);
  INIT_LIST_HEAD(&g_studio.all_ports);
  INIT_LIST_HEAD(&g_studio.all_clients);
  INIT_LIST_HEAD(&g_studio.jack_connections);
  INIT_LIST_HEAD(&g_studio.jack_ports);
  INIT_LIST_HEAD(&g_studio.jack_clients);
  INIT_LIST_HEAD(&g_studio.rooms);
  INIT_LIST_HEAD(&g_studio.clients);
  INIT_LIST_HEAD(&g_studio.ports);

  INIT_LIST_HEAD(&g_studio.jack_conf);
  INIT_LIST_HEAD(&g_studio.jack_params);

  INIT_LIST_HEAD(&g_studio.event_queue);

  g_studio.dbus_object = NULL;
  g_studio.name = NULL;
  g_studio.filename = NULL;
  g_studio.jack_running = false;

  //studio_clear();

  if (!ladish_graph_create(&g_studio.jack_graph, NULL))
  {
    log_error("ladish_graph_create() failed to create jack graph object.");
    goto free_studios_dir;
  }

  if (!ladish_graph_create(&g_studio.studio_graph, STUDIO_OBJECT_PATH))
  {
    log_error("ladish_graph_create() failed to create studio graph object.");
    goto jack_graph_destroy;
  }

  if (!jack_proxy_init(
        on_jack_server_started,
        on_jack_server_stopped,
        on_jack_server_appeared,
        on_jack_server_disappeared))
  {
    log_error("jack_proxy_init() failed.");
    goto studio_graph_destroy;
  }

  return true;

studio_graph_destroy:
  ladish_graph_destroy(g_studio.studio_graph);
jack_graph_destroy:
  ladish_graph_destroy(g_studio.jack_graph);
free_studios_dir:
  studio_clear();
  free(g_studios_dir);
fail:
  return false;
}

void studio_uninit(void)
{
  jack_proxy_uninit();

  ladish_graph_destroy(g_studio.studio_graph);

  studio_clear();

  free(g_studios_dir);

  log_info("studio object destroy");
}

bool studio_is_loaded(void)
{
  return g_studio.dbus_object != NULL;
}

bool studio_compose_filename(const char * name, char ** filename_ptr_ptr, char ** backup_filename_ptr_ptr)
{
  size_t len_dir;
  char * p;
  const char * src;
  char * filename_ptr;
  char * backup_filename_ptr;

  len_dir = strlen(g_studios_dir);

  filename_ptr = malloc(len_dir + 1 + strlen(name) * 3 + 4 + 1);
  if (filename_ptr == NULL)
  {
    log_error("malloc failed to allocate memory for studio file path");
    return false;
  }

  if (backup_filename_ptr_ptr != NULL)
  {
    backup_filename_ptr = malloc(len_dir + 1 + strlen(name) * 3 + 4 + 4 + 1);
    if (backup_filename_ptr == NULL)
    {
      log_error("malloc failed to allocate memory for studio backup file path");
      free(filename_ptr);
      return false;
    }
  }

  p = filename_ptr;
  memcpy(p, g_studios_dir, len_dir);
  p += len_dir;

  *p++ = '/';

  src = name;
  escape(&src, &p);
  strcpy(p, ".xml");

  *filename_ptr_ptr = filename_ptr;

  if (backup_filename_ptr_ptr != NULL)
  {
    strcpy(backup_filename_ptr, filename_ptr);
    strcat(backup_filename_ptr, ".bak");
    *backup_filename_ptr_ptr = backup_filename_ptr;
  }

  return true;
}

bool studios_iterate(void * call_ptr, void * context, bool (* callback)(void * call_ptr, void * context, const char * studio, uint32_t modtime))
{
  DIR * dir;
  struct dirent * dentry;
  size_t len;
  struct stat st;
  char * path;
  char * name;

  dir = opendir(g_studios_dir);
  if (dir == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "Cannot open directory '%s': %d (%s)", g_studios_dir, errno, strerror(errno));
    return false;
  }

  while ((dentry = readdir(dir)) != NULL)
  {
    if (dentry->d_type != DT_REG)
      continue;

    len = strlen(dentry->d_name);
    if (len <= 4 || strcmp(dentry->d_name + (len - 4), ".xml") != 0)
      continue;

    path = catdup(g_studios_dir, dentry->d_name);
    if (path == NULL)
    {
      lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "catdup() failed");
      return false;
    }

    if (stat(path, &st) != 0)
    {
      lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "failed to stat '%s': %d (%s)", path, errno, strerror(errno));
      free(path);
      return false;
    }

    free(path);

    name = malloc(len - 4 + 1);
    name[unescape(dentry->d_name, len - 4, name)] = 0;
    //log_info("name = '%s'", name);

    if (!callback(call_ptr, context, name, st.st_mtime))
    {
      free(name);
      closedir(dir);
      return false;
    }

    free(name);
  }

  closedir(dir);
  return true;
}

bool studio_delete(void * call_ptr, const char * studio_name)
{
  char * filename;
  char * bak_filename;
  struct stat st;
  bool ret;

  ret = false;

  if (!studio_compose_filename(studio_name, &filename, &bak_filename))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "failed to compose studio filename");
    goto exit;
  }

  log_info("Deleting studio ('%s')", filename);

  if (unlink(filename) != 0)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "unlink(%s) failed: %d (%s)", filename, errno, strerror(errno));
    goto free;
  }

  /* try to delete the backup file */
  if (stat(bak_filename, &st) == 0)
  {
    if (unlink(bak_filename) != 0)
    {
      /* failing to delete backup file will not case delete command failure */
      log_error("unlink(%s) failed: %d (%s)", bak_filename, errno, strerror(errno));
    }
  }

  ret = true;

free:
  free(filename);
  free(bak_filename);
exit:
  return ret;
}

void emit_studio_renamed()
{
  dbus_signal_emit(g_dbus_connection, STUDIO_OBJECT_PATH, IFACE_STUDIO, "StudioRenamed", "s", &g_studio.name);
}

static void ladish_get_studio_name(struct dbus_method_call * call_ptr)
{
  method_return_new_single(call_ptr, DBUS_TYPE_STRING, &g_studio.name);
}

static void ladish_rename_studio(struct dbus_method_call * call_ptr)
{
  const char * new_name;
  char * new_name_dup;
  
  if (!dbus_message_get_args(call_ptr->message, &g_dbus_error, DBUS_TYPE_STRING, &new_name, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  log_info("Rename studio request (%s)", new_name);

  new_name_dup = strdup(new_name);
  if (new_name_dup == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup() failed to allocate new name.");
    return;
  }

  free(g_studio.name);
  g_studio.name = new_name_dup;

  method_return_new_void(call_ptr);
  emit_studio_renamed();
}

static void ladish_save_studio(struct dbus_method_call * call_ptr)
{
  if (studio_save(call_ptr))
  {
    method_return_new_void(call_ptr);
  }
}

static void ladish_unload_studio(struct dbus_method_call * call_ptr)
{
  log_info("Unload studio request");
  studio_clear();
  method_return_new_void(call_ptr);
}

bool studio_new(void * call_ptr, const char * studio_name)
{
  log_info("New studio request (%s)", studio_name);
  studio_clear();

  assert(g_studio.name == NULL);
  if (*studio_name != 0)
  {
    g_studio.name = strdup(studio_name);
    if (g_studio.name == NULL)
    {
      lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup() failed to allocate studio name.");
      return false;
    }
  }
  else if (!studio_name_generate(&g_studio.name))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "studio_name_generate() failed.");
    return false;
  }

  if (!studio_publish())
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "studio_publish() failed.");
    studio_clear();
    return false;
  }

  return true;
}

static void ladish_stop_studio(struct dbus_method_call * call_ptr)
{
  log_info("Studio stop requested");

  if (!studio_stop())
  {
      lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "Failed to stop studio.");
  }
  else
  {
    method_return_new_void(call_ptr);
  }
}

static void ladish_start_studio(struct dbus_method_call * call_ptr)
{
  log_info("Studio start requested");

  if (!studio_start())
  {
      lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "Failed to start studio.");
  }
  else
  {
    method_return_new_void(call_ptr);
  }
}

METHOD_ARGS_BEGIN(GetName, "Get studio name")
  METHOD_ARG_DESCRIBE_OUT("studio_name", "s", "Name of studio")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Rename, "Rename studio")
  METHOD_ARG_DESCRIBE_IN("studio_name", "s", "New name")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Save, "Save studio")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Unload, "Unload studio")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Start, "Start studio")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Stop, "Stop studio")
METHOD_ARGS_END

METHODS_BEGIN
  METHOD_DESCRIBE(GetName, ladish_get_studio_name)
  METHOD_DESCRIBE(Rename, ladish_rename_studio)
  METHOD_DESCRIBE(Save, ladish_save_studio)
  METHOD_DESCRIBE(Unload, ladish_unload_studio)
  METHOD_DESCRIBE(Start, ladish_start_studio)
  METHOD_DESCRIBE(Stop, ladish_stop_studio)
METHODS_END

SIGNAL_ARGS_BEGIN(StudioRenamed, "Studio name changed")
  SIGNAL_ARG_DESCRIBE("studio_name", "s", "New studio name")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(StudioStarted, "Studio started")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(StudioStopped, "Studio stopped")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(RoomAppeared, "Room D-Bus object appeared")
  SIGNAL_ARG_DESCRIBE("room_path", "s", "room object path")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(RoomDisappeared, "Room D-Bus object disappeared")
  SIGNAL_ARG_DESCRIBE("room_path", "s", "room object path")
SIGNAL_ARGS_END

SIGNALS_BEGIN
  SIGNAL_DESCRIBE(StudioRenamed)
  SIGNAL_DESCRIBE(StudioStarted)
  SIGNAL_DESCRIBE(StudioStopped)
  SIGNAL_DESCRIBE(RoomAppeared)
  SIGNAL_DESCRIBE(RoomDisappeared)
SIGNALS_END

INTERFACE_BEGIN(g_interface_studio, IFACE_STUDIO)
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
  INTERFACE_EXPOSE_SIGNALS
INTERFACE_END
