/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <stdbool.h>

#include "../common/safety.h"
#include "../common/debug.h"

#include "client_dependency.h"
#include "client.h"
#include "project.h"

/*
 * Find out whether adding a dependency on client with ID 'new_id' would
 * introduce a circular dependency to the dependency chain originating
 * at client with ID 'orig_id'.
 */
static bool
client_dependency_find_circular(struct list_head *client_list,
                                uuid_t            orig_id,
                                uuid_t            new_id)
{
  struct lash_client *client = NULL;
  struct list_head *node;
  client_dependency_t *dep;

  client = project_get_client_by_id(client_list, new_id);

  /* Didn't find an existing client with id new_id, no possibility
     of a circular dependency here */
  if (!client)
    return false;

  /* Check the client's dependencies for matches with the original ID */
  list_for_each(node, &client->dependencies) {
    dep = list_entry(node, client_dependency_t, siblings);
    if (uuid_compare(orig_id, dep->client_id) == 0
        || client_dependency_find_circular(client_list, orig_id,
                                           dep->client_id))
      /* Found circular dependency */
      return true;
  }

  /* Didn't find a circular dependency along this path */
  return false;
}

void
client_dependency_add(struct list_head *client_list,
                      struct lash_client         *client,
                      uuid_t            client_id)
{
  if (!client_list || !client) {
    lash_error("Invalid arguments");
    return;
  }

  struct list_head *node;
  client_dependency_t *dep;

  /* Check if we're trying to add a duplicate dependency */
  list_for_each(node, &client->dependencies) {
    dep = list_entry(node, client_dependency_t, siblings);
    if (uuid_compare(client_id, dep->client_id) == 0) {
      /* Found duplicate dependency */
      lash_error("Refusing to add duplicate dependency");
      return;
    }
  }

  /* Check if adding dependency would introduce a circular */
  if (client_dependency_find_circular(client_list, client->id, client_id)) {
    lash_error("Refusing to add circular dependency");
    return;
  }

  dep = lash_calloc(1, sizeof(client_dependency_t));
  uuid_copy(dep->client_id, client_id);
  list_add(&dep->siblings, &client->dependencies);
#ifdef LASH_DEBUG
  char id_str[37];
  uuid_unparse(client_id, id_str);
  lash_debug("Added dependency on client %s to client '%s'",
             id_str, client->name);
#endif
}

void
client_dependency_remove(struct list_head *head,
                         uuid_t            client_id)
{
  if (!head) {
    lash_error("List head pointer is NULL");
    return;
  }

  struct list_head *node;
  client_dependency_t *dep;

  list_for_each(node, head) {
    dep = list_entry(node, client_dependency_t, siblings);
    if (uuid_compare(dep->client_id, client_id) == 0) {
      list_del(node);
      free(dep);
#ifdef LASH_DEBUG
      char id_str[37];
      uuid_unparse(client_id, id_str);
      lash_debug("Removed dependency on client %s", id_str);
#endif
      break;
    }
  }
}

void
client_dependency_list_sanity_check(struct list_head *client_list,
                                    struct lash_client         *client)
{
  if (!client_list || !client) {
    lash_error("Invalid arguments");
    return;
  }

  struct list_head *head, *node;
  client_dependency_t *dep;

  lash_debug("Sanity checking client %s dependencies", client->id_str);

  head = &client->dependencies;
  node = head->next;

  while (node != head) {
    dep = list_entry(node, client_dependency_t, siblings);
    node = node->next;

    if (!project_get_client_by_id(client_list, dep->client_id)) {
#ifdef LASH_DEBUG
      char id_str[37];
      uuid_unparse(dep->client_id, id_str);
      lash_debug("Dropping bogus dependency on nonexistent "
                 "client %s", id_str);
#endif
      list_del(&dep->siblings);
      free(dep);
    }
  }
}

void
client_dependency_init_unsatisfied(struct lash_client *client)
{
  if (!client) {
    lash_error("Client pointer is NULL");
    return;
  }

  /* Clear the existing list */
  if (!list_empty(&client->unsatisfied_deps))
    client_dependency_remove_all(&client->unsatisfied_deps);

  /* Nothing to do */
  if (list_empty(&client->dependencies))
    return;

  struct list_head *node;
  client_dependency_t *dep, *new;

  list_for_each(node, &client->dependencies) {
    dep = list_entry(node, client_dependency_t, siblings);
    new = lash_calloc(1, sizeof(client_dependency_t));
    uuid_copy(new->client_id, dep->client_id);
    list_add(&new->siblings, &client->unsatisfied_deps);
  }
}

void
client_dependency_remove_all(struct list_head *head)
{
  if (!head) {
    lash_error("List head pointer is NULL");
    return;
  }

  struct list_head *node;
  client_dependency_t *dep;

  node = head->next;

  while (node != head) {
    dep = list_entry(node, client_dependency_t, siblings);
    node = node->next;
    free(dep);
  }

  INIT_LIST_HEAD(head);
}

/* EOF */
