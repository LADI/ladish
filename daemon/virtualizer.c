/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
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
#include "a2j_proxy.h"
#include "procfs.h"
#include "app_supervisor.h"
#include "studio_internal.h"

struct virtualizer
{

  graph_proxy_handle jack_graph_proxy;
  ladish_graph_handle jack_graph;
  ladish_graph_handle studio_graph;
  uint64_t system_client_id;
  unsigned int our_clients_count;
};

/* 47c1cd18-7b21-4389-bec4-6e0658e1d6b1 */
UUID_DEFINE(g_system_capture_uuid,0x47,0xC1,0xCD,0x18,0x7B,0x21,0x43,0x89,0xBE,0xC4,0x6E,0x06,0x58,0xE1,0xD6,0xB1);

/* b2a0bb06-28d8-4bfe-956e-eb24378f9629 */
UUID_DEFINE(g_system_playback_uuid,0xB2,0xA0,0xBB,0x06,0x28,0xD8,0x4B,0xFE,0x95,0x6E,0xEB,0x24,0x37,0x8F,0x96,0x29);

/* be23a242-e2b2-11de-b795-002618af5e42 */
UUID_DEFINE(g_a2j_uuid,0xBE,0x23,0xA2,0x42,0xE2,0xB2,0x11,0xDE,0xB7,0x95,0x00,0x26,0x18,0xAF,0x5E,0x42);

char * get_app_name(struct virtualizer * virtualizer_ptr, uint64_t client_id, pid_t * app_pid_ptr)
{
  int64_t pid;
  unsigned long long ppid;
  char * app_name;

  if (!graph_proxy_get_client_pid(virtualizer_ptr->jack_graph_proxy, client_id, &pid))
  {
    pid = 0;
  }

  log_info("client pid is %"PRId64, pid);
  app_name = NULL;
  if (pid != 0)
  {
    ppid = (unsigned long long)pid;
  loop:
    app_name = ladish_app_supervisor_search_app(g_studio.app_supervisor, ppid);
    if (app_name == NULL)
    {
      ppid = procfs_get_process_parent(ppid);
      if (ppid != 0)
      {
        //log_info("parent pid %llu", ppid);
        goto loop;
      }
    }
  }

  *app_pid_ptr = (pid_t)ppid;

  return app_name;
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

  log_info("client_appeared(%"PRIu64", %s)", id, jack_name);

  a2j_name = a2j_proxy_get_jack_client_name_cached();
  is_a2j = a2j_name != NULL && strcmp(a2j_name, name) == 0;

  app_name = get_app_name(virtualizer_ptr, id, &pid);
  if (app_name != NULL)
  {
    log_info("app name is '%s'", app_name);
    name = app_name;
  }
  else
  {
    name = jack_name;
  }

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
    ASSERT(ladish_client_get_jack_id(client) == 0); /* two JACK clients with same name? */
    ladish_client_set_jack_id(client, id);
    ladish_graph_show_client(virtualizer_ptr->jack_graph, client);
    goto exit;
  }

  if (!ladish_client_create(is_a2j ? g_a2j_uuid : NULL, true, false, false, &client))
  {
    log_error("ladish_client_create() failed. Ignoring client %"PRIu64" (%s)", id, jack_name);
    return;
  }

  ladish_client_set_jack_id(client, id);

  if (!ladish_graph_add_client(virtualizer_ptr->jack_graph, client, name, false))
  {
    log_error("ladish_graph_add_client() failed to add client %"PRIu64" (%s) to JACK graph", id, name);
    ladish_client_destroy(client);
    return;
  }

exit:
  if (strcmp(jack_name, "system") == 0)
  {
    virtualizer_ptr->system_client_id = id;
  }

  if (app_name != NULL)
  {
    ladish_client_set_pid(client, pid);
    virtualizer_ptr->our_clients_count++;
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
    ladish_graph_remove_client(virtualizer_ptr->jack_graph, client, false);
    ladish_client_destroy(client);
  }
}

static void port_appeared(void * context, uint64_t client_id, uint64_t port_id, const char * port_name, bool is_input, bool is_terminal, bool is_midi)
{
  ladish_client_handle jack_client;
  ladish_client_handle studio_client;
  ladish_port_handle port;
  uint32_t type;
  uint32_t flags;
  const char * jack_client_name;
  bool is_a2j;
  uuid_t client_uuid;
  char * alsa_client_name;
  char * alsa_port_name;
  uint32_t alsa_client_id;

  log_info("port_appeared(%"PRIu64", %"PRIu64", %s (%s, %s))", client_id, port_id, port_name, is_input ? "in" : "out", is_midi ? "midi" : "audio");

  /********************/
  /* gather info about the appeared port */

  jack_client = ladish_graph_find_client_by_jack_id(virtualizer_ptr->jack_graph, client_id);
  if (jack_client == NULL)
  {
    log_error("Port of unknown JACK client with id %"PRIu64" appeared", client_id);
    goto fail;
  }

  ladish_client_get_uuid(jack_client, client_uuid);
  is_a2j = uuid_compare(client_uuid, g_a2j_uuid) == 0;
  if (is_a2j)
  {
    log_info("a2j port appeared");
    if (!a2j_proxy_map_jack_port(port_name, &alsa_client_name, &alsa_port_name, &alsa_client_id))
    {
      is_a2j = false;
    }
    else
    {
      log_info("a2j: '%s':'%s' (%"PRIu32")", alsa_client_name, alsa_port_name, alsa_client_id);
    }
  }

  jack_client_name = ladish_graph_get_client_name(virtualizer_ptr->jack_graph, jack_client);

  type = is_midi ? JACKDBUS_PORT_TYPE_MIDI : JACKDBUS_PORT_TYPE_AUDIO;
  flags = is_input ? JACKDBUS_PORT_FLAG_INPUT : JACKDBUS_PORT_FLAG_OUTPUT;
  if (is_terminal)
  {
    flags |= JACKDBUS_PORT_FLAG_TERMINAL;
  }

  /********************/

  if (!is_a2j) /* a2j jack port names are not persistent because they contain ALSA client ID */
  {
    /* search (by name) the appeared port in jack graph
     * if found - show it in both graphs.
     * if not found - create new port and add it to the jack graph.
     * The process to adding it to studio graph */

    port = ladish_graph_find_port_by_name(virtualizer_ptr->jack_graph, jack_client, port_name);
    if (port != NULL)
    {
      log_info("found existing port");

      ASSERT(ladish_port_get_jack_id(port) == 0); /* two JACK ports with same name? */
      ladish_port_set_jack_id(port, port_id);
      ladish_graph_adjust_port(virtualizer_ptr->jack_graph, port, type, flags);
      ladish_graph_show_port(virtualizer_ptr->jack_graph, port);

      studio_client = ladish_graph_get_port_client(virtualizer_ptr->studio_graph, port);
      if (studio_client == NULL)
      {
        log_error("JACK port not found in studio graph");
        ASSERT_NO_PASS;
        goto free_alsa_names;
      }

      ladish_client_set_jack_id(studio_client, client_id);
      ladish_graph_adjust_port(virtualizer_ptr->studio_graph, port, type, flags);
      ladish_graph_show_port(virtualizer_ptr->studio_graph, port);
      goto free_alsa_names;
    }

    if (!ladish_port_create(NULL, &port))
    {
      log_error("ladish_port_create() failed.");
      goto free_alsa_names;
    }

    /* set port jack id so invisible connections to/from it can be restored */
    ladish_port_set_jack_id(port, port_id);

    if (!ladish_graph_add_port(virtualizer_ptr->jack_graph, jack_client, port, port_name, type, flags, false))
    {
      log_error("ladish_graph_add_port() failed.");
      ladish_port_destroy(port);
      goto free_alsa_names;
    }
  }
  else
  {
    /* a2j ports are present and referenced from the jack graph their names in the JACK graph are invalid
     * so they will be searched by uuid when studio graph is inspected later. */
    port = NULL;
  }

  /********************/
  /* find/create the studio client where port will be added eventually */

  if (is_a2j)
  {
    /* search (by name) the appeared a2j port and its client in the studio graph.
     * if client is not found - create both client and port.
     * if client is found - show it if this is the first port visible port.
     *   if port is found - show it
     *   if port is not found - create new port and add it to the studio graph */
    studio_client = ladish_graph_find_client_by_name(virtualizer_ptr->studio_graph, alsa_client_name);
    if (studio_client == NULL)
    {
        if (!ladish_client_create(g_a2j_uuid, true, false, true, &studio_client))
        {
          log_error("ladish_client_create() failed.");
          goto free_alsa_names;
        }

        if (!ladish_graph_add_client(virtualizer_ptr->studio_graph, studio_client, alsa_client_name, false))
        {
          log_error("ladish_graph_add_client() failed.");
          ladish_client_destroy(studio_client);
          goto free_alsa_names;
        }
    }
    else
    {
      port = ladish_graph_find_port_by_name(virtualizer_ptr->studio_graph, studio_client, alsa_port_name);
      if (port != NULL)
      { /* found existing port - show it */
        ASSERT(ladish_port_get_jack_id(port) == 0); /* two a2j ports with same name? */
        ladish_port_set_jack_id(port, port_id); /* set port jack id so invisible connections to/from it can be restored */

        ladish_graph_adjust_port(virtualizer_ptr->jack_graph, port, type, flags);
        ladish_graph_show_port(virtualizer_ptr->jack_graph, port);

        ladish_graph_adjust_port(virtualizer_ptr->studio_graph, port, type, flags);
        ladish_graph_show_port(virtualizer_ptr->studio_graph, port);

        goto free_alsa_names;
      }
    }

    /* port not found - create it */
    if (!ladish_port_create(NULL, &port))
    {
      log_error("ladish_port_create() failed.");
      goto free_alsa_names;
    }

    /* set port jack id so invisible connections to/from it can be restored */
    ladish_port_set_jack_id(port, port_id);

    port_name = alsa_port_name;
  }
  else if (client_id == virtualizer_ptr->system_client_id)
  {
    log_info("system client port appeared");

    if (!is_input)
    { /* output capture port */

      studio_client = ladish_graph_find_client_by_uuid(virtualizer_ptr->studio_graph, g_system_capture_uuid);
      if (studio_client == NULL)
      {
        if (!ladish_client_create(g_system_capture_uuid, true, false, true, &studio_client))
        {
          log_error("ladish_client_create() failed.");
          goto free_alsa_names;
        }

        if (!ladish_graph_add_client(virtualizer_ptr->studio_graph, studio_client, "Hardware Capture", false))
        {
          log_error("ladish_graph_add_client() failed.");
          ladish_client_destroy(studio_client);
          goto free_alsa_names;
        }
      }
    }
    else
    { /* input playback port */
      studio_client = ladish_graph_find_client_by_uuid(virtualizer_ptr->studio_graph, g_system_playback_uuid);
      if (studio_client == NULL)
      {
        if (!ladish_client_create(g_system_playback_uuid, true, false, true, &studio_client))
        {
          log_error("ladish_client_create() failed.");
          goto free_alsa_names;
        }

        if (!ladish_graph_add_client(virtualizer_ptr->studio_graph, studio_client, "Hardware Playback", false))
        {
          ladish_client_destroy(studio_client);
          goto free_alsa_names;
        }
      }
    }
  }
  else
  { /* non-system client */
    log_info("non-system client port appeared");

    studio_client = ladish_graph_find_client_by_jack_id(virtualizer_ptr->studio_graph, client_id);
    if (studio_client == NULL)
    {
      if (!ladish_client_create(NULL, false, false, false, &studio_client))
      {
        log_error("ladish_client_create() failed.");
        goto free_alsa_names;
      }

      ladish_client_set_jack_id(studio_client, client_id);

      if (!ladish_graph_add_client(virtualizer_ptr->studio_graph, studio_client, jack_client_name, false))
      {
        log_error("ladish_graph_add_client() failed to add client '%s' to studio graph", jack_client_name);
        ladish_client_destroy(studio_client);
        goto free_alsa_names;
      }
    }
  }

  /* add newly appeared port to the studio graph */

  if (!ladish_graph_add_port(virtualizer_ptr->studio_graph, studio_client, port, port_name, type, flags, false))
  {
    log_error("ladish_graph_add_port() failed.");
    goto free_alsa_names;
  }

free_alsa_names:
  if (is_a2j)
  {
    free(alsa_client_name);
    free(alsa_port_name);
  }

fail:
  return;
}

static void port_disappeared(void * context, uint64_t client_id, uint64_t port_id)
{
  ladish_client_handle client;
  ladish_port_handle port;

  log_info("port_disappeared(%"PRIu64")", port_id);

  client = ladish_graph_find_client_by_jack_id(virtualizer_ptr->jack_graph, client_id);
  if (client == NULL)
  {
    log_error("Port of unknown JACK client with id %"PRIu64" disappeared", client_id);
    return;
  }

  port = ladish_graph_find_port_by_jack_id(virtualizer_ptr->jack_graph, port_id);
  if (port == NULL)
  {
    log_error("Unknown JACK port with id %"PRIu64" disappeared", port_id);
    return;
  }

  if (true)                     /* if client is supposed to be persisted */
  {
    ladish_port_set_jack_id(port, 0);
    ladish_graph_hide_port(virtualizer_ptr->jack_graph, port);
    ladish_graph_hide_port(virtualizer_ptr->studio_graph, port);
    client = ladish_graph_get_port_client(virtualizer_ptr->studio_graph, port);
    if (ladish_graph_is_client_looks_empty(virtualizer_ptr->studio_graph, client))
    {
        ladish_graph_hide_client(virtualizer_ptr->studio_graph, client);
    }
  }
  else
  {
    ladish_graph_remove_port(virtualizer_ptr->jack_graph, port);

    client = ladish_graph_remove_port(virtualizer_ptr->studio_graph, port);
    if (client != NULL)
    {
      if (ladish_graph_is_client_empty(virtualizer_ptr->studio_graph, client))
      {
        ladish_graph_remove_client(virtualizer_ptr->studio_graph, client, false);
      }
    }
  }
}

static bool ports_connect_request(void * context, ladish_graph_handle graph_handle, ladish_port_handle port1, ladish_port_handle port2)
{
  uint64_t port1_id;
  uint64_t port2_id;

  ASSERT(graph_handle == virtualizer_ptr->studio_graph);
  log_info("virtualizer: ports connect request");

  port1_id = ladish_port_get_jack_id(port1);
  port2_id = ladish_port_get_jack_id(port2);

  graph_proxy_connect_ports(virtualizer_ptr->jack_graph_proxy, port1_id, port2_id);

  return true;
}

static bool ports_disconnect_request(void * context, ladish_graph_handle graph_handle, uint64_t connection_id)
{
  ladish_port_handle port1;
  ladish_port_handle port2;
  uint64_t port1_id;
  uint64_t port2_id;

  log_info("virtualizer: ports disconnect request");

  ASSERT(graph_handle == virtualizer_ptr->studio_graph);

  if (!ladish_graph_get_connection_ports(virtualizer_ptr->studio_graph, connection_id, &port1, &port2))
  {
    log_error("cannot find ports that are disconnect-requested");
    ASSERT_NO_PASS;
    return false;
  }

  port1_id = ladish_port_get_jack_id(port1);
  port2_id = ladish_port_get_jack_id(port2);

  graph_proxy_disconnect_ports(virtualizer_ptr->jack_graph_proxy, port1_id, port2_id);

  return true;
}

static void ports_connected(void * context, uint64_t client1_id, uint64_t port1_id, uint64_t client2_id, uint64_t port2_id)
{
  ladish_port_handle port1;
  ladish_port_handle port2;
  uint64_t connection_id;

  log_info("ports_connected %"PRIu64":%"PRIu64" %"PRIu64":%"PRIu64"", client1_id, port1_id, client2_id, port2_id);

  port1 = ladish_graph_find_port_by_jack_id(virtualizer_ptr->jack_graph, port1_id);
  if (port1 == NULL)
  {
    log_error("Unknown JACK port with id %"PRIu64" connected", port1_id);
    return;
  }

  port2 = ladish_graph_find_port_by_jack_id(virtualizer_ptr->jack_graph, port2_id);
  if (port2 == NULL)
  {
    log_error("Unknown JACK port with id %"PRIu64" connected", port2_id);
    return;
  }

  ladish_graph_add_connection(virtualizer_ptr->jack_graph, port1, port2, false);

  if (ladish_graph_find_connection(virtualizer_ptr->studio_graph, port1, port2, &connection_id))
  {
    log_info("showing hidden connection");
    ladish_graph_show_connection(virtualizer_ptr->studio_graph, connection_id);
  }
  else
  {
    log_info("creating new connection");
    ladish_graph_add_connection(virtualizer_ptr->studio_graph, port1, port2, false);
  }
}

static void ports_disconnected(void * context, uint64_t client1_id, uint64_t port1_id, uint64_t client2_id, uint64_t port2_id)
{
  ladish_port_handle port1;
  ladish_port_handle port2;
  uint64_t connection_id;

  log_info("ports_disconnected %"PRIu64":%"PRIu64" %"PRIu64":%"PRIu64"", client1_id, port1_id, client2_id, port2_id);

  port1 = ladish_graph_find_port_by_jack_id(virtualizer_ptr->jack_graph, port1_id);
  if (port1 == NULL)
  {
    log_error("Unknown JACK port with id %"PRIu64" disconnected", port1_id);
    return;
  }

  port2 = ladish_graph_find_port_by_jack_id(virtualizer_ptr->jack_graph, port2_id);
  if (port2 == NULL)
  {
    log_error("Unknown JACK port with id %"PRIu64" disconnected", port2_id);
    return;
  }

  if (ladish_graph_find_connection(virtualizer_ptr->jack_graph, port1, port2, &connection_id))
  {
    ladish_graph_remove_connection(virtualizer_ptr->jack_graph, connection_id);
  }
  else
  {
    log_error("ports %"PRIu64":%"PRIu64" and %"PRIu64":%"PRIu64" are not connected in the JACK graph", client1_id, port1_id, client2_id, port2_id);
  }

  if (ladish_graph_find_connection(virtualizer_ptr->studio_graph, port1, port2, &connection_id))
  {
    ladish_graph_remove_connection(virtualizer_ptr->studio_graph, connection_id);
  }
  else
  {
    log_error("ports %"PRIu64":%"PRIu64" and %"PRIu64":%"PRIu64" are not connected in the Studio graph", client1_id, port1_id, client2_id, port2_id);
  }
}

#undef virtualizer_ptr

bool
ladish_virtualizer_create(
  graph_proxy_handle jack_graph_proxy,
  ladish_graph_handle jack_graph,
  ladish_graph_handle studio_graph,
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
  virtualizer_ptr->studio_graph = studio_graph;
  virtualizer_ptr->system_client_id = 0;
  virtualizer_ptr->our_clients_count = 0;

  if (!graph_proxy_attach(
        jack_graph_proxy,
        virtualizer_ptr,
        clear,
        client_appeared,
        client_disappeared,
        port_appeared,
        port_disappeared,
        ports_connected,
        ports_disconnected))
  {
    free(virtualizer_ptr);
    return false;
  }

  ladish_graph_set_connection_handlers(studio_graph, virtualizer_ptr, ports_connect_request, ports_disconnect_request);

  *handle_ptr = (ladish_virtualizer_handle)virtualizer_ptr;
  return true;
}

#define virtualizer_ptr ((struct virtualizer *)handle)

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
