/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the graph virtualizer object
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

#include "virtualizer.h"
#include "../dbus_constants.h"
#include "../proxies/a2j_proxy.h"
#include "../proxies/jmcore_proxy.h"
#include "procfs.h"
#include "app_supervisor.h"
#include "studio_internal.h"
#include "../catdup.h"
#include "room.h"
#include "studio.h"

struct virtualizer
{
  graph_proxy_handle jack_graph_proxy;
  ladish_graph_handle jack_graph;
  uint64_t system_client_id;
  unsigned int our_clients_count;
};

/* 47c1cd18-7b21-4389-bec4-6e0658e1d6b1 */
UUID_DEFINE(g_system_capture_uuid,0x47,0xC1,0xCD,0x18,0x7B,0x21,0x43,0x89,0xBE,0xC4,0x6E,0x06,0x58,0xE1,0xD6,0xB1);

/* b2a0bb06-28d8-4bfe-956e-eb24378f9629 */
UUID_DEFINE(g_system_playback_uuid,0xB2,0xA0,0xBB,0x06,0x28,0xD8,0x4B,0xFE,0x95,0x6E,0xEB,0x24,0x37,0x8F,0x96,0x29);

/* be23a242-e2b2-11de-b795-002618af5e42 */
UUID_DEFINE(g_a2j_uuid,0xBE,0x23,0xA2,0x42,0xE2,0xB2,0x11,0xDE,0xB7,0x95,0x00,0x26,0x18,0xAF,0x5E,0x42);

struct app_find_context
{
  pid_t pid;
  char * app_name;
  ladish_graph_handle graph;
};

#define app_find_context_ptr ((struct app_find_context *)context)

bool get_app_name_from_supervisor(void * context, ladish_graph_handle graph, ladish_app_supervisor_handle app_supervisor)
{
  pid_t pid;
  char * app_name;

  ASSERT(app_find_context_ptr->app_name == NULL); /* we stop iteration when app is found */

  //log_info("checking app supervisor \"%s\" for pid %llu", ladish_app_supervisor_get_name(app_supervisor), (unsigned long long)pid);

  pid = app_find_context_ptr->pid;
  do
  {
    app_name = ladish_app_supervisor_search_app(app_supervisor, pid);
    if (app_name != NULL)
      break;

    pid = (pid_t)procfs_get_process_parent((unsigned long long)pid);
#if 0
    if (pid != 0)
    {
      log_info("parent pid %llu", (unsigned long long)pid);
    }
#endif
  }
  while (pid != 0);

  if (app_name != NULL)
  {
    app_find_context_ptr->app_name = app_name;
    app_find_context_ptr->pid = pid;
    app_find_context_ptr->graph = graph;
    return false;               /* stop app supervisor iteration */
  }

  return true;                  /* continue app supervisor iteration */
}

#undef app_find_context_ptr

char * get_app_name(struct virtualizer * virtualizer_ptr, uint64_t client_id, pid_t pid, ladish_graph_handle * graph_ptr)
{
  struct app_find_context context;

  context.pid = (pid_t)pid;
  context.app_name = NULL;
  context.graph = NULL;

  studio_iterate_virtual_graphs(&context, get_app_name_from_supervisor);

  if (context.app_name != NULL)
  {
    ASSERT(context.graph != NULL);
    *graph_ptr = context.graph;
  }

  return context.app_name;
}

struct find_link_port_context
{
  uuid_t uuid;
  uint64_t jack_id;
  ladish_port_handle port;
  ladish_graph_handle graph;
};

#define find_link_port_context_ptr ((struct find_link_port_context *)context)

static bool find_link_port_vgraph_callback_by_uuid(void * context, ladish_graph_handle graph, ladish_app_supervisor_handle app_supervisor)
{
  ladish_port_handle port;

  port = ladish_graph_find_port_by_uuid(graph, find_link_port_context_ptr->uuid, true);
  if (port != NULL)
  {
    find_link_port_context_ptr->port = port;
    find_link_port_context_ptr->graph = graph;
    return false;
  }

  return true;                  /* continue vgraph iteration */
}

static bool find_link_port_vgraph_callback_by_jack_id(void * context, ladish_graph_handle graph, ladish_app_supervisor_handle app_supervisor)
{
  ladish_port_handle port;
  bool room;

  log_info("searching link port with jack id %"PRIu64" in graph %s", find_link_port_context_ptr->jack_id, ladish_graph_get_description(graph));

  room = graph != g_studio.studio_graph;

  port = ladish_graph_find_port_by_jack_id(graph, find_link_port_context_ptr->jack_id, room, !room);
  if (port != NULL)
  {
    find_link_port_context_ptr->port = port;
    find_link_port_context_ptr->graph = graph;
    return false;
  }

  return true;                  /* continue vgraph iteration */
}

#undef find_link_port_context_ptr

static ladish_graph_handle find_link_port_vgraph_by_uuid(struct virtualizer * virtualizer_ptr, const char * port_name, ladish_port_handle * port_ptr)
{
  struct find_link_port_context context;

  uuid_parse(port_name, context.uuid);
  context.graph = NULL;
  context.port = NULL;

  studio_iterate_virtual_graphs(&context, find_link_port_vgraph_callback_by_uuid);

  if (port_ptr != NULL && context.graph != NULL)
  {
    *port_ptr = context.port;
  }

  return context.graph;
}

static ladish_graph_handle find_link_port_vgraph_by_jack_id(struct virtualizer * virtualizer_ptr, uint64_t jack_id, ladish_port_handle * port_ptr)
{
  struct find_link_port_context context;

  context.jack_id = jack_id;
  context.graph = NULL;
  context.port = NULL;

  studio_iterate_virtual_graphs(&context, find_link_port_vgraph_callback_by_jack_id);

  if (port_ptr != NULL && context.graph != NULL)
  {
    *port_ptr = context.port;
  }

  return context.graph;
}

static
bool
lookup_port(
  struct virtualizer * virtualizer_ptr,
  uint64_t port_id,
  ladish_port_handle * port_ptr,
  ladish_graph_handle * vgraph_ptr)
{
  ladish_port_handle port;
  ladish_client_handle jclient;
  ladish_graph_handle vgraph;

  port = ladish_graph_find_port_by_jack_id(virtualizer_ptr->jack_graph, port_id, true, true);
  if (port == NULL)
  {
    log_error("Unknown JACK port with id %"PRIu64" (dis)connected", port_id);
    return false;
  }

  jclient = ladish_graph_get_port_client(virtualizer_ptr->jack_graph, port);
  if (jclient == NULL)
  {
    log_error("Port %"PRIu64" without jack client was (dis)connected", port_id);
    return false;
  }

  vgraph = ladish_client_get_vgraph(jclient);
  if (vgraph == NULL)
  {
    vgraph = find_link_port_vgraph_by_jack_id(virtualizer_ptr, port_id, NULL);
    if (vgraph == NULL)
    {
      log_error("Cannot find vgraph for (dis)connected jmcore port");
      return false;
    }

    log_info("link port found in graph %s", ladish_graph_get_description(vgraph));
  }

  *port_ptr = port;
  *vgraph_ptr = vgraph;
  return true;
}

#define virtualizer_ptr ((struct virtualizer *)context)

static void clear(void * context)
{
  log_info("clear");
}

static void client_appeared(void * context, uint64_t id, const char * jack_name)
{
  ladish_client_handle client;
  const char * a2j_name;
  bool is_a2j;
  char * app_name;
  const char * name;
  pid_t pid;
  ladish_graph_handle graph;
  bool jmcore;

  log_info("client_appeared(%"PRIu64", %s)", id, jack_name);

  a2j_name = a2j_proxy_get_jack_client_name_cached();
  is_a2j = a2j_name != NULL && strcmp(a2j_name, jack_name) == 0;

  name = jack_name;
  app_name = NULL;
  jmcore = false;

  if (!graph_proxy_get_client_pid(virtualizer_ptr->jack_graph_proxy, id, &pid))
  {
    log_info("client %"PRIu64" pid is unknown", id);
  }
  else
  {
    log_info("client pid is %"PRId64, (int64_t)pid);

    if (pid != 0) /* skip internal clients that will match the pending clients in the graph, both have zero pid */
    {
      jmcore = pid == jmcore_proxy_get_pid_cached();
      if (jmcore)
      {
        log_info("jmcore client appeared");
      }
      else
      {
        app_name = get_app_name(virtualizer_ptr, id, pid, &graph);
        if (app_name != NULL)
        {
          log_info("app name is '%s'", app_name);
          name = app_name;
        }
      }
    }
  }

  if (!jmcore)
  {
    if (is_a2j)
    {
      client = ladish_graph_find_client_by_uuid(virtualizer_ptr->jack_graph, g_a2j_uuid);
    }
    else
    {
      client = ladish_graph_find_client_by_name(virtualizer_ptr->jack_graph, name);
    }

    if (client != NULL)
    {
      log_info("found existing client");
      if (ladish_client_get_jack_id(client) != 0)
      {
        log_error("Ignoring client with duplicate name '%s' ('%s')", name, jack_name);
        goto free_app_name;
      }

      ladish_client_set_jack_id(client, id);
      ladish_graph_show_client(virtualizer_ptr->jack_graph, client);
      goto done;
    }
  }

  if (!ladish_client_create(is_a2j ? g_a2j_uuid : NULL, &client))
  {
    log_error("ladish_client_create() failed. Ignoring client %"PRIu64" (%s)", id, jack_name);
    goto free_app_name;
  }

  ladish_client_set_jack_id(client, id);

  if (!ladish_graph_add_client(virtualizer_ptr->jack_graph, client, name, false))
  {
    log_error("ladish_graph_add_client() failed to add client %"PRIu64" (%s) to JACK graph", id, name);
    ladish_client_destroy(client);
    goto free_app_name;
  }

done:
  if (strcmp(jack_name, "system") == 0)
  {
    virtualizer_ptr->system_client_id = id;
  }

  if (app_name != NULL)
  {
    ladish_client_set_pid(client, pid);
    ladish_client_set_vgraph(client, graph);
    virtualizer_ptr->our_clients_count++;
  }
  else if (jmcore)
  {
    ladish_client_set_pid(client, pid);
    ASSERT(ladish_client_get_vgraph(client) == NULL);
  }
  else
  {
    /* unknown and internal clients appear in the studio graph */
    ladish_client_set_vgraph(client, g_studio.studio_graph);
  }

free_app_name:
  if (app_name != NULL)
  {
    free(app_name);
  }
}

static void client_disappeared(void * context, uint64_t id)
{
  ladish_client_handle client;
  pid_t pid;

  log_info("client_disappeared(%"PRIu64")", id);

  client = ladish_graph_find_client_by_jack_id(virtualizer_ptr->jack_graph, id);
  if (client == NULL)
  {
    log_error("Unknown JACK client with id %"PRIu64" disappeared", id);
    return;
  }

  log_info("client disappeared: '%s'", ladish_graph_get_client_name(virtualizer_ptr->jack_graph, client));

  if (ladish_client_get_vgraph(client) == NULL)
  { /* remove jmcore clients, the are not persisted in the jack graph */
    ladish_graph_remove_client(virtualizer_ptr->jack_graph, client);
    ladish_client_destroy(client);
    return;
  }

  pid = ladish_client_get_pid(client);
  if (pid != 0)
  {
    virtualizer_ptr->our_clients_count--;
  }

  if (id == virtualizer_ptr->system_client_id)
  {
    virtualizer_ptr->system_client_id = 0;
  }

  if (true)                     /* if client is supposed to be persisted */
  {
    ladish_client_set_jack_id(client, 0);
    ladish_graph_hide_client(virtualizer_ptr->jack_graph, client);
  }
  else
  {
    ladish_graph_remove_client(virtualizer_ptr->jack_graph, client);
    ladish_client_destroy(client);
  }
}

static
void
port_appeared(
  void * context,
  uint64_t client_id,
  uint64_t port_id,
  const char * real_jack_port_name,
  bool is_input,
  bool is_terminal,
  bool is_midi)
{
  ladish_client_handle jack_client;
  ladish_client_handle vclient;
  ladish_port_handle port;
  uint32_t type;
  uint32_t flags;
  const char * jack_client_name;
  bool is_a2j;
  uuid_t client_uuid;
  char * alsa_client_name;
  char * alsa_port_name;
  char * a2j_fake_jack_port_name = NULL;
  uint32_t alsa_client_id;
  const char * jack_port_name;
  const char * vport_name;
  ladish_graph_handle vgraph;

  log_info("port_appeared(%"PRIu64", %"PRIu64", %s (%s, %s))", client_id, port_id, real_jack_port_name, is_input ? "in" : "out", is_midi ? "midi" : "audio");

  type = is_midi ? JACKDBUS_PORT_TYPE_MIDI : JACKDBUS_PORT_TYPE_AUDIO;
  flags = is_input ? JACKDBUS_PORT_FLAG_INPUT : JACKDBUS_PORT_FLAG_OUTPUT;
  if (is_terminal)
  {
    flags |= JACKDBUS_PORT_FLAG_TERMINAL;
  }

  /********************/
  /* gather info about the appeared port */

  jack_client = ladish_graph_find_client_by_jack_id(virtualizer_ptr->jack_graph, client_id);
  if (jack_client == NULL)
  {
    log_error("Port of unknown JACK client with id %"PRIu64" appeared", client_id);
    goto exit;
  }

  /* find the virtual graph that owns the app that owns the client that owns the appeared port */
  vgraph = ladish_client_get_vgraph(jack_client);
  if (vgraph == NULL)
  {
    vgraph = find_link_port_vgraph_by_uuid(virtualizer_ptr, real_jack_port_name, &port);
    if (vgraph == NULL)
    {
      log_error("Cannot find vgraph for appeared jmcore port '%s'", real_jack_port_name);
      goto exit;
    }

    /* jmcore port appeared */

    log_info("jmcore port appeared in vgraph %s", ladish_graph_get_description(vgraph));

    if (!ladish_graph_add_port(virtualizer_ptr->jack_graph, jack_client, port, real_jack_port_name, type, flags, false))
    {
      log_error("ladish_graph_add_port() failed.");
      goto exit;
    }

    if (vgraph == g_studio.studio_graph)
    {
      ladish_port_set_jack_id(port, port_id);
    }
    else
    {
      ladish_port_set_jack_id_room(port, port_id);
    }

    vclient = ladish_graph_get_port_client(vgraph, port);
    if (vclient == NULL)
    {
      log_error("link port client not found in vgraph %s", ladish_graph_get_description(vgraph));
      ASSERT_NO_PASS;
      goto exit;
    }

    ladish_graph_show_port(vgraph, port);
    goto exit;
  }

  ladish_client_get_uuid(jack_client, client_uuid);
  is_a2j = uuid_compare(client_uuid, g_a2j_uuid) == 0;
  if (is_a2j)
  {
    log_info("a2j port appeared");
    if (!a2j_proxy_map_jack_port(real_jack_port_name, &alsa_client_name, &alsa_port_name, &alsa_client_id))
    {
      is_a2j = false;
    }
    else
    {
      log_info("a2j: '%s':'%s' (%"PRIu32")", alsa_client_name, alsa_port_name, alsa_client_id);
    }

    a2j_fake_jack_port_name = catdup4(alsa_client_name, is_input ? " (playback)" : " (capture)", ": ", alsa_port_name);
    if (a2j_fake_jack_port_name == NULL)
    {
      log_error("catdup4() failed");
      goto free_alsa_names;
    }

    jack_port_name = a2j_fake_jack_port_name;
  }
  else
  {
    jack_port_name = real_jack_port_name;
  }

  jack_client_name = ladish_graph_get_client_name(virtualizer_ptr->jack_graph, jack_client);

  /********************/

  /* search (by name) the appeared port in jack graph
   * if found - show it in both graphs.
   * if not found - create new port and add it to the jack graph.
   * Then process to adding it to virtual graph */

  port = ladish_graph_find_port_by_name(virtualizer_ptr->jack_graph, jack_client, jack_port_name);
  if (port != NULL)
  {
    log_info("found existing port");

    if (ladish_port_get_jack_id(port) != 0)
    {
      log_error("Ignoring duplicate JACK port '%s':'%s'", jack_client_name, jack_port_name);
      goto free_alsa_names;
    }

    ladish_port_set_jack_id(port, port_id);
    ladish_graph_adjust_port(virtualizer_ptr->jack_graph, port, type, flags);
    ladish_graph_show_port(virtualizer_ptr->jack_graph, port);

    vclient = ladish_graph_get_port_client(vgraph, port);
    if (vclient == NULL)
    {
      log_error("JACK port not found in virtual graph");
      ASSERT_NO_PASS;
      goto free_alsa_names;
    }

    ladish_client_set_jack_id(vclient, client_id);
    ladish_graph_adjust_port(vgraph, port, type, flags);
    ladish_graph_show_port(vgraph, port);
    goto free_alsa_names;
  }

  if (!ladish_port_create(NULL, false, &port))
  {
    log_error("ladish_port_create() failed.");
    goto free_alsa_names;
  }

  /* set port jack id so invisible connections to/from it can be restored */
  ladish_port_set_jack_id(port, port_id);

  if (!ladish_graph_add_port(virtualizer_ptr->jack_graph, jack_client, port, jack_port_name, type, flags, false))
  {
    log_error("ladish_graph_add_port() failed.");
    ladish_port_destroy(port);
    goto free_alsa_names;
  }

  /********************/
  /* find/create the virtual client where port will be added */

  if (is_a2j)
  {
    vclient = ladish_graph_find_client_by_name(vgraph, alsa_client_name);
    if (vclient == NULL)
    {
        if (!ladish_client_create(NULL, &vclient))
        {
          log_error("ladish_client_create() failed.");
          goto free_alsa_names;
        }

        if (!ladish_graph_add_client(vgraph, vclient, alsa_client_name, false))
        {
          log_error("ladish_graph_add_client() failed.");
          ladish_client_destroy(vclient);
          goto free_alsa_names;
        }
    }

  }
  else if (client_id == virtualizer_ptr->system_client_id)
  {
    log_info("system client port appeared");

    if (!is_input)
    { /* output capture port */

      vclient = ladish_graph_find_client_by_uuid(vgraph, g_system_capture_uuid);
      if (vclient == NULL)
      {
        if (!ladish_client_create(g_system_capture_uuid, &vclient))
        {
          log_error("ladish_client_create() failed.");
          goto free_alsa_names;
        }

        if (!ladish_graph_add_client(vgraph, vclient, "Hardware Capture", false))
        {
          log_error("ladish_graph_add_client() failed.");
          ladish_client_destroy(vclient);
          goto free_alsa_names;
        }
      }
    }
    else
    { /* input playback port */
      vclient = ladish_graph_find_client_by_uuid(vgraph, g_system_playback_uuid);
      if (vclient == NULL)
      {
        if (!ladish_client_create(g_system_playback_uuid, &vclient))
        {
          log_error("ladish_client_create() failed.");
          goto free_alsa_names;
        }

        if (!ladish_graph_add_client(vgraph, vclient, "Hardware Playback", false))
        {
          ladish_client_destroy(vclient);
          goto free_alsa_names;
        }
      }
    }
  }
  else
  { /* non-system client */
    log_info("non-system client port appeared");

    vclient = ladish_graph_find_client_by_jack_id(vgraph, client_id);
    if (vclient == NULL)
    {
      if (!ladish_client_create(NULL, &vclient))
      {
        log_error("ladish_client_create() failed.");
        goto free_alsa_names;
      }

      ladish_client_set_jack_id(vclient, client_id);

      if (!ladish_graph_add_client(vgraph, vclient, jack_client_name, false))
      {
        log_error("ladish_graph_add_client() failed to add client '%s' to virtual graph", jack_client_name);
        ladish_client_destroy(vclient);
        goto free_alsa_names;
      }
    }
  }

  /********************/
  /* add newly appeared port to the virtual graph */

  if (is_a2j)
  {
    vport_name = alsa_port_name;
  }
  else
  {
    vport_name = jack_port_name;
  }

  if (!ladish_graph_add_port(vgraph, vclient, port, vport_name, type, flags, false))
  {
    log_error("ladish_graph_add_port() failed.");
    goto free_alsa_names;
  }

free_alsa_names:
  if (a2j_fake_jack_port_name != NULL)
  {
    free(a2j_fake_jack_port_name);
  }

  if (is_a2j)
  {
    free(alsa_client_name);
    free(alsa_port_name);
  }

exit:
  return;
}

static void port_disappeared(void * context, uint64_t client_id, uint64_t port_id)
{
  ladish_client_handle client;
  ladish_port_handle port;
  ladish_graph_handle vgraph;
  bool jmcore;

  log_info("port_disappeared(%"PRIu64", %"PRIu64")", client_id, port_id);

  client = ladish_graph_find_client_by_jack_id(virtualizer_ptr->jack_graph, client_id);
  if (client == NULL)
  {
    log_error("Port of unknown JACK client with id %"PRIu64" disappeared", client_id);
    return;
  }

  port = ladish_graph_find_port_by_jack_id(virtualizer_ptr->jack_graph, port_id, true, true);
  if (port == NULL)
  {
    log_error("Unknown JACK port with id %"PRIu64" disappeared", port_id);
    return;
  }

  /* find the virtual graph that owns the app that owns the client that owns the disappeared port */
  jmcore = false;
  vgraph = ladish_client_get_vgraph(client);
  if (vgraph == NULL)
  {
    vgraph = find_link_port_vgraph_by_uuid(virtualizer_ptr, ladish_graph_get_port_name(virtualizer_ptr->jack_graph, port), NULL);
    if (vgraph == NULL)
    {
      log_error("Cannot find vgraph for disappeared jmcore port");
    }
    else
    {
      jmcore = true;
      ladish_graph_remove_port_by_jack_id(virtualizer_ptr->jack_graph, port_id, true, true);
    }
  }

  if (true)                     /* if client is supposed to be persisted */
  {
    if (!jmcore)
    {
      ladish_port_set_jack_id(port, 0);
      ladish_graph_hide_port(virtualizer_ptr->jack_graph, port);
    }
    if (vgraph != NULL)
    {
      ladish_graph_hide_port(vgraph, port);
      client = ladish_graph_get_port_client(vgraph, port);
      if (ladish_graph_is_client_looks_empty(vgraph, client))
      {
        ladish_graph_hide_client(vgraph, client);
      }
    }
  }
  else
  {
    if (!jmcore)
    {
      ladish_graph_remove_port(virtualizer_ptr->jack_graph, port);
    }

    if (vgraph != NULL)
    {
      client = ladish_graph_remove_port(vgraph, port);
      if (client != NULL)
      {
        if (ladish_graph_is_client_empty(vgraph, client))
        {
          ladish_graph_remove_client(vgraph, client);
        }
      }
    }
  }
}

static void port_renamed(void * context, uint64_t client_id, uint64_t port_id, const char * old_port_name, const char * new_port_name)
{
  ladish_client_handle client;
  ladish_port_handle port;
  ladish_graph_handle vgraph;

  log_info("port_renamed(%"PRIu64", '%s', '%s')", port_id, old_port_name, new_port_name);

  client = ladish_graph_find_client_by_jack_id(virtualizer_ptr->jack_graph, client_id);
  if (client == NULL)
  {
    log_error("Port of unknown JACK client with id %"PRIu64" was renamed", client_id);
    return;
  }

  /* find the virtual graph that owns the app that owns the client that owns the renamed port */
  vgraph = ladish_client_get_vgraph(client);

  port = ladish_graph_find_port_by_jack_id(virtualizer_ptr->jack_graph, port_id, true, true);
  if (port == NULL)
  {
    log_error("Unknown JACK port with id %"PRIu64" was renamed", port_id);
    return;
  }

  if (!ladish_graph_rename_port(virtualizer_ptr->jack_graph, port, new_port_name))
  {
    log_error("renaming of port in jack graph failed");
  }

  if (!ladish_graph_rename_port(vgraph, port, new_port_name))
  {
    log_error("renaming of port in virtual graph failed");
  }
}

static bool ports_connect_request(void * context, ladish_graph_handle graph_handle, ladish_port_handle port1, ladish_port_handle port2)
{
  uint64_t port1_id;
  uint64_t port2_id;

  ASSERT(ladish_graph_get_opath(graph_handle)); /* studio or room virtual graph */
  log_info("virtualizer: ports connect request");

  if (graph_handle == g_studio.studio_graph)
  {
    port1_id = ladish_port_get_jack_id(port1);
    port2_id = ladish_port_get_jack_id(port2);
  }
  else
  {
    port1_id = ladish_port_get_jack_id_room(port1);
    port2_id = ladish_port_get_jack_id_room(port2);
  }

  graph_proxy_connect_ports(virtualizer_ptr->jack_graph_proxy, port1_id, port2_id);

  return true;
}

static bool ports_disconnect_request(void * context, ladish_graph_handle graph_handle, uint64_t connection_id)
{
  ladish_port_handle port1;
  ladish_port_handle port2;
  uint64_t port1_id;
  uint64_t port2_id;

  ASSERT(ladish_graph_get_opath(graph_handle)); /* studio or room virtual graph */
  log_info("virtualizer: ports disconnect request");

  if (!ladish_graph_get_connection_ports(graph_handle, connection_id, &port1, &port2))
  {
    log_error("cannot find ports that are disconnect-requested");
    ASSERT_NO_PASS;
    return false;
  }

  if (graph_handle == g_studio.studio_graph)
  {
    port1_id = ladish_port_get_jack_id(port1);
    port2_id = ladish_port_get_jack_id(port2);
  }
  else
  {
    port1_id = ladish_port_get_jack_id_room(port1);
    port2_id = ladish_port_get_jack_id_room(port2);
  }

  graph_proxy_disconnect_ports(virtualizer_ptr->jack_graph_proxy, port1_id, port2_id);

  return true;
}

static void ports_connected(void * context, uint64_t client1_id, uint64_t port1_id, uint64_t client2_id, uint64_t port2_id)
{
  ladish_port_handle port1;
  ladish_port_handle port2;
  uint64_t connection_id;
  ladish_graph_handle vgraph1;
  ladish_graph_handle vgraph2;

  log_info("ports_connected %"PRIu64":%"PRIu64" %"PRIu64":%"PRIu64"", client1_id, port1_id, client2_id, port2_id);

  if (!lookup_port(virtualizer_ptr, port1_id, &port1, &vgraph1))
  {
    return;
  }

  if (!lookup_port(virtualizer_ptr, port2_id, &port2, &vgraph2))
  {
    return;
  }

  if (vgraph1 != vgraph2)
  {
    /* TODO */
    log_error("ignoring connection with endpoints in different vgraphs");
    return;
  }

  ladish_graph_add_connection(virtualizer_ptr->jack_graph, port1, port2, false);

  if (ladish_graph_find_connection(vgraph1, port1, port2, &connection_id))
  {
    log_info("showing hidden virtual connection");
    ladish_graph_show_connection(vgraph1, connection_id);
  }
  else
  {
    log_info("creating new virtual connection");
    ladish_graph_add_connection(vgraph1, port1, port2, false);
  }
}

static void ports_disconnected(void * context, uint64_t client1_id, uint64_t port1_id, uint64_t client2_id, uint64_t port2_id)
{
  ladish_port_handle port1;
  ladish_port_handle port2;
  uint64_t connection_id;
  ladish_graph_handle vgraph1;
  ladish_graph_handle vgraph2;

  log_info("ports_disconnected %"PRIu64":%"PRIu64" %"PRIu64":%"PRIu64"", client1_id, port1_id, client2_id, port2_id);

  if (!lookup_port(virtualizer_ptr, port1_id, &port1, &vgraph1))
  {
    return;
  }

  if (!lookup_port(virtualizer_ptr, port2_id, &port2, &vgraph2))
  {
    return;
  }

  if (vgraph1 != vgraph2)
  {
    /* TODO */
    log_error("ignoring connection with endpoints in different vgraphs");
    return;
  }

  if (ladish_graph_find_connection(virtualizer_ptr->jack_graph, port1, port2, &connection_id))
  {
    ladish_graph_remove_connection(virtualizer_ptr->jack_graph, connection_id, true);
  }
  else
  {
    log_error("ports %"PRIu64":%"PRIu64" and %"PRIu64":%"PRIu64" are not connected in the JACK graph", client1_id, port1_id, client2_id, port2_id);
  }

  if (ladish_graph_find_connection(vgraph1, port1, port2, &connection_id))
  {
    ladish_graph_remove_connection(vgraph1, connection_id, false);
  }
  else
  {
    log_error("ports %"PRIu64":%"PRIu64" and %"PRIu64":%"PRIu64" are not connected in the virtual graph", client1_id, port1_id, client2_id, port2_id);
  }
}

#undef virtualizer_ptr

bool
ladish_virtualizer_create(
  graph_proxy_handle jack_graph_proxy,
  ladish_graph_handle jack_graph,
  ladish_virtualizer_handle * handle_ptr)
{
  struct virtualizer * virtualizer_ptr;

  virtualizer_ptr = malloc(sizeof(struct virtualizer));
  if (virtualizer_ptr == NULL)
  {
    log_error("malloc() failed for struct virtualizer");
    return false;
  }

  virtualizer_ptr->jack_graph_proxy = jack_graph_proxy;
  virtualizer_ptr->jack_graph = jack_graph;
  virtualizer_ptr->system_client_id = 0;
  virtualizer_ptr->our_clients_count = 0;

  if (!graph_proxy_attach(
        jack_graph_proxy,
        virtualizer_ptr,
        clear,
        client_appeared,
        NULL,                   /* jackdbus does not have client rename functionality (yet) */
        client_disappeared,
        port_appeared,
        port_renamed,
        port_disappeared,
        ports_connected,
        ports_disconnected))
  {
    free(virtualizer_ptr);
    return false;
  }

  *handle_ptr = (ladish_virtualizer_handle)virtualizer_ptr;
  return true;
}

#define virtualizer_ptr ((struct virtualizer *)handle)

void
ladish_virtualizer_set_graph_connection_handlers(
  ladish_virtualizer_handle handle,
  ladish_graph_handle graph)
{
  ladish_graph_set_connection_handlers(graph, virtualizer_ptr, ports_connect_request, ports_disconnect_request);
}

unsigned int
ladish_virtualizer_get_our_clients_count(
  ladish_virtualizer_handle handle)
{
  return virtualizer_ptr->our_clients_count;
}

void
ladish_virtualizer_destroy(
  ladish_virtualizer_handle handle)
{
  log_info("ladish_virtualizer_destroy() called");

  graph_proxy_detach((graph_proxy_handle)handle, virtualizer_ptr);
  free(virtualizer_ptr);
}

#undef virtualizer_ptr
