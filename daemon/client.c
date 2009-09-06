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

struct ladish_client_graph_link
{
  struct list_head siblings_client;
  struct list_head siblings_graph;
  char * graph_opath;
  char * name;
  uint64_t id;
  struct ladish_client * owner_ptr;
};

struct ladish_client
{
  struct list_head siblings_studio_all;    /* link for the studio::all_clients list */
  struct list_head siblings_room;          /* link for the room::clients list */
  struct list_head ports;                  /* List of ports that belong to the client */
  uuid_t uuid;                             /* The UUID of the client */
  uint64_t jack_id;                        /* JACK client ID */
  char * jack_name;                        /* JACK name, not valid for virtual clients */
  bool virtual:1;                          /* Whether client is virtual */
  bool link:1;                             /* Whether client is a studio-room link */
  bool system:1;                           /* Whether client is system (server in-process) */
  pid_t pid;                               /* process id. Not valid for system and virtual clients */
  struct room * room_ptr;                  /* Room this client belongs to. If NULL, client belongs only to studio guts. */
  struct list_head graph_links;            /* list of ladish_client_graph_link structs linked through ladish_client_graph_link::siblings_client */
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
  INIT_LIST_HEAD(&client_ptr->ports);
  client_ptr->jack_id = 0;
  client_ptr->jack_name = NULL;
  client_ptr->virtual = virtual;
  client_ptr->link = link;
  client_ptr->system = system;
  client_ptr->pid = 0;
  client_ptr->room_ptr = NULL;
  INIT_LIST_HEAD(&client_ptr->graph_links);

  *client_handle_ptr = (ladish_client_handle)client_ptr;
  return true;
}

static
struct ladish_client_graph_link *
ladish_client_find_graph_link(
  struct ladish_client * client_ptr,
  const char * opath)
{
  struct list_head * node_ptr;
  struct ladish_client_graph_link * link_ptr;

  list_for_each(node_ptr, &client_ptr->graph_links)
  {
    link_ptr = list_entry(node_ptr, struct ladish_client_graph_link, siblings_client);
    if (strcmp(opath, link_ptr->graph_opath) == 0)
    {
      return link_ptr;
    }
  }

  return NULL;
}

#define client_ptr ((struct ladish_client *)client_handle)

bool
ladish_client_set_graph_name(
  ladish_client_handle client_handle,
  const char * opath,
  const char * name)
{
  struct ladish_client_graph_link * link_ptr;

  lash_info("setting %s name of client %p to '%s'", opath, client_handle, name);

  link_ptr = malloc(sizeof(struct ladish_client_graph_link));
  if (link_ptr == NULL)
  {
    lash_error("malloc() failed for struct ladish_client_link_ptr");
    return false;
  }

  link_ptr->graph_opath = strdup(opath);
  if (link_ptr->graph_opath == NULL)
  {
    lash_error("strdup() failed for graph opath");
    free(link_ptr);
    return false;
  }

  link_ptr->name = strdup(name);
  if (link_ptr->name == NULL)
  {
    lash_error("strdup() failed for graph client name");
    free(link_ptr->graph_opath);
    free(link_ptr);
    return false;
  }

  link_ptr->id = 0;
  link_ptr->owner_ptr = client_ptr;
  INIT_LIST_HEAD(&link_ptr->siblings_graph);
  list_add_tail(&link_ptr->siblings_client, &client_ptr->graph_links);
  return true;
}

void
ladish_client_drop_graph_name(
  ladish_client_handle client_handle,
  const char * opath)
{
  struct ladish_client_graph_link * link_ptr;

  link_ptr = ladish_client_find_graph_link(client_ptr, opath);
  if (link_ptr != NULL)
  {
    list_del(&link_ptr->siblings_client);
    free(link_ptr->name);
    free(link_ptr->graph_opath);
    free(link_ptr);
  }
}

const char *
ladish_client_get_graph_name(
  ladish_client_handle client_handle,
  const char * opath)
{
  struct ladish_client_graph_link * link_ptr;

  link_ptr = ladish_client_find_graph_link(client_ptr, opath);
  if (link_ptr != NULL)
  {
      return link_ptr->name;
  }

  return NULL;
}

void
ladish_client_set_graph_id(
  ladish_client_handle client_handle,
  const char * opath,
  uint64_t id)
{
  struct ladish_client_graph_link * link_ptr;

  link_ptr = ladish_client_find_graph_link(client_ptr, opath);
  if (link_ptr != NULL)
  {
    link_ptr->id = id;
  }
  else
  {
    assert(false);
  }
}

uint64_t
ladish_client_get_graph_id(
  ladish_client_handle client_handle,
  const char * opath)
{
  struct ladish_client_graph_link * link_ptr;

  link_ptr = ladish_client_find_graph_link(client_ptr, opath);
  if (link_ptr != NULL)
  {
    assert(link_ptr->id != 0); /* nobody set it yet */
    return link_ptr->id;
  }
  else
  {
    assert(false);
    return 0;
  }
}

struct list_head *
ladish_client_get_graph_link(
  ladish_client_handle client_handle,
  const char * opath)
{
  struct ladish_client_graph_link * link_ptr;

  link_ptr = ladish_client_find_graph_link(client_ptr, opath);
  if (link_ptr == NULL)
  {
    assert(false);
    return NULL;
  }

  return &link_ptr->siblings_graph;
}

ladish_client_handle
ladish_client_get_by_graph_link(
  struct list_head * link)
{
  struct ladish_client_graph_link * link_ptr;
  link_ptr = list_entry(link, struct ladish_client_graph_link, siblings_graph);
  return (ladish_client_handle)link_ptr->owner_ptr;
}

void
ladish_client_destroy(
  ladish_client_handle client_handle)
{
  struct list_head * node_ptr;
  struct ladish_client_graph_link * link_ptr;

  while (!list_empty(&client_ptr->graph_links))
  {
    node_ptr = client_ptr->graph_links.next;
    list_del(node_ptr);
    link_ptr = list_entry(node_ptr, struct ladish_client_graph_link, siblings_client);
    if (!list_empty(&link_ptr->siblings_graph))
    {
      list_del_init(&link_ptr->siblings_graph);
    }
    free(link_ptr->name);
    free(link_ptr->graph_opath);
    free(link_ptr);
  }

  assert(list_empty(&client_ptr->ports));
  assert(list_empty(&client_ptr->siblings_studio_all));
  assert(list_empty(&client_ptr->siblings_room));

  if (client_ptr->jack_name != NULL)
  {
    free(client_ptr->jack_name);
  }

  free(client_ptr);
}

#undef client_ptr
