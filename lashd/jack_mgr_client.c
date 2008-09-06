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

#include "common/safety.h"
#include "common/debug.h"

#include "jack_mgr_client.h"
#include "jack_patch.h"

jack_mgr_client_t *
jack_mgr_client_new(void)
{
	return lash_calloc(1, sizeof(jack_mgr_client_t));
}

void
jack_mgr_client_destroy(jack_mgr_client_t *client)
{
	if (client) {
		lash_free(&client->name);
		jack_mgr_client_free_patch_list(client->old_patches);
		jack_mgr_client_free_patch_list(client->backup_patches);
#ifndef HAVE_JACK_DBUS
		jack_mgr_client_free_patch_list(client->patches);
#endif
		free(client);
	}
}

lash_list_t *
jack_mgr_client_dup_patch_list(lash_list_t *patch_list)
{
	lash_list_t *node, *new_list = NULL;
	jack_patch_t *new_patch;

	for (node = patch_list; node; node = lash_list_next(node)) {
		new_patch = jack_patch_dup((jack_patch_t *) node->data);
		new_list = lash_list_append(new_list, new_patch);
	}

	return new_list;
}

void
jack_mgr_client_free_patch_list(lash_list_t *patch_list)
{
	if (patch_list) {
		lash_list_t *node;

		for (node = patch_list; node; node = lash_list_next(node))
			jack_patch_destroy((jack_patch_t *) node->data);

		lash_list_free(patch_list);
	}
}

jack_mgr_client_t *
jack_mgr_client_find_by_id(lash_list_t *client_list,
                           uuid_t       id)
{
	if (client_list) {
		lash_list_t *node;
		jack_mgr_client_t *client;

		for (node = client_list; node; node = lash_list_next(node)) {
			client = (jack_mgr_client_t *) node->data;

			if (uuid_compare(id, client->id) == 0)
				return client;
		}
	}

	return NULL;
}

#ifdef HAVE_JACK_DBUS

jack_mgr_client_t *
jack_mgr_client_find_by_jackdbus_id(lash_list_t   *client_list,
                                    dbus_uint64_t  id)
{
	if (client_list) {
		lash_list_t *node;
		jack_mgr_client_t *client;

		for (node = client_list; node; node = lash_list_next(node)) {
			client = (jack_mgr_client_t *) node->data;

			if (client->jackdbus_id == id)
				return client;
		}
	}

	return NULL;
}

#else /* !HAVE_JACK_DBUS */

lash_list_t *
jack_mgr_client_dup_uniq_patches(lash_list_t *jack_mgr_clients,
                                 uuid_t       client_id)
{
	jack_mgr_client_t *client;
	lash_list_t *node, *uniq_node, *patches, *uniq_patches = NULL;
	jack_patch_t *patch, *uniq_patch;

	client = jack_mgr_client_find_by_id(jack_mgr_clients, client_id);
	if (!client) {
		char id_str[37];
		uuid_unparse(client_id, id_str);
		lash_error("Unknown client %s", id_str);
		return NULL;
	}

	patches = jack_mgr_client_dup_patch_list(client->patches);

	for (node = patches; node; node = lash_list_next(node)) {
		patch = (jack_patch_t *) node->data;

		jack_patch_set(patch, jack_mgr_clients);

		for (uniq_node = uniq_patches; uniq_node;
		     uniq_node = lash_list_next(uniq_node)) {
			uniq_patch = (jack_patch_t *) uniq_node->data;

			if (strcmp(patch->src_client,
			           uniq_patch->src_client) == 0
			    && strcmp(patch->src_port,
			              uniq_patch->src_port) == 0
			    && strcmp(patch->dest_client,
			              uniq_patch->dest_client) == 0
			    && strcmp(patch->dest_port,
			              uniq_patch->dest_port) == 0)
				break;
		}

		if (uniq_node)
			jack_patch_destroy(patch);
		else
			uniq_patches = lash_list_append(uniq_patches, patch);
	}

	lash_list_free(patches);

	return uniq_patches;
}

#endif

/* EOF */
