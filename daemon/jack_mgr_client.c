/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *   Copyright (C) 2002 Robert Ham <rah@bash.sh>
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

#define _GNU_SOURCE

#include <string.h>

#include "../common/safety.h"
#include "../common/debug.h"

#include "jack_mgr_client.h"
#include "jack_patch.h"
#include "client.h"
#include "project.h"
#include "server.h"

jack_mgr_client_t *
jack_mgr_client_new(void)
{
	jack_mgr_client_t *client;

	if ((client = lash_calloc(1, sizeof(jack_mgr_client_t)))) {
		INIT_LIST_HEAD(&client->old_patches);
		INIT_LIST_HEAD(&client->backup_patches);
	}

	return client;
}

void
jack_mgr_client_destroy(jack_mgr_client_t *client)
{
	if (client) {
		lash_free(&client->name);
		jack_mgr_client_free_patch_list(&client->old_patches);
		jack_mgr_client_free_patch_list(&client->backup_patches);
		free(client);
	}
}

void
jack_mgr_client_dup_patch_list(struct list_head *src,
                               struct list_head *dest)
{
	struct list_head *node;
	jack_patch_t *patch;

	list_for_each(node, src) {
		patch = jack_patch_dup(list_entry(node, jack_patch_t, siblings));
		list_add_tail(&patch->siblings, dest);
	}
}

void
jack_mgr_client_free_patch_list(struct list_head *patch_list)
{
	struct list_head *node, *next;

	list_for_each_safe(node, next, patch_list)
		jack_patch_destroy(list_entry(node, jack_patch_t, siblings));
}

jack_mgr_client_t *
jack_mgr_client_find_by_id(struct list_head *client_list,
                           uuid_t            id)
{
	if (client_list) {
		struct list_head *node;
		jack_mgr_client_t *client;

		list_for_each (node, client_list) {
			client = list_entry(node, jack_mgr_client_t, siblings);

			if (uuid_compare(id, client->id) == 0)
				return client;
		}
	}

	return NULL;
}

jack_mgr_client_t *
jack_mgr_client_find_by_jackdbus_id(struct list_head   *client_list,
                                    dbus_uint64_t       id)
{
	if (client_list) {
		struct list_head *node;
		jack_mgr_client_t *client;

		list_for_each (node, client_list) {
			client = list_entry(node, jack_mgr_client_t, siblings);

			if (client->jackdbus_id == id)
				return client;
		}
	}

	return NULL;
}

jack_mgr_client_t *
jack_mgr_client_find_by_pid(struct list_head *client_list,
                            pid_t             pid)
{
	if (client_list) {
		struct list_head *node;
		jack_mgr_client_t *client;

		list_for_each (node, client_list) {
			client = list_entry(node, jack_mgr_client_t, siblings);

			if (client->pid == pid)
				return client;
		}
	}

	return NULL;
}

void
jack_mgr_client_modified(jack_mgr_client_t *client)
{
	struct lash_client *lash_client;
	if ((lash_client = server_find_client_by_id(client->id))
	    && lash_client->project)
		project_set_modified_status(lash_client->project, true);
}

/* EOF */
