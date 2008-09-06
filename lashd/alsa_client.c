/*
 *   LASH
 *
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

#include "alsa_client.h"

#ifdef HAVE_ALSA

# define _GNU_SOURCE

# include <stdlib.h>

# include "alsa_patch.h"
# include "common/safety.h"
# include "common/debug.h"

void
alsa_client_init(alsa_client_t * client)
{
	client->client_id = 0;
	client->patches = NULL;
	client->old_patches = NULL;
	client->backup_patches = NULL;

	uuid_clear(client->id);
}

static void
alsa_client_free_patch_list(lash_list_t ** list_ptr)
{
	lash_list_t *list;
	alsa_patch_t *patch;

	for (list = *list_ptr; list; list = lash_list_next(list)) {
		patch = (alsa_patch_t *) list->data;
		if (!patch) {
			lash_debug("NULL patch!");
		} else
			alsa_patch_destroy(patch);
	}

	lash_list_free(*list_ptr);

	*list_ptr = NULL;
}

void
alsa_client_free_patches(alsa_client_t * client)
{
	if (!client->patches)
		return;

	alsa_client_free_patch_list(&client->patches);
}

void
alsa_client_free_backup_patches(alsa_client_t * client)
{
	if (!client->backup_patches)
		return;

	alsa_client_free_patch_list(&client->backup_patches);
}

static void
alsa_client_free_old_patches(alsa_client_t * client)
{
	alsa_client_free_patch_list(&client->old_patches);
}

void
alsa_client_free(alsa_client_t * client)
{
	alsa_client_free_patches(client);
	alsa_client_free_old_patches(client);
}

alsa_client_t *
alsa_client_new()
{
	alsa_client_t *client;

	client = lash_malloc(1, sizeof(alsa_client_t));
	alsa_client_init(client);
	return client;
}

void
alsa_client_destroy(alsa_client_t * client)
{
	alsa_client_free(client);
	free(client);
}

void
alsa_client_set_id(alsa_client_t * client, uuid_t id)
{
	uuid_copy(client->id, id);
}

void
alsa_client_set_client_id(alsa_client_t * client, unsigned char id)
{
	client->client_id = id;
}

lash_list_t *
alsa_client_dup_patches(const alsa_client_t * client)
{
	lash_list_t *list = NULL, *exlist;
	alsa_patch_t *patch;

	for (exlist = client->patches; exlist; exlist = lash_list_next(exlist)) {
		patch = alsa_patch_dup((alsa_patch_t *) exlist->data);
		list = lash_list_append(list, patch);
	}

	return list;
}

lash_list_t *
alsa_client_get_patches(alsa_client_t * client)
{
	return client->patches;
}

unsigned char
alsa_client_get_client_id(const alsa_client_t * client)
{
	return client->client_id;
}

void
alsa_client_get_id(const alsa_client_t * client, uuid_t id)
{
	uuid_copy(id, ((alsa_client_t *) client)->id);
}

#endif /* HAVE_ALSA */
