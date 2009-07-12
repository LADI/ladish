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

#ifndef __LASHD_CLIENT_DEPENDENCY_H__
#define __LASHD_CLIENT_DEPENDENCY_H__

#include <uuid/uuid.h>

#include "../common/klist.h"

#include "types.h"
#include "client.h"

/**
 * A client dependency list entry.
 */
struct _client_dependency
{
  struct list_head siblings;
  uuid_t           client_id;
};

/**
 * Add a dependency to a client. The operation will fail if adding the
 * dependency would introduce a circular dependency to the client's
 * dependency chain.
 *
 * @param client_list The client list that the client belongs to.
 * @param client The client to add the dependency to.
 * @param client_id The client ID of the dependency.
 */
void
client_dependency_add(struct list_head *client_list,
                      struct lash_client         *client,
                      uuid_t            client_id);

/**
 * Remove a dependency from a client's dependency list.
 *
 * @param head The dependency list head; a pointer to a 'dependencies'
 *             or 'unsatisfied_deps' member of a client object.
 * @param client_id The client ID of the dependency to remove.
 */
void
client_dependency_remove(struct list_head *head,
                         uuid_t            client_id);

/**
 * Check a client's dependency list for dependencies on nonexistant
 * clients and drop bogus dependencies from the list.
 *
 * @param client_list A list of clients to compare the dependencies against.
 * @param client The client whose dependencies to sanity check.
 */
void
client_dependency_list_sanity_check(struct list_head *client_list,
                                    struct lash_client         *client);

/**
 * Initialize a client's unsatisfied dependencies. Simply builds
 * a fresh 'unsatisfied_deps' list from the 'dependencies' list.
 *
 * @param client The client whose unsatisfied dependencies to initialize.
 */
void
client_dependency_init_unsatisfied(struct lash_client *client);

/**
 * Remove all dependencies from a client's dependency list.
 *
 * @param head The dependency list head; a pointer to a 'dependencies'
 *             or 'unsatisfied_deps' member of a client object.
 */
void
client_dependency_remove_all(struct list_head *head);

#endif /* __LASHD_CLIENT_DEPENDENCY_H__ */
