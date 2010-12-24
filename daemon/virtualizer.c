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
#include "../common/catdup.h"
#include "room.h"
#include "studio.h"
#include "../alsapid/alsapid.h"

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
  ladish_graph_handle graph;
  ladish_app_handle app;
};

#define app_find_context_ptr ((struct app_find_context *)context)

static bool lookup_app_in_supervisor(void * context, ladish_graph_handle graph, ladish_app_supervisor_handle app_supervisor)
{
  pid_t pid;
  ladish_app_handle app;

  /* we stop iteration when app is found */
  ASSERT(app_find_context_ptr->app == NULL && app_find_context_ptr->graph == NULL);

  //log_info("checking app supervisor \"%s\" for pid %llu", ladish_app_supervisor_get_name(app_supervisor), (unsigned long long)pid);

  pid = app_find_context_ptr->pid;
  do
  {
    app = ladish_app_supervisor_find_app_by_pid(app_supervisor, pid);
    if (app != NULL)
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

  if (app == NULL)
  {                            /* app not found in current supervisor */
    return true;               /* continue app supervisor iteration */
  }

  app_find_context_ptr->app = app;
  app_find_context_ptr->graph = graph;

  return false;               /* stop app supervisor iteration */
}

#undef app_find_context_ptr

static ladish_app_handle ladish_virtualizer_find_app_by_pid(struct virtualizer * virtualizer_ptr, pid_t pid, ladish_graph_handle * graph_ptr)
{
  struct app_find_context context;

  context.pid = pid;
  context.app = NULL;
  context.graph = NULL;

  ladish_studio_iterate_virtual_graphs(&context, lookup_app_in_supervisor);

  if (context.app == NULL)
  { /* app not found */
    ASSERT(context.graph == NULL);
    return NULL;
  }

  ASSERT(context.graph != NULL);
  *graph_ptr = context.graph;

  return context.app;
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

  port = ladish_graph_find_port_by_uuid(graph, find_link_port_context_ptr->uuid, true, NULL);
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

  ladish_studio_iterate_virtual_graphs(&context, find_link_port_vgraph_callback_by_uuid);

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

  ladish_studio_iterate_virtual_graphs(&context, find_link_port_vgraph_callback_by_jack_id);

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
  ladish_graph_handle vgraph;

  port = ladish_graph_find_port_by_jack_id(virtualizer_ptr->jack_graph, port_id, true, true);
  if (port == NULL)
  {
    log_error("Unknown JACK port with id %"PRIu64" (dis)connected", port_id);
    return false;
  }

  vgraph = ladish_port_get_vgraph(port);
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
  ladish_app_handle app;
  uuid_t app_uuid;
  const char * name;
  pid_t pid;
  ladish_graph_handle graph;
  bool jmcore;

  log_info("client_appeared(%"PRIu64", %s)", id, jack_name);

  a2j_name = a2j_proxy_get_jack_client_name_cached();
  is_a2j = a2j_name != NULL && strcmp(a2j_name, jack_name) == 0;

  name = jack_name;
  app = NULL;
  graph = NULL;
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
        app = ladish_virtualizer_find_app_by_pid(virtualizer_ptr, pid, &graph);
        if (app != NULL)
        {
          ladish_app_get_uuid(app, app_uuid);
          ASSERT(!uuid_is_null(app_uuid));
          name = ladish_app_get_name(app);
          log_info("app name is '%s'", name);
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
      client = ladish_graph_find_client_by_app(virtualizer_ptr->jack_graph, app_uuid);
      if (client == NULL)
      {
        log_info("Lookup by app uuid failed, attempting lookup by name '%s'", name);
        client = ladish_graph_find_client_by_name(virtualizer_ptr->jack_graph, name, true);
      }
    }

    if (client != NULL)
    {
      log_info("found existing client");
      if (ladish_client_get_jack_id(client) != 0)
      {
        log_error("Ignoring client with duplicate name '%s' ('%s')", name, jack_name);
        goto exit;
      }

      ladish_client_set_jack_id(client, id);
      ladish_graph_show_client(virtualizer_ptr->jack_graph, client);
      goto done;
    }
  }

  if (!ladish_client_create(is_a2j ? g_a2j_uuid : NULL, &client))
  {
    log_error("ladish_client_create() failed. Ignoring client %"PRIu64" (%s)", id, jack_name);
    goto exit;
  }

  ladish_client_set_jack_id(client, id);

  if (!ladish_graph_add_client(virtualizer_ptr->jack_graph, client, name, false))
  {
    log_error("ladish_graph_add_client() failed to add client %"PRIu64" (%s) to JACK graph", id, name);
    ladish_client_destroy(client);
    goto exit;
  }

done:
  if (strcmp(jack_name, "system") == 0)
  {
    virtualizer_ptr->system_client_id = id;
  }

  if (app != NULL)
  {
    /* interlink client and app */
    ladish_app_add_pid(app, pid);
    ladish_client_set_pid(client, pid);
    ladish_client_set_app(client, app_uuid);

    ASSERT(graph);
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

exit:
  return;
}

static void client_disappeared(void * context, uint64_t id)
{
  ladish_client_handle client;
  pid_t pid;
  uuid_t app_uuid;
  ladish_app_handle app;
  ladish_graph_handle vgraph;

  log_info("client_disappeared(%"PRIu64")", id);

  client = ladish_graph_find_client_by_jack_id(virtualizer_ptr->jack_graph, id);
  if (client == NULL)
  {
    log_error("Unknown JACK client with id %"PRIu64" disappeared", id);
    return;
  }

  log_info("client disappeared: '%s'", ladish_graph_get_client_name(virtualizer_ptr->jack_graph, client));

  vgraph = ladish_client_get_vgraph(client);

  pid = ladish_client_get_pid(client);
  if (ladish_client_get_app(client, app_uuid))
  {
    virtualizer_ptr->our_clients_count--;
    app = ladish_studio_find_app_by_uuid(app_uuid);
    if (app != NULL)
    {
      ladish_app_del_pid(app, pid);
    }
    else
    {
      log_error("app of disappearing client %"PRIu64" not found. pid is %"PRIu64, id, (uint64_t)pid);
      ASSERT_NO_PASS;
    }
  }

  if (id == virtualizer_ptr->system_client_id)
  {
    virtualizer_ptr->system_client_id = 0;
  }

  if (vgraph != NULL && ladish_graph_is_persist(vgraph)) /* if client is supposed to be persisted */
  {
    ladish_client_set_jack_id(client, 0);
    ladish_graph_hide_client(virtualizer_ptr->jack_graph, client);
  }
  else
  {
    ladish_graph_remove_client(virtualizer_ptr->jack_graph, client);
    ladish_client_destroy(client);
    /* no need to clear vclient interlink because it either does not exist (vgraph is NULL) or
     * it will be destroyed before it is accessed (persist flag is cleared on room deletion) */
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
  const char * vclient_name;
  bool is_a2j;
  uuid_t vclient_uuid;
  pid_t pid;
  ladish_app_handle app;
  bool has_app;
  uuid_t app_uuid;
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

  has_app = ladish_client_get_app(jack_client, app_uuid);

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
  else
  {
    //log_info("Port of virtual graph '%s'", ladish_graph_get_description(vgraph));
  }

  jack_client_name = ladish_graph_get_client_name(virtualizer_ptr->jack_graph, jack_client);

  is_a2j = ladish_virtualizer_is_a2j_client(jack_client);
  if (is_a2j)
  {
    log_info("a2j port appeared");
    if (!a2j_proxy_map_jack_port(real_jack_port_name, &alsa_client_name, &alsa_port_name, &alsa_client_id))
    {
      is_a2j = false;
      alsa_client_name = catdup("FAILED ", jack_client_name);
      if (alsa_client_name == NULL)
      {
        log_error("catdup failed to duplicate a2j jack client name after map failure");
        goto exit;
      }

      alsa_port_name = strdup(real_jack_port_name);
      if (alsa_port_name == NULL)
      {
        log_error("catdup failed to duplicate a2j jack port name after map failure");
        free(alsa_client_name);
        goto exit;
      }

      vclient_name = alsa_client_name;
    }
    else
    {
      log_info("a2j: '%s':'%s' (%"PRIu32")", alsa_client_name, alsa_port_name, alsa_client_id);
      vclient_name = alsa_client_name;
      if (alsapid_get_pid(alsa_client_id, &pid))
      {
        log_info("ALSA client pid is %lld", (long long)pid);

        app = ladish_virtualizer_find_app_by_pid(virtualizer_ptr, pid, &vgraph);
        if (app != NULL)
        {
          ladish_app_get_uuid(app, app_uuid);
          ASSERT(!uuid_is_null(app_uuid));
          vclient_name = ladish_app_get_name(app);
          has_app = true;
          log_info("ALSA app name is '%s'", vclient_name);
        }
      }
      else
      {
        log_error("UNKNOWN ALSA client pid");
      }
    }

    a2j_fake_jack_port_name = catdup4(vclient_name, is_input ? " (playback)" : " (capture)", ": ", alsa_port_name);
    if (a2j_fake_jack_port_name == NULL)
    {
      log_error("catdup4() failed");
      goto free_alsa_names;
    }

    jack_port_name = a2j_fake_jack_port_name;
  }
  else
  {
    vclient_name = jack_client_name;
    jack_port_name = real_jack_port_name;
  }

  /********************/

  /* search (by name) the appeared port in jack graph
   * if found - show it in both graphs.
   * if not found - create new port and add it to the jack graph.
   * Then process to adding it to virtual graph */

  port = ladish_graph_find_port_by_name(virtualizer_ptr->jack_graph, jack_client, jack_port_name, vgraph);
  if (port != NULL)
  {
    log_info("found existing port %p", port);

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
      log_error("JACK port not found in virtual graph '%s'", ladish_graph_get_description(vgraph));
      ladish_graph_dump(g_studio.jack_graph);
      ladish_graph_dump(vgraph);
      ASSERT_NO_PASS;
      goto free_alsa_names;
    }

    /* for normal ports, one can find the app_uuid through the jack client,
       but for a2j ports the jack client is shared between graphs */
    if (has_app)
    {
      ladish_port_set_app(port, app_uuid);
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

  /* for normal ports, one can find the vgraph and app_uuid through the jack client,
     but for a2j ports the jack client is shared between graphs */
  ladish_port_set_vgraph(port, vgraph);
  if (has_app)
  {
    ladish_port_set_app(port, app_uuid);
  }

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
    vclient = ladish_graph_find_client_by_name(vgraph, vclient_name, false);
    if (vclient == NULL)
    {
      if (!ladish_client_create(NULL, &vclient))
      {
        log_error("ladish_client_create() failed.");
        goto free_alsa_names;
      }

      if (has_app)
      {
        ladish_client_set_app(vclient, app_uuid);
      }

      if (!ladish_graph_add_client(vgraph, vclient, vclient_name, false))
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

    if (ladish_client_get_interlink(jack_client, vclient_uuid))
    {
      vclient = ladish_graph_find_client_by_uuid(vgraph, vclient_uuid);
      ASSERT(vclient != NULL);
    }
    else
    {
      vclient = ladish_graph_find_client_by_app(vgraph, app_uuid);
      if (vclient == NULL)
      {
        log_info("creating new vclient");
        if (!ladish_client_create(NULL, &vclient))
        {
          log_error("ladish_client_create() failed.");
          goto free_alsa_names;
        }

        ladish_client_interlink(vclient, jack_client);

        if (has_app)
        {
          ladish_client_set_app(vclient, app_uuid);
        }

        if (!ladish_graph_add_client(vgraph, vclient, vclient_name, false))
        {
          log_error("ladish_graph_add_client() failed to add client '%s' to virtual graph", jack_client_name);
          ladish_client_destroy(vclient);
          goto free_alsa_names;
        }
      }
      else
      {
        /* vclient exists but is not interlinked with vclient */
        /* this can happen when client is created because of a2j port appear */
        ladish_client_interlink(vclient, jack_client);
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
  ladish_client_handle jclient;
  ladish_client_handle vclient;
  ladish_port_handle port;
  ladish_graph_handle vgraph;
  bool jmcore;

  log_info("port_disappeared(%"PRIu64", %"PRIu64")", client_id, port_id);

  jclient = ladish_graph_find_client_by_jack_id(virtualizer_ptr->jack_graph, client_id);
  if (jclient == NULL)
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
  vgraph = ladish_port_get_vgraph(port);
  if (vgraph == NULL)
  {
    vgraph = find_link_port_vgraph_by_uuid(virtualizer_ptr, ladish_graph_get_port_name(virtualizer_ptr->jack_graph, port), NULL);
    if (vgraph == NULL)
    {
      log_error("Cannot find vgraph for disappeared jmcore port");
      ASSERT_NO_PASS;
      return;
    }

    jmcore = true;
    ladish_graph_remove_port_by_jack_id(virtualizer_ptr->jack_graph, port_id, true, true);
  }

  if (ladish_graph_is_persist(vgraph)) /* if port is supposed to be persisted */
  {
    if (!jmcore)
    {
      ladish_port_set_jack_id(port, 0);
      ladish_graph_hide_port(virtualizer_ptr->jack_graph, port);
    }
    if (vgraph != NULL)
    {
      ladish_graph_hide_port(vgraph, port);
      vclient = ladish_graph_get_port_client(vgraph, port);
      if (ladish_graph_client_looks_empty(vgraph, vclient))
      {
        ladish_graph_hide_client(vgraph, vclient);
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
      vclient = ladish_graph_remove_port(vgraph, port);
      if (vclient != NULL)
      {
        if (ladish_graph_client_is_empty(vgraph, vclient))
        {
          ladish_graph_remove_client(vgraph, vclient);
          ladish_client_clear_interlink(jclient);
        }
      }
    }
  }
}

static void port_renamed(void * context, uint64_t client_id, uint64_t port_id, const char * old_port_name, const char * new_port_name)
{
  ladish_port_handle port;
  ladish_graph_handle vgraph;

  log_info("port_renamed(%"PRIu64", '%s', '%s')", port_id, old_port_name, new_port_name);

  port = ladish_graph_find_port_by_jack_id(virtualizer_ptr->jack_graph, port_id, true, true);
  if (port == NULL)
  {
    log_error("Unknown JACK port with id %"PRIu64" was renamed", port_id);
    return;
  }

  /* find the virtual graph that owns the app that owns the client that owns the renamed port */
  vgraph = ladish_port_get_vgraph(port);

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

  return graph_proxy_connect_ports(virtualizer_ptr->jack_graph_proxy, port1_id, port2_id);
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

  return graph_proxy_disconnect_ports(virtualizer_ptr->jack_graph_proxy, port1_id, port2_id);
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

static bool app_has_a2j_ports(ladish_graph_handle jack_graph, const uuid_t app_uuid)
{
  ladish_client_handle a2jclient;

  a2jclient = ladish_graph_find_client_by_uuid(jack_graph, g_a2j_uuid);
  if (a2jclient == NULL)
  {
    return false;
  }

  return ladish_graph_client_has_visible_app_port(jack_graph, a2jclient, app_uuid);
}

bool
ladish_virtualizer_is_hidden_app(
  ladish_graph_handle jack_graph,
  const uuid_t app_uuid,
  const char * app_name)
{
  ladish_client_handle jclient;
  ladish_graph_handle vgraph;
  uuid_t vclient_uuid;
  ladish_client_handle vclient;

  //ladish_graph_dump(g_studio.jack_graph);

  if (app_has_a2j_ports(jack_graph, app_uuid))
  {
    log_info("app '%s' still has a2j ports", app_name);
    return false;
  }

  jclient = ladish_graph_find_client_by_app(jack_graph, app_uuid);
  if (jclient == NULL)
  {
    log_info("App without JACK client is treated as hidden one");
    return true;
  }

  ASSERT(!ladish_virtualizer_is_a2j_client(jclient)); /* a2j client has no app associated */

  vgraph = ladish_client_get_vgraph(jclient);
  if (vgraph == NULL)
  {
    ASSERT_NO_PASS;
    return true;
  }

  //ladish_graph_dump(vgraph);

  if (!ladish_graph_client_looks_empty(jack_graph, jclient) ||
      !ladish_graph_client_is_hidden(jack_graph, jclient))
  {
    return false;
  }

  if (!ladish_client_get_interlink(jclient, vclient_uuid))
  {
    if (ladish_graph_client_is_empty(jack_graph, jclient))
    {
      log_info("jack client of app '%s' has no interlinked vgraph client and no ports", app_name);
    }
    else
    {
      log_error("jack client of app '%s' has no interlinked vgraph client", app_name);
      ASSERT_NO_PASS;
    }
    return true;
  }

  vclient = ladish_graph_find_client_by_uuid(vgraph, vclient_uuid);
  if (vclient == NULL)
  {
    ASSERT_NO_PASS;
    return true;
  }

  if (!ladish_graph_client_looks_empty(vgraph, vclient))
  {
    return false;
  }

  ASSERT(ladish_graph_client_is_hidden(vgraph, vclient)); /* vclients are automatically hidden when they start looking empty (on port disappear) */
  return true;
}

struct app_remove_context
{
  uuid_t app_uuid;
  const char * app_name;
};

#define app_info_ptr ((struct app_remove_context *)context)

static
bool
remove_app_port(
  void * context,
  ladish_graph_handle graph_handle,
  bool hidden,
  void * client_iteration_context_ptr,
  ladish_client_handle client_handle,
  const char * client_name,
  ladish_port_handle port_handle,
  const char * port_name,
  uint32_t port_type,
  uint32_t port_flags)
{
  ladish_graph_handle vgraph;
  ladish_client_handle vclient;

  if (!ladish_port_belongs_to_app(port_handle, app_info_ptr->app_uuid))
  {
    return true;
  }

  //log_info("removing port '%s':'%s' (JACK) of app '%s'", client_name, port_name, app_info_ptr->app_name);

  vgraph = ladish_port_get_vgraph(port_handle);
  if (vgraph == NULL)
  {
    log_error("port '%s':'%s' of app '5s' has no vgraph", client_name, port_name, app_info_ptr->app_name);
    ASSERT_NO_PASS;
    return true;
  }

  vclient = ladish_graph_get_port_client(vgraph, port_handle);
  if (vgraph == NULL)
  {
    log_error("app port '%s':'%s' not found in vgraph '%s'", client_name, port_name, ladish_graph_get_description(vgraph));
    ASSERT_NO_PASS;
    return true;
  }

  log_info(
    "removing %s %s port %p of app '%s' ('%s':'%s' in %s)",
    port_type == JACKDBUS_PORT_TYPE_AUDIO ? "audio" : "midi",
    JACKDBUS_PORT_IS_INPUT(port_flags) ? "input" : "output",
    port_handle,
    app_info_ptr->app_name,
    ladish_graph_get_client_name(vgraph, vclient),
    ladish_graph_get_port_name(vgraph, port_handle),
    ladish_graph_get_description(vgraph));

  ladish_graph_remove_port(graph_handle, port_handle);
  ladish_graph_remove_port(vgraph, port_handle);

  return true;
}

#undef app_info_ptr

void
ladish_virtualizer_remove_app(
  ladish_graph_handle jack_graph,
  const uuid_t app_uuid,
  const char * app_name)
{
  ladish_client_handle jclient;
  ladish_graph_handle vgraph;
  uuid_t vclient_uuid;
  ladish_client_handle vclient;
  bool is_empty;
  struct app_remove_context ctx;

  //ladish_graph_dump(g_studio.jack_graph);

  uuid_copy(ctx.app_uuid, app_uuid);
  ctx.app_name = app_name;

  ladish_graph_iterate_nodes(jack_graph, &ctx, NULL, remove_app_port, NULL);

  jclient = ladish_graph_find_client_by_app(jack_graph, app_uuid);
  if (jclient == NULL)
  {
    log_info("removing app without JACK client");
    return;
  }

  ASSERT(!ladish_virtualizer_is_a2j_client(jclient)); /* a2j client has no app associated */

  vgraph = ladish_client_get_vgraph(jclient);
  if (vgraph == NULL)
  {
    ASSERT_NO_PASS;
    return;
  }

  //ladish_graph_dump(vgraph);

  /* check whether the client is empty because this cannot
     be checked later because the client was removed
     (see where is_empty is used) */
  is_empty = ladish_graph_client_is_empty(jack_graph, jclient);

  ladish_graph_remove_client(jack_graph, jclient);

  if (!ladish_client_get_interlink(jclient, vclient_uuid))
  {
    if (is_empty)
    {
      /* jack client without ports and thus without vgraph client */
      return;
    }

    log_error("jack client of app '%s' has no interlinked vgraph client", app_name);
    ladish_graph_dump(g_studio.jack_graph);
    ladish_graph_dump(vgraph);
    ASSERT_NO_PASS;
    return;
  }

  vclient = ladish_graph_find_client_by_uuid(vgraph, vclient_uuid);
  if (vclient == NULL)
  {
    ASSERT_NO_PASS;
    return;
  }

  ladish_graph_remove_client(vgraph, vclient);
  ladish_graph_dump(g_studio.jack_graph);
  ladish_graph_dump(vgraph);
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

#define vgraph ((ladish_graph_handle)vgraph_context)
void
ladish_virtualizer_rename_app(
  void * vgraph_context,
  const uuid_t uuid,
  const char * old_name,
  const char * new_app_name)
{
  ladish_client_handle client;

  client = ladish_graph_find_client_by_app(vgraph, uuid);
  if (client != NULL)
  {
    ladish_graph_rename_client(vgraph, client, new_app_name);
  }

  client = ladish_graph_find_client_by_app(g_studio.jack_graph, uuid);
  if (client != NULL)
  {
    ladish_graph_rename_client(g_studio.jack_graph, client, new_app_name);
  }
}
#undef vgraph

bool
ladish_virtualizer_is_system_client(
  uuid_t uuid)
{
  if (uuid_compare(uuid, g_system_capture_uuid) == 0)
  {
    return true;
  }

  if (uuid_compare(uuid, g_system_playback_uuid) == 0)
  {
    return true;
  }

  return false;
}

bool ladish_virtualizer_is_a2j_client(ladish_client_handle jclient)
{
  uuid_t jclient_uuid;

  ladish_client_get_uuid(jclient, jclient_uuid);
  return uuid_compare(jclient_uuid, g_a2j_uuid) == 0;
}
