/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the implementation of the client objects
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
#include "client.h"

struct ladish_client_graph_name
{
  struct list_head siblings;
  char * graph_opath;
  char * client_name;
};

struct ladish_client
{
  struct list_head siblings_studio_all;    /* link for the studio::all_clients list */
  struct list_head siblings_room;          /* link for the room::clients list */
  struct list_head siblings_graph;
  struct list_head ports;                  /* List of ports that belong to the client */
  uint64_t graph_id;
  uuid_t uuid;                             /* The UUID of the client */
  uint64_t jack_id;                        /* JACK client ID */
  char * jack_name;                        /* JACK name, not valid for virtual clients */
  char * human_name;                       /* User assigned name, not valid for studio-room link clients */
  bool virtual:1;                          /* Whether client is virtual */
  bool link:1;                             /* Whether client is a studio-room link */
  bool system:1;                           /* Whether client is system (server in-process) */
  pid_t pid;                               /* process id. Not valid for system and virtual clients */
  struct room * room_ptr;                  /* Room this client belongs to. If NULL, client belongs only to studio guts. */
  struct list_head graph_names;
};

bool
ladish_client_create(
  uuid_t uuid_ptr,
  bool virtual,
  bool link,
  bool system,
  ladish_client_handle * client_handle_ptr)
{
  struct ladish_client * client_ptr;

  client_ptr = malloc(sizeof(struct ladish_client));
  if (client_ptr == NULL)
  {
    lash_error("malloc() failed to allocate struct ladish_client");
    return false;
  }

  if (uuid_ptr == NULL)
  {
    uuid_generate(client_ptr->uuid);
  }
  else
  {
    uuid_copy(client_ptr->uuid, uuid_ptr);
  }

  INIT_LIST_HEAD(&client_ptr->siblings_studio_all);
  INIT_LIST_HEAD(&client_ptr->siblings_room);
  INIT_LIST_HEAD(&client_ptr->siblings_graph);
  INIT_LIST_HEAD(&client_ptr->ports);
  client_ptr->graph_id = 0;
  client_ptr->jack_id = 0;
  client_ptr->jack_name = NULL;
  client_ptr->human_name = NULL;
  client_ptr->virtual = virtual;
  client_ptr->link = link;
  client_ptr->system = system;
  client_ptr->pid = 0;
  client_ptr->room_ptr = NULL;
  INIT_LIST_HEAD(&client_ptr->graph_names);

  *client_handle_ptr = (ladish_client_handle)client_ptr;
  return true;
}

#define client_ptr ((struct ladish_client *)client_handle)

bool
ladish_client_set_graph_name(
  ladish_client_handle client_handle,
  const char * opath,
  const char * name)
{
  struct ladish_client_graph_name * graph_name;

  lash_info("setting %s name of client %p to '%s'", opath, client_handle, name);

  graph_name = malloc(sizeof(struct ladish_client_graph_name));
  if (graph_name == NULL)
  {
    lash_error("malloc() failed for struct ladish_client_graph_name");
    return false;
  }

  graph_name->graph_opath = strdup(opath);
  if (graph_name->graph_opath == NULL)
  {
    lash_error("strdup() failed for graph opath");
    free(graph_name);
    return false;
  }

  graph_name->client_name = strdup(name);
  if (graph_name->client_name == NULL)
  {
    lash_error("strdup() failed for graph client name");
    free(graph_name->graph_opath);
    free(graph_name);
    return false;
  }

  list_add_tail(&graph_name->siblings, &client_ptr->graph_names);
  return true;
}

void
ladish_client_drop_graph_name(
  ladish_client_handle client_handle,
  const char * opath)
{
  struct list_head * node_ptr;
  struct ladish_client_graph_name * graph_name;

  list_for_each(node_ptr, &client_ptr->graph_names)
  {
    graph_name = list_entry(node_ptr, struct ladish_client_graph_name, siblings);
    if (strcmp(opath, graph_name->graph_opath) == 0)
    {
      list_del(node_ptr);
      free(graph_name->client_name);
      free(graph_name->graph_opath);
      free(graph_name);
      return;
    }
  }
}

const char *
ladish_client_get_graph_name(
  ladish_client_handle client_handle,
  const char * opath)
{
  struct list_head * node_ptr;
  struct ladish_client_graph_name * graph_name;

  list_for_each(node_ptr, &client_ptr->graph_names)
  {
    graph_name = list_entry(node_ptr, struct ladish_client_graph_name, siblings);
    if (strcmp(opath, graph_name->graph_opath) == 0)
    {
      return graph_name->client_name;
    }
  }

  return NULL;
}

void
ladish_client_set_graph_id(
  ladish_client_handle client_handle,
  uint64_t id)
{
  client_ptr->graph_id = id;
}

uint64_t
ladish_client_get_graph_id(
  ladish_client_handle client_handle)
{
  assert(client_ptr->graph_id != 0); /* nobody set it yet */
  return client_ptr->graph_id;
}

struct list_head *
ladish_client_get_graph_link(
  ladish_client_handle client_handle)
{
  return &client_ptr->siblings_graph;
}

ladish_client_handle
ladish_client_get_by_graph_link(
  struct list_head * link)
{
  return (ladish_client_handle)list_entry(link, struct ladish_client, siblings_graph);
}

void
ladish_client_detach(
  ladish_client_handle client_handle)
{
  if (!list_empty(&client_ptr->siblings_graph))
  {
    list_del_init(&client_ptr->siblings_graph);
  }
}

void
ladish_client_destroy(
  ladish_client_handle client_handle)
{
  struct list_head * node_ptr;
  struct ladish_client_graph_name * graph_name;

  ladish_client_detach(client_handle);

  assert(list_empty(&client_ptr->ports));
  assert(list_empty(&client_ptr->siblings_studio_all));
  assert(list_empty(&client_ptr->siblings_room));

  while (!list_empty(&client_ptr->graph_names))
  {
    node_ptr = client_ptr->graph_names.next;
    graph_name = list_entry(node_ptr, struct ladish_client_graph_name, siblings);
    list_del(node_ptr);
    free(graph_name->client_name);
    free(graph_name->graph_opath);
    free(graph_name);
  }

  if (client_ptr->jack_name != NULL)
  {
    free(client_ptr->jack_name);
  }

  if (client_ptr->human_name != NULL)
  {
    free(client_ptr->human_name);
  }

  free(client_ptr);
}

#undef client_ptr
