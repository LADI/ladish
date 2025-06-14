/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010, 2011, 2012 Nedko Arnaudov <nedko@arnaudov.name>
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

#include "studio_internal.h"
#include "../dbus_constants.h"
#include "control.h"
#include "../common/catdup.h"
#include "../common/dirhelpers.h"
#include "graph_dict.h"
#include "graph_manager.h"
#include "escape.h"
#include "studio.h"
#include "../proxies/notify_proxy.h"

#define STUDIOS_DIR "/studios/"

#define RECENT_STUDIOS_STORE_FILE "recent_studios"
#define RECENT_STUDIOS_STORE_MAX_ITEMS 50

char * g_studios_dir;
ladish_recent_store_handle g_studios_recent_store;

struct studio g_studio;

bool ladish_studio_name_generate(char ** name_ptr)
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

bool ladish_studio_show(void)
{
  cdbus_object_path object;

  ASSERT(g_studio.name != NULL);
  ASSERT(!g_studio.announced);

  object = cdbus_object_path_new(
    STUDIO_OBJECT_PATH,
    &g_interface_studio, &g_studio,
    &g_interface_patchbay, ladish_graph_get_dbus_context(g_studio.studio_graph),
    &g_iface_graph_dict, g_studio.studio_graph,
    &g_iface_graph_manager, g_studio.studio_graph,
    &g_iface_app_supervisor, g_studio.app_supervisor,
    NULL);
  if (object == NULL)
  {
    log_error("cdbus_object_path_new() failed");
    return false;
  }

  if (!cdbus_object_path_register(cdbus_g_dbus_connection, object))
  {
    log_error("object_path_register() failed");
    cdbus_object_path_destroy(cdbus_g_dbus_connection, object);
    return false;
  }

  log_info("Studio D-Bus object created. \"%s\"", g_studio.name);

  g_studio.dbus_object = object;

  emit_studio_appeared();

  return true;
}

void ladish_studio_announce(void)
{
  ASSERT(!g_studio.announced);

  /* notify the user that studio started successfully, but dont lie when jack was started externally */
  if (!g_studio.automatic)
  {
    ladish_notify_simple(LADISH_NOTIFY_URGENCY_NORMAL, "Studio loaded", NULL);
  }

  g_studio.announced = true;
}

bool ladish_studio_publish(void)
{
  if (!ladish_studio_show())
  {
    return false;
  }

  ladish_studio_announce();
  return true;
}

void ladish_studio_clear(void)
{
  ladish_graph_dump(g_studio.studio_graph);
  ladish_graph_dump(g_studio.jack_graph);

  /* remove rooms that own clients in studio graph before clearing it */
  ladish_studio_remove_all_rooms();

  ladish_graph_clear(g_studio.studio_graph, NULL);
  ladish_graph_clear(g_studio.jack_graph, NULL);

  ladish_studio_jack_conf_clear();

  g_studio.modified = false;
  g_studio.persisted = false;

  ladish_app_supervisor_clear(g_studio.app_supervisor);

  if (g_studio.dbus_object != NULL)
  {
    cdbus_object_path_destroy(cdbus_g_dbus_connection, g_studio.dbus_object);
    g_studio.dbus_object = NULL;
    emit_studio_disappeared();
  }

  if (g_studio.announced)
  {
    ladish_notify_simple(LADISH_NOTIFY_URGENCY_NORMAL, "Studio unloaded", NULL);
    g_studio.announced = false;
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

void ladish_studio_emit_started(void)
{
  cdbus_signal_emit(cdbus_g_dbus_connection, STUDIO_OBJECT_PATH, IFACE_STUDIO, "StudioStarted", "");
}

void ladish_studio_emit_crashed(void)
{
  cdbus_signal_emit(cdbus_g_dbus_connection, STUDIO_OBJECT_PATH, IFACE_STUDIO, "StudioCrashed", "");
}

void ladish_studio_emit_stopped(void)
{
  cdbus_signal_emit(cdbus_g_dbus_connection, STUDIO_OBJECT_PATH, IFACE_STUDIO, "StudioStopped", "");
}

static bool ladish_studio_fill_room_info(DBusMessageIter * iter_ptr, ladish_room_handle room)
{
  DBusMessageIter dict_iter;
  const char * name;
  uuid_t template_uuid;
  ladish_room_handle template;
  const char * template_name;
  const char * opath;

  name = ladish_room_get_name(room);
  opath = ladish_room_get_opath(room);

  if (!ladish_room_get_template_uuid(room, template_uuid))
  {
    template = NULL;
    template_name = NULL;
  }
  else
  {
    template = find_room_template_by_uuid(template_uuid);
    if (template != NULL)
    {
      template_name = ladish_room_get_name(template);
    }
    else
    {
      template_name = NULL;
    }
  }

  if (!dbus_message_iter_append_basic(iter_ptr, DBUS_TYPE_STRING, &opath))
  {
    log_error("dbus_message_iter_append_basic() failed.");
    return false;
  }

  if (!dbus_message_iter_open_container(iter_ptr, DBUS_TYPE_ARRAY, "{sv}", &dict_iter))
  {
    log_error("dbus_message_iter_open_container() failed.");
    return false;
  }

  if (!cdbus_maybe_add_dict_entry_string(&dict_iter, "template", template_name))
  {
    log_error("dbus_maybe_add_dict_entry_string() failed.");
    return false;
  }

  if (!cdbus_maybe_add_dict_entry_string(&dict_iter, "name", name))
  {
    log_error("dbus_maybe_add_dict_entry_string() failed.");
    return false;
  }

  if (!dbus_message_iter_close_container(iter_ptr, &dict_iter))
  {
    log_error("dbus_message_iter_close_container() failed.");
    return false;
  }

  return true;
}

static void ladish_studio_emit_room_appeared(ladish_room_handle room)
{
  DBusMessage * message_ptr;
  DBusMessageIter iter;

  message_ptr = dbus_message_new_signal(STUDIO_OBJECT_PATH, IFACE_STUDIO, "RoomAppeared");
  if (message_ptr == NULL)
  {
    log_error("dbus_message_new_signal() failed.");
    return;
  }

  dbus_message_iter_init_append(message_ptr, &iter);

  if (ladish_studio_fill_room_info(&iter, room))
  {
    cdbus_signal_send(cdbus_g_dbus_connection, message_ptr);
  }

  dbus_message_unref(message_ptr);
}

void ladish_studio_emit_room_disappeared(ladish_room_handle room)
{
  DBusMessage * message_ptr;
  DBusMessageIter iter;

  message_ptr = dbus_message_new_signal(STUDIO_OBJECT_PATH, IFACE_STUDIO, "RoomDisappeared");
  if (message_ptr == NULL)
  {
    log_error("dbus_message_new_signal() failed.");
    return;
  }

  dbus_message_iter_init_append(message_ptr, &iter);

  if (ladish_studio_fill_room_info(&iter, room))
  {
    cdbus_signal_send(cdbus_g_dbus_connection, message_ptr);
  }

  dbus_message_unref(message_ptr);
}

void ladish_studio_room_appeared(ladish_room_handle room)
{
  log_info("Room \"%s\" appeared", ladish_room_get_name(room));
  list_add_tail(ladish_room_get_list_node(room), &g_studio.rooms);
  ladish_studio_emit_room_appeared(room);
}

void ladish_studio_room_disappeared(ladish_room_handle room)
{
  log_info("Room \"%s\" disappeared", ladish_room_get_name(room));
  list_del(ladish_room_get_list_node(room));
  ladish_studio_emit_room_disappeared(room);
}

bool
ladish_studio_set_graph_connection_handlers(
  void * context,
  ladish_graph_handle graph,
  ladish_app_supervisor_handle UNUSED(app_supervisor))
{
  ladish_virtualizer_set_graph_connection_handlers(context, graph);
  return true;                  /* iterate all graphs */
}

void ladish_studio_on_event_jack_started(void)
{
  if (!ladish_studio_fetch_jack_settings())
  {
    log_error("studio_fetch_jack_settings() failed.");

    return;
  }

  log_info("jack conf successfully retrieved");
  g_studio.jack_conf_valid = true;

  if (!graph_proxy_create(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, false, false, &g_studio.jack_graph_proxy))
  {
    log_error("graph_proxy_create() failed for jackdbus");
  }
  else
  {
    if (!ladish_virtualizer_create(g_studio.jack_graph_proxy, g_studio.jack_graph, &g_studio.virtualizer))
    {
      log_error("ladish_virtualizer_create() failed.");
    }
    else
    {
      ladish_studio_iterate_virtual_graphs(g_studio.virtualizer, ladish_studio_set_graph_connection_handlers);
    }

    if (!graph_proxy_activate(g_studio.jack_graph_proxy))
    {
      log_error("graph_proxy_activate() failed.");
    }
  }

  ladish_app_supervisor_autorun(g_studio.app_supervisor);

  ladish_studio_emit_started();

  /* notify the user that studio started successfully, but dont lie when jack was started externally */
  if (!g_studio.automatic)
  {
    ladish_notify_simple(LADISH_NOTIFY_URGENCY_NORMAL, "Studio started", NULL);
  }
}

static void ladish_studio_on_jack_stopped_internal(void)
{
  if (g_studio.virtualizer)
  {
    ladish_virtualizer_destroy(g_studio.virtualizer);
    g_studio.virtualizer = NULL;
  }

  if (g_studio.jack_graph_proxy)
  {
    graph_proxy_destroy(g_studio.jack_graph_proxy);
    g_studio.jack_graph_proxy = NULL;
  }
}

void ladish_studio_on_event_jack_stopped(void)
{
  ladish_studio_emit_stopped();
  ladish_studio_on_jack_stopped_internal();
  ladish_notify_simple(LADISH_NOTIFY_URGENCY_NORMAL, "Studio stopped", NULL);
}

void ladish_studio_handle_unexpected_jack_server_stop(void)
{
  ladish_studio_emit_crashed();
  ladish_studio_on_jack_stopped_internal();

  /* TODO: if user wants, restart jack server and reconnect all jack apps to it */
}

static
bool
ladish_studio_hide_vgraph_non_virtual(
  void * UNUSED(context),
  ladish_graph_handle graph,
  ladish_app_supervisor_handle UNUSED(app_supervisor))
{
  ladish_graph_hide_non_virtual(graph);
  return true;                  /* iterate all vgraphs */
}

void ladish_studio_run(void)
{
  bool state;

  ladish_cqueue_run(&g_studio.cmd_queue);
  if (g_quit)
  { /* if quit is requested, don't bother to process external events */
    return;
  }

  if (ladish_environment_consume_change(&g_studio.env_store, ladish_environment_jack_server_started, &state))
  {
    ladish_cqueue_clear(&g_studio.cmd_queue);

    if (state)
    {
      ladish_environment_ignore(&g_studio.env_store, ladish_environment_jack_server_present);

      if (g_studio.jack_graph_proxy != NULL)
      {
        log_error("Ignoring \"JACK started\" notification because it is already known that JACK is currently started.");
        return;
      }

      /* Automatic studio creation on JACK server start */
      if (g_studio.dbus_object == NULL)
      {
        ASSERT(g_studio.name == NULL);
        if (!ladish_studio_name_generate(&g_studio.name))
        {
          log_error("studio_name_generate() failed.");
          return;
        }

        g_studio.automatic = true;

        ladish_studio_publish();
      }

      ladish_studio_on_event_jack_started();

      if (g_studio.automatic)
      {
        ladish_notify_simple(
          LADISH_NOTIFY_URGENCY_NORMAL,
          "Automatic studio created",
          "JACK server is started not by ladish daemon and there is no loaded studio, so a new studio is created and marked as started.");
      }
    }
    else
    {
      /* JACK stopped but this was not expected. When expected.
       * the change will be consumed by the run method of the studio stop command */

      if (g_studio.jack_graph_proxy == NULL)
      {
        log_error("Ignoring \"JACK stopped\" notification because it is already known that JACK is currently stopped.");
        return;
      }

      if (g_studio.automatic)
      {
        log_info("Unloading automatic studio.");
        if (!ladish_command_unload_studio(NULL, &g_studio.cmd_queue)) {
          log_error("Unable to unload automatic studio.");
        }

        ladish_studio_on_event_jack_stopped();
        return;
      }

      log_error("JACK stopped unexpectedly.");
      log_error("Save your work, then unload and reload the studio.");
      ladish_notify_simple(
        LADISH_NOTIFY_URGENCY_HIGH,
        "Studio crashed",
        "JACK stopped unexpectedly.\n\n"
        "Save your work, then unload and reload the studio.");
      ladish_studio_handle_unexpected_jack_server_stop();
    }
  }

  if (ladish_environment_consume_change(&g_studio.env_store, ladish_environment_jack_server_present, &state))
  {
    if (g_studio.jack_graph_proxy != NULL)
    {
      ladish_cqueue_clear(&g_studio.cmd_queue);

      /* jack was started, this probably means that jackdbus has crashed */
      log_error("JACK disappeared unexpectedly. Maybe it crashed.");
      log_error("Save your work, then unload and reload the studio.");
      ladish_notify_simple(
        LADISH_NOTIFY_URGENCY_HIGH,
        "Studio crashed",
        "JACK disappeared unexpectedly. Maybe it crashed.\n\n"
        "Save your work, then unload and reload the studio.");
      ladish_environment_reset_stealth(&g_studio.env_store, ladish_environment_jack_server_started);

      ladish_studio_iterate_virtual_graphs(NULL, ladish_studio_hide_vgraph_non_virtual);

      ladish_studio_handle_unexpected_jack_server_stop();
    }
  }

  ladish_environment_assert_consumed(&g_studio.env_store);
}

static void ladish_studio_on_jack_server_started(void)
{
  log_info("JACK server start detected.");
  ladish_environment_set(&g_studio.env_store, ladish_environment_jack_server_started);
}

static void ladish_studio_on_jack_server_stopped(void)
{
  log_info("JACK server stop detected.");
  ladish_environment_reset(&g_studio.env_store, ladish_environment_jack_server_started);
}

static void ladish_studio_on_jack_server_appeared(void)
{
  log_info("JACK controller appeared.");
  ladish_environment_set(&g_studio.env_store, ladish_environment_jack_server_present);
}

static void ladish_studio_on_jack_server_disappeared(void)
{
  log_info("JACK controller disappeared.");
  ladish_environment_reset(&g_studio.env_store, ladish_environment_jack_server_present);
}

bool ladish_studio_init(void)
{
  char * studios_recent_store_path;

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

  studios_recent_store_path = catdup(g_base_dir, "/" RECENT_STUDIOS_STORE_FILE);
  if (studios_recent_store_path == NULL)
  {
    log_error("catdup failed for to compose recent studios store file path");
    goto free_studios_dir;
  }

  if (!ladish_recent_store_create(studios_recent_store_path, 10, &g_studios_recent_store))
  {
    free(studios_recent_store_path);
    goto free_studios_dir;
  }

  free(studios_recent_store_path);

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

  g_studio.dbus_object = NULL;
  g_studio.announced = false;
  g_studio.name = NULL;
  g_studio.filename = NULL;

  g_studio.room_count = 0;

  if (!ladish_graph_create(&g_studio.jack_graph, NULL))
  {
    log_error("ladish_graph_create() failed to create jack graph object.");
    goto destroy_recent_store;
  }

  if (!ladish_graph_create(&g_studio.studio_graph, STUDIO_OBJECT_PATH))
  {
    log_error("ladish_graph_create() failed to create studio graph object.");
    goto jack_graph_destroy;
  }

  if (!ladish_app_supervisor_create(&g_studio.app_supervisor, STUDIO_OBJECT_PATH, "studio", g_studio.studio_graph, ladish_virtualizer_rename_app))
  {
    log_error("ladish_app_supervisor_create() failed.");
    goto studio_graph_destroy;
  }

  ladish_cqueue_init(&g_studio.cmd_queue);
  ladish_environment_init(&g_studio.env_store);

  if (!jack_proxy_init(
        ladish_studio_on_jack_server_started,
        ladish_studio_on_jack_server_stopped,
        ladish_studio_on_jack_server_appeared,
        ladish_studio_on_jack_server_disappeared))
  {
    log_error("jack_proxy_init() failed.");
    goto app_supervisor_destroy;
  }

  return true;

app_supervisor_destroy:
  ladish_app_supervisor_destroy(g_studio.app_supervisor);
studio_graph_destroy:
  ladish_graph_destroy(g_studio.studio_graph);
jack_graph_destroy:
  ladish_graph_destroy(g_studio.jack_graph);
destroy_recent_store:
  ladish_recent_store_destroy(g_studios_recent_store);
free_studios_dir:
  free(g_studios_dir);
fail:
  return false;
}

void ladish_studio_uninit(void)
{
  log_info("studio_uninit()");

  jack_proxy_uninit();

  ladish_cqueue_clear(&g_studio.cmd_queue);

  ladish_graph_destroy(g_studio.studio_graph);
  ladish_graph_destroy(g_studio.jack_graph);

  ladish_recent_store_destroy(g_studios_recent_store);

  free(g_studios_dir);

  log_info("studio object destroy");
}

struct on_child_exit_context
{
  pid_t pid;
  int exit_status;
  bool found;
};

#define child_exit_context_ptr ((struct on_child_exit_context *)context)

static
bool
ladish_studio_on_child_exit_callback(
  void * context,
  ladish_graph_handle UNUSED(graph),
  ladish_app_supervisor_handle app_supervisor)
{
  child_exit_context_ptr->found = ladish_app_supervisor_child_exit(app_supervisor, child_exit_context_ptr->pid, child_exit_context_ptr->exit_status);
  /* if child is found, return false - it will cause iteration to stop */
  /* if child is not found, return true - it will cause next supervisor to be checked */
  return !child_exit_context_ptr->found;
}

#undef child_exit_context_ptr

void ladish_studio_on_child_exit(pid_t pid, int exit_status)
{
  struct on_child_exit_context context;

  context.pid = pid;
  context.exit_status = exit_status;
  context.found = false;

  ladish_studio_iterate_virtual_graphs(&context, ladish_studio_on_child_exit_callback);

  if (!context.found)
  {
    log_error("unknown child exit detected. pid is %llu", (unsigned long long)pid);
  }
}

bool ladish_studio_is_loaded(void)
{
  return g_studio.dbus_object != NULL && g_studio.announced;
}

bool ladish_studio_is_started(void)
{
  return ladish_environment_get(&g_studio.env_store, ladish_environment_jack_server_started);
}

bool ladish_studio_compose_filename(const char * name, char ** filename_ptr_ptr, char ** backup_filename_ptr_ptr)
{
  size_t len_dir;
  char * p;
  const char * src;
  char * filename_ptr;
  char * backup_filename_ptr = NULL;

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
  escape(&src, &p, LADISH_ESCAPE_FLAG_ALL);
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

struct ladish_cqueue * ladish_studio_get_cmd_queue(void)
{
  return &g_studio.cmd_queue;
}

ladish_virtualizer_handle ladish_studio_get_virtualizer(void)
{
  return g_studio.virtualizer;
}

ladish_graph_handle ladish_studio_get_jack_graph(void)
{
  return g_studio.jack_graph;
}

ladish_graph_handle ladish_studio_get_studio_graph(void)
{
  return g_studio.studio_graph;
}

ladish_app_supervisor_handle ladish_studio_get_studio_app_supervisor(void)
{
  return g_studio.app_supervisor;
}

struct ladish_studio_app_supervisor_match_opath_context
{
  const char * opath;
  ladish_app_supervisor_handle supervisor;
};

#define iterate_context_ptr ((struct ladish_studio_app_supervisor_match_opath_context *)context)

static bool ladish_studio_app_supervisor_match_opath(void * context, ladish_graph_handle graph, ladish_app_supervisor_handle app_supervisor)
{
  ASSERT(strcmp(ladish_app_supervisor_get_opath(app_supervisor), ladish_graph_get_opath(graph)) == 0);
  if (strcmp(ladish_app_supervisor_get_opath(app_supervisor), iterate_context_ptr->opath) == 0)
  {
    ASSERT(iterate_context_ptr->supervisor == NULL);
    iterate_context_ptr->supervisor = app_supervisor;
    return false;               /* stop iteration */
  }

  return true;                  /* continue iteration */
}

#undef iterate_context_ptr

ladish_app_supervisor_handle ladish_studio_find_app_supervisor(const char * opath)
{
  struct ladish_studio_app_supervisor_match_opath_context ctx;

  ctx.opath = opath;
  ctx.supervisor = NULL;
  ladish_studio_iterate_virtual_graphs(&ctx, ladish_studio_app_supervisor_match_opath);
  return ctx.supervisor;
}

struct ladish_studio_app_supervisor_match_app_context
{
  uuid_t app_uuid;
  ladish_app_handle app;
};

#define iterate_context_ptr ((struct ladish_studio_app_supervisor_match_app_context *)context)

static bool ladish_studio_app_supervisor_match_app(void * context, ladish_graph_handle graph, ladish_app_supervisor_handle app_supervisor)
{
  ASSERT(strcmp(ladish_app_supervisor_get_opath(app_supervisor), ladish_graph_get_opath(graph)) == 0);
  ASSERT(iterate_context_ptr->app == NULL);

  iterate_context_ptr->app = ladish_app_supervisor_find_app_by_uuid(app_supervisor, iterate_context_ptr->app_uuid);
  if (iterate_context_ptr->app != NULL)
  {
    return false;               /* stop iteration */
  }

  return true;                  /* continue iteration */
}

#undef iterate_context_ptr

ladish_app_handle ladish_studio_find_app_by_uuid(const uuid_t app_uuid)
{
  struct ladish_studio_app_supervisor_match_app_context ctx;

  uuid_copy(ctx.app_uuid, app_uuid);
  ctx.app = NULL;
  ladish_studio_iterate_virtual_graphs(&ctx, ladish_studio_app_supervisor_match_app);
  return ctx.app;
}

bool ladish_studio_delete(void * call_ptr, const char * studio_name)
{
  char * filename;
  char * bak_filename;
  bool ret;

  ret = false;

  if (!ladish_studio_compose_filename(studio_name, &filename, &bak_filename))
  {
    cdbus_error(call_ptr, DBUS_ERROR_FAILED, "failed to compose studio filename");
    goto exit;
  }

  log_info("Deleting studio ('%s')", filename);

  if (unlink(filename) != 0)
  {
    cdbus_error(call_ptr, DBUS_ERROR_FAILED, "unlink(%s) failed: %d (%s)", filename, errno, strerror(errno));
    goto free;
  }

  /* try to delete the backup file */
  if (unlink(bak_filename) != 0)
  {
    /* failing to delete backup file will not case delete command failure */
    log_error("unlink(%s) failed: %d (%s)", bak_filename, errno, strerror(errno));
  }

  ret = true;

free:
  free(filename);
  free(bak_filename);
exit:
  return ret;
}

bool
ladish_studio_iterate_virtual_graphs(
  void * context,
  bool (* callback)(
    void * context,
    ladish_graph_handle graph,
    ladish_app_supervisor_handle app_supervisor))
{
  struct list_head * node_ptr;
  ladish_room_handle room;
  ladish_app_supervisor_handle room_app_supervisor;
  ladish_graph_handle room_graph;

  if (!callback(context, g_studio.studio_graph, g_studio.app_supervisor))
  {
    return false;
  }

  list_for_each(node_ptr, &g_studio.rooms)
  {
    room = ladish_room_from_list_node(node_ptr);
    room_app_supervisor = ladish_room_get_app_supervisor(room);
    ASSERT(room_app_supervisor != NULL);
    room_graph = ladish_room_get_graph(room);

    if (!callback(context, room_graph, room_app_supervisor))
    {
      return false;
    }
  }

  return true;
}

bool
ladish_studio_iterate_rooms(
  void * context,
  bool (* callback)(
    void * context,
    ladish_room_handle room))
{
  struct list_head * node_ptr;
  ladish_room_handle room;

  list_for_each(node_ptr, &g_studio.rooms)
  {
    room = ladish_room_from_list_node(node_ptr);
    if (!callback(context, room))
    {
      return false;
    }
  }

  return true;
}

bool ladish_studio_has_rooms(void)
{
  return !list_empty(&g_studio.rooms);
}

static
bool
ladish_studio_stop_app_supervisor(
  void * UNUSED(context),
  ladish_graph_handle UNUSED(graph),
  ladish_app_supervisor_handle app_supervisor)
{
  ladish_app_supervisor_stop(app_supervisor);
  return true;                  /* iterate all supervisors */
}

void ladish_studio_stop_app_supervisors(void)
{
  ladish_studio_iterate_virtual_graphs(NULL, ladish_studio_stop_app_supervisor);
}

void ladish_studio_emit_renamed(void)
{
  cdbus_signal_emit(cdbus_g_dbus_connection, STUDIO_OBJECT_PATH, IFACE_STUDIO, "StudioRenamed", "s", &g_studio.name);
}

unsigned int ladish_studio_get_room_index(void)
{
  return ++g_studio.room_count;
}

void ladish_studio_release_room_index(unsigned int index)
{
  if (index == g_studio.room_count)
  {
    g_studio.room_count--;
  }
}

void ladish_studio_remove_all_rooms(void)
{
  while (!list_empty(&g_studio.rooms))
  {
    ladish_room_destroy(ladish_room_from_list_node(g_studio.rooms.next));
  }
}

ladish_room_handle ladish_studio_find_room_by_uuid(const uuid_t room_uuid_ptr)
{
  struct list_head * node_ptr;
  ladish_room_handle room;
  uuid_t room_uuid;

  list_for_each(node_ptr, &g_studio.rooms)
  {
    room = ladish_room_from_list_node(node_ptr);
    ladish_room_get_uuid(room, room_uuid);
    if (uuid_compare(room_uuid, room_uuid_ptr) == 0)
    {
      return room;
    }
  }

  return NULL;
}

bool ladish_studio_check_room_name(const char * room_name)
{
  struct list_head * node_ptr;
  ladish_room_handle room;

  list_for_each(node_ptr, &g_studio.rooms)
  {
    room = ladish_room_from_list_node(node_ptr);
    if (strcmp(ladish_room_get_name(room), room_name) == 0)
    {
      return true;
    }
  }

  return false;
}

/**********************************************************************************/
/*                                D-Bus methods                                   */
/**********************************************************************************/

static void ladish_studio_dbus_get_name(struct cdbus_method_call * call_ptr)
{
  cdbus_method_return_new_single(call_ptr, DBUS_TYPE_STRING, &g_studio.name);
}

static void ladish_studio_dbus_rename(struct cdbus_method_call * call_ptr)
{
  const char * new_name;
  char * new_name_dup;
  
  if (!dbus_message_get_args(call_ptr->message, &cdbus_g_dbus_error, DBUS_TYPE_STRING, &new_name, DBUS_TYPE_INVALID))
  {
    cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  log_info("Rename studio request (%s)", new_name);

  new_name_dup = strdup(new_name);
  if (new_name_dup == NULL)
  {
    cdbus_error(call_ptr, DBUS_ERROR_FAILED, "strdup() failed to allocate new name.");
    return;
  }

  free(g_studio.name);
  g_studio.name = new_name_dup;

  cdbus_method_return_new_void(call_ptr);
  ladish_studio_emit_renamed();
}

static void ladish_studio_dbus_save(struct cdbus_method_call * call_ptr)
{
  log_info("Save studio request");

  if (ladish_command_save_studio(call_ptr, &g_studio.cmd_queue, g_studio.name))
  {
    cdbus_method_return_new_void(call_ptr);
  }
}

static void ladish_studio_dbus_save_as(struct cdbus_method_call * call_ptr)
{
  const char * new_name;

  log_info("SaveAs studio request");

  if (!dbus_message_get_args(call_ptr->message, &cdbus_g_dbus_error, DBUS_TYPE_STRING, &new_name, DBUS_TYPE_INVALID))
  {
    cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  if (ladish_command_save_studio(call_ptr, &g_studio.cmd_queue, new_name))
  {
    cdbus_method_return_new_void(call_ptr);
  }
}

static void ladish_studio_dbus_unload(struct cdbus_method_call * call_ptr)
{
  log_info("Unload studio request");

  if (ladish_command_unload_studio(call_ptr, &g_studio.cmd_queue))
  {
    cdbus_method_return_new_void(call_ptr);
  }
}

static void ladish_studio_dbus_stop(struct cdbus_method_call * call_ptr)
{
  log_info("Stop studio request");

  g_studio.automatic = false;   /* even if it was automatic, it is not anymore because user knows about it */

  if (ladish_command_stop_studio(call_ptr, &g_studio.cmd_queue))
  {
    cdbus_method_return_new_void(call_ptr);
  }
}

static void ladish_studio_dbus_start(struct cdbus_method_call * call_ptr)
{
  log_info("Start studio request");

  g_studio.automatic = false;   /* even if it was automatic, it is not anymore because user knows about it */

  if (ladish_command_start_studio(call_ptr, &g_studio.cmd_queue))
  {
    cdbus_method_return_new_void(call_ptr);
  }
}

static void ladish_studio_dbus_is_started(struct cdbus_method_call * call_ptr)
{
  dbus_bool_t started;

  started = g_studio.jack_graph_proxy != NULL;

  cdbus_method_return_new_single(call_ptr, DBUS_TYPE_BOOLEAN, &started);
}

static void ladish_studio_dbus_create_room(struct cdbus_method_call * call_ptr)
{
  const char * room_name;
  const char * template_name;

  dbus_error_init(&cdbus_g_dbus_error);

  if (!dbus_message_get_args(call_ptr->message, &cdbus_g_dbus_error, DBUS_TYPE_STRING, &room_name, DBUS_TYPE_STRING, &template_name, DBUS_TYPE_INVALID))
  {
    cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  if (ladish_command_create_room(call_ptr, ladish_studio_get_cmd_queue(), room_name, template_name))
  {
    cdbus_method_return_new_void(call_ptr);
  }
}

static void ladish_studio_dbus_get_room_list(struct cdbus_method_call * call_ptr)
{
  DBusMessageIter iter, array_iter;
  DBusMessageIter struct_iter;
  struct list_head * node_ptr;
  ladish_room_handle room;

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

  list_for_each(node_ptr, &g_studio.rooms)
  {
    room = ladish_room_from_list_node(node_ptr);

    if (!dbus_message_iter_open_container(&array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter))
      goto fail_unref;

    if (!ladish_studio_fill_room_info(&struct_iter, room))
      goto fail_unref;

    if (!dbus_message_iter_close_container(&array_iter, &struct_iter))
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

static void ladish_studio_dbus_delete_room(struct cdbus_method_call * call_ptr)
{
  const char * name;

  dbus_error_init(&cdbus_g_dbus_error);

  if (!dbus_message_get_args(call_ptr->message, &cdbus_g_dbus_error, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID))
  {
    cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  if (ladish_command_delete_room(call_ptr, ladish_studio_get_cmd_queue(), name))
  {
    cdbus_method_return_new_void(call_ptr);
  }
}

CDBUS_METHOD_ARGS_BEGIN(GetName, "Get studio name")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("studio_name", "s", "Name of studio")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(Rename, "Rename studio")
  CDBUS_METHOD_ARG_DESCRIBE_IN("studio_name", "s", "New name")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(Save, "Save studio")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(SaveAs, "SaveAs studio")
  CDBUS_METHOD_ARG_DESCRIBE_IN("studio_name", "s", "New name")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(Unload, "Unload studio")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(Start, "Start studio")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(Stop, "Stop studio")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(IsStarted, "Check whether studio is started")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("started", "b", "Whether studio is started")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(CreateRoom, "Create new studio room")
  CDBUS_METHOD_ARG_DESCRIBE_IN("room_name", "s", "Studio room name")
  CDBUS_METHOD_ARG_DESCRIBE_IN("room_template_name", "s", "Room template name")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(GetRoomList, "Get list of rooms in this studio")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("room_list", "a(sa{sv})", "List of studio rooms: opaths and properties")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(DeleteRoom, "Delete studio room")
  CDBUS_METHOD_ARG_DESCRIBE_IN("room_name", "s", "Name of studio room to delete")
CDBUS_METHOD_ARGS_END

CDBUS_METHODS_BEGIN
  CDBUS_METHOD_DESCRIBE(GetName, ladish_studio_dbus_get_name)          /* sync */
  CDBUS_METHOD_DESCRIBE(Rename, ladish_studio_dbus_rename)             /* sync */
  CDBUS_METHOD_DESCRIBE(Save, ladish_studio_dbus_save)                 /* async */
  CDBUS_METHOD_DESCRIBE(SaveAs, ladish_studio_dbus_save_as)            /* async */
  CDBUS_METHOD_DESCRIBE(Unload, ladish_studio_dbus_unload)             /* async */
  CDBUS_METHOD_DESCRIBE(Start, ladish_studio_dbus_start)               /* async */
  CDBUS_METHOD_DESCRIBE(Stop, ladish_studio_dbus_stop)                 /* async */
  CDBUS_METHOD_DESCRIBE(IsStarted, ladish_studio_dbus_is_started)      /* sync */
  CDBUS_METHOD_DESCRIBE(CreateRoom, ladish_studio_dbus_create_room)    /* async */
  CDBUS_METHOD_DESCRIBE(GetRoomList, ladish_studio_dbus_get_room_list) /* sync */
  CDBUS_METHOD_DESCRIBE(DeleteRoom, ladish_studio_dbus_delete_room)    /* async */
CDBUS_METHODS_END

CDBUS_SIGNAL_ARGS_BEGIN(StudioRenamed, "Studio name changed")
  CDBUS_SIGNAL_ARG_DESCRIBE("studio_name", "s", "New studio name")
CDBUS_SIGNAL_ARGS_END

CDBUS_SIGNAL_ARGS_BEGIN(StudioStarted, "Studio started")
CDBUS_SIGNAL_ARGS_END

CDBUS_SIGNAL_ARGS_BEGIN(StudioCrashed, "Studio crashed")
CDBUS_SIGNAL_ARGS_END

CDBUS_SIGNAL_ARGS_BEGIN(StudioStopped, "Studio stopped")
CDBUS_SIGNAL_ARGS_END

CDBUS_SIGNAL_ARGS_BEGIN(RoomAppeared, "Room D-Bus object appeared")
  CDBUS_SIGNAL_ARG_DESCRIBE("opath", "s", "room object path")
  CDBUS_SIGNAL_ARG_DESCRIBE("properties", "a{sv}", "room properties")
CDBUS_SIGNAL_ARGS_END

CDBUS_SIGNAL_ARGS_BEGIN(RoomChanged, "Room D-Bus object changed")
  CDBUS_SIGNAL_ARG_DESCRIBE("opath", "s", "room object path")
  CDBUS_SIGNAL_ARG_DESCRIBE("properties", "a{sv}", "room properties")
CDBUS_SIGNAL_ARGS_END

CDBUS_SIGNAL_ARGS_BEGIN(RoomDisappeared, "Room D-Bus object disappeared")
  CDBUS_SIGNAL_ARG_DESCRIBE("opath", "s", "room object path")
  CDBUS_SIGNAL_ARG_DESCRIBE("properties", "a{sv}", "room properties")
CDBUS_SIGNAL_ARGS_END

CDBUS_SIGNALS_BEGIN
  CDBUS_SIGNAL_DESCRIBE(StudioRenamed)
  CDBUS_SIGNAL_DESCRIBE(StudioStarted)
  CDBUS_SIGNAL_DESCRIBE(StudioCrashed)
  CDBUS_SIGNAL_DESCRIBE(StudioStopped)
  CDBUS_SIGNAL_DESCRIBE(RoomAppeared)
  CDBUS_SIGNAL_DESCRIBE(RoomDisappeared)
  CDBUS_SIGNAL_DESCRIBE(RoomChanged)
CDBUS_SIGNALS_END

CDBUS_INTERFACE_DEFAULT_HANDLER_METHODS_AND_SIGNALS(g_interface_studio, IFACE_STUDIO)
