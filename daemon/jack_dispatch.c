/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the graph dispatcher object
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

#include "jack_dispatch.h"
#include "../dbus_constants.h"

struct jack_dispatcher
{

  graph_proxy_handle jack_graph_proxy;
  ladish_graph_handle jack_graph;
  ladish_graph_handle studio_graph;
  uint64_t system_client_id;
  ladish_client_handle system_capture_client;
  ladish_client_handle system_playback_client;
};

/* 47c1cd18-7b21-4389-bec4-6e0658e1d6b1 */
UUID_DEFINE(g_system_capture_uuid,0x47,0xC1,0xCD,0x18,0x7B,0x21,0x43,0x89,0xBE,0xC4,0x6E,0x06,0x58,0xE1,0xD6,0xB1);

/* b2a0bb06-28d8-4bfe-956e-eb24378f9629 */
UUID_DEFINE(g_system_playback_uuid,0xB2,0xA0,0xBB,0x06,0x28,0xD8,0x4B,0xFE,0x95,0x6E,0xEB,0x24,0x37,0x8F,0x96,0x29);

#define dispatcher_ptr ((struct jack_dispatcher *)context)

static void clear(void * context)
{
  log_info("clear");
}

static void client_appeared(void * context, uint64_t id, const char * name)
{
  ladish_client_handle client;

  log_info("client_appeared(%"PRIu64", %s)", id, name);

  client = ladish_graph_find_client_by_name(dispatcher_ptr->jack_graph, name);
  if (client != NULL)
  {
    log_info("found existing client");
    ASSERT(ladish_client_get_jack_id(client) == 0); /* two JACK clients with same name? */
    ladish_client_set_jack_id(client, id);
    goto exit;
  }

  if (!ladish_client_create(NULL, true, false, false, &client))
  {
    log_error("ladish_client_create() failed. Ignoring client %"PRIu64" (%s)", id, name);
    return;
  }

  ladish_client_set_jack_id(client, id);

  if (!ladish_graph_add_client(dispatcher_ptr->jack_graph, client, name, false))
  {
    log_error("ladish_graph_add_client() failed to add client %"PRIu64" (%s) to JACK graph", id, name);
    ladish_client_destroy(client);
    return;
  }

exit:
  if (strcmp(name, "system") == 0)
  {
    dispatcher_ptr->system_client_id = id;
  }
}

static void client_disappeared(void * context, uint64_t id)
{
  ladish_client_handle client;

  log_info("client_disappeared(%"PRIu64")", id);

  client = ladish_graph_find_client_by_jack_id(dispatcher_ptr->jack_graph, id);
  if (client == NULL)
  {
    log_error("Unknown JACK client with id %"PRIu64" disappeared", id);
    return;
  }

  if (id == dispatcher_ptr->system_client_id)
  {
    dispatcher_ptr->system_client_id = 0;
  }

  if (true)                     /* if client is supposed to be persisted */
  {
    ladish_client_set_jack_id(client, 0);
    ladish_graph_hide_client(dispatcher_ptr->jack_graph, client);
  }
  else
  {
    ladish_graph_remove_client(dispatcher_ptr->jack_graph, client, false);
    ladish_client_destroy(client);
  }
}

static void port_appeared(void * context, uint64_t client_id, uint64_t port_id, const char * port_name, bool is_input, bool is_terminal, bool is_midi)
{
  ladish_client_handle client;
  ladish_port_handle port;
  uint32_t type;
  uint32_t flags;
  const char * jack_client_name;

  log_info("port_appeared(%"PRIu64", %"PRIu64", %s (%s, %s))", client_id, port_id, port_name, is_input ? "in" : "out", is_midi ? "midi" : "audio");

  client = ladish_graph_find_client_by_jack_id(dispatcher_ptr->jack_graph, client_id);
  if (client == NULL)
  {
    log_error("Port of unknown JACK client with id %"PRIu64" appeared", client_id);
    return;
  }

  jack_client_name = ladish_graph_get_client_name(dispatcher_ptr->jack_graph, client);

  type = is_midi ? JACKDBUS_PORT_TYPE_MIDI : JACKDBUS_PORT_TYPE_AUDIO;
  flags = is_input ? JACKDBUS_PORT_FLAG_INPUT : JACKDBUS_PORT_FLAG_OUTPUT;
  if (is_terminal)
  {
    flags |= JACKDBUS_PORT_FLAG_TERMINAL;
  }

  port = ladish_graph_find_port_by_name(dispatcher_ptr->jack_graph, client, port_name);
  if (port != NULL)
  {
    log_info("found existing port");

    ASSERT(ladish_port_get_jack_id(port) == 0); /* two JACK ports with same name? */
    ladish_port_set_jack_id(port, port_id);
    ladish_graph_adjust_port(dispatcher_ptr->jack_graph, port, type, flags);
    ladish_graph_show_port(dispatcher_ptr->jack_graph, port);

    if (!ladish_graph_is_port_present(dispatcher_ptr->studio_graph, port))
    {
      log_error("JACK port not found in studio graph");
      ASSERT_NO_PASS;
      return;
    }
    else
    {
      ladish_graph_adjust_port(dispatcher_ptr->studio_graph, port, type, flags);
      ladish_graph_show_port(dispatcher_ptr->studio_graph, port);
      return;
    }
  }

  if (!ladish_port_create(NULL, &port))
  {
    log_error("ladish_port_create() failed.");
    return;
  }

  ladish_port_set_jack_id(port, port_id);

  if (!ladish_graph_add_port(dispatcher_ptr->jack_graph, client, port, port_name, type, flags, false))
  {
    log_error("ladish_graph_add_port() failed.");
    ladish_port_destroy(port);
    return;
  }

  if (client_id == dispatcher_ptr->system_client_id)
  {
    if (!is_input)
    { /* output capture port */
      if (dispatcher_ptr->system_capture_client == NULL)
      {
        if (!ladish_client_create(g_system_capture_uuid, true, false, true, &dispatcher_ptr->system_capture_client))
        {
          log_error("ladish_client_create() failed.");
          return;
        }

        if (!ladish_graph_add_client(dispatcher_ptr->studio_graph, dispatcher_ptr->system_capture_client, "Hardware Capture", false))
        {
          log_error("ladish_graph_add_client() failed.");
          ladish_graph_remove_client(dispatcher_ptr->studio_graph, dispatcher_ptr->system_capture_client, true);
          dispatcher_ptr->system_capture_client = NULL;
          return;
        }
      }

      client = dispatcher_ptr->system_capture_client;
    }
    else
    { /* input capture port */
      if (dispatcher_ptr->system_playback_client == NULL)
      {
        if (!ladish_client_create(g_system_playback_uuid, true, false, true, &dispatcher_ptr->system_playback_client))
        {
          log_error("ladish_client_create() failed.");
          return;
        }

        if (!ladish_graph_add_client(dispatcher_ptr->studio_graph, dispatcher_ptr->system_playback_client, "Hardware Playback", false))
        {
          ladish_graph_remove_client(dispatcher_ptr->studio_graph, dispatcher_ptr->system_playback_client, true);
          dispatcher_ptr->system_playback_client = NULL;
          return;
        }
      }

      client = dispatcher_ptr->system_playback_client;
    }
  }
  else
  { /* non-system client */
    client = ladish_graph_find_client_by_jack_id(dispatcher_ptr->studio_graph, client_id);
    if (client == NULL)
    {
      if (!ladish_client_create(NULL, false, false, false, &client))
      {
        log_error("ladish_client_create() failed.");
        return;
      }

      ladish_client_set_jack_id(client, client_id);

      if (!ladish_graph_add_client(dispatcher_ptr->studio_graph, client, jack_client_name, false))
      {
        log_error("ladish_graph_add_client() failed to add client '%s' to studio graph", jack_client_name);
        ladish_client_destroy(client);
        return;
      }
    }
  }

  if (!ladish_graph_add_port(dispatcher_ptr->studio_graph, client, port, port_name, type, flags, false))
  {
    log_error("ladish_graph_add_port() failed.");
    return;
  }
}

static void port_disappeared(void * context, uint64_t client_id, uint64_t port_id)
{
  ladish_client_handle client;
  ladish_port_handle port;

  log_info("port_disappeared(%"PRIu64")", port_id);

  client = ladish_graph_find_client_by_jack_id(dispatcher_ptr->jack_graph, client_id);
  if (client == NULL)
  {
    log_error("Port of unknown JACK client with id %"PRIu64" disappeared", client_id);
    return;
  }

  port = ladish_graph_find_port_by_jack_id(dispatcher_ptr->jack_graph, port_id);
  if (client == NULL)
  {
    log_error("Unknown JACK port with id %"PRIu64" disappeared", port_id);
    return;
  }

  if (true)                     /* if client is supposed to be persisted */
  {
    ladish_port_set_jack_id(port, 0);
    ladish_graph_hide_port(dispatcher_ptr->jack_graph, port);
    ladish_graph_hide_port(dispatcher_ptr->studio_graph, port);
    client = ladish_graph_get_port_client(dispatcher_ptr->studio_graph, port);
    if (ladish_graph_is_client_looks_empty(dispatcher_ptr->studio_graph, client))
    {
        ladish_graph_hide_client(dispatcher_ptr->studio_graph, client);
    }
  }
  else
  {
    ladish_graph_remove_port(dispatcher_ptr->jack_graph, port);

    client = ladish_graph_remove_port(dispatcher_ptr->studio_graph, port);
    if (client != NULL)
    {
      if (ladish_graph_is_client_empty(dispatcher_ptr->studio_graph, client))
      {
        ladish_graph_remove_client(dispatcher_ptr->studio_graph, client, false);
        if (client == dispatcher_ptr->system_capture_client)
        {
          dispatcher_ptr->system_capture_client = NULL;
        }
        if (client == dispatcher_ptr->system_playback_client)
        {
          dispatcher_ptr->system_playback_client = NULL;
        }
      }
    }
  }
}

static void ports_connected(void * context, uint64_t client1_id, uint64_t port1_id, uint64_t client2_id, uint64_t port2_id)
{
  log_info("ports_connected");
}

static void ports_disconnected(void * context, uint64_t client1_id, uint64_t port1_id, uint64_t client2_id, uint64_t port2_id)
{
  log_info("ports_disconnected");
}

#undef dispatcher_ptr

bool
ladish_jack_dispatcher_create(
  graph_proxy_handle jack_graph_proxy,
  ladish_graph_handle jack_graph,
  ladish_graph_handle studio_graph,
  ladish_jack_dispatcher_handle * handle_ptr)
{
  struct jack_dispatcher * dispatcher_ptr;

  dispatcher_ptr = malloc(sizeof(struct jack_dispatcher));
  if (dispatcher_ptr == NULL)
  {
    log_error("malloc() failed for struct jack_dispatcher");
    return false;
  }

  dispatcher_ptr->jack_graph_proxy = jack_graph_proxy;
  dispatcher_ptr->jack_graph = jack_graph;
  dispatcher_ptr->studio_graph = studio_graph;
  dispatcher_ptr->system_client_id = 0;
  dispatcher_ptr->system_capture_client = NULL;
  dispatcher_ptr->system_playback_client = NULL;

  if (!graph_proxy_attach(
        jack_graph_proxy,
        dispatcher_ptr,
        clear,
        client_appeared,
        client_disappeared,
        port_appeared,
        port_disappeared,
        ports_connected,
        ports_disconnected))
  {
    free(dispatcher_ptr);
    return false;
  }

  *handle_ptr = (ladish_jack_dispatcher_handle)dispatcher_ptr;
  return true;
}

#define dispatcher_ptr ((struct jack_dispatcher *)handle)

void
ladish_jack_dispatcher_destroy(
  ladish_jack_dispatcher_handle handle)
{
  graph_proxy_detach((graph_proxy_handle)handle, dispatcher_ptr);

  if (dispatcher_ptr->system_capture_client != NULL)
  {
    ladish_graph_remove_client(dispatcher_ptr->studio_graph, dispatcher_ptr->system_capture_client, true);
  }

  if (dispatcher_ptr->system_playback_client != NULL)
  {
    ladish_graph_remove_client(dispatcher_ptr->studio_graph, dispatcher_ptr->system_playback_client, true);
  }
}

#undef dispatcher_ptr
