/*
 *   LASH
 *
 *   Copyright (C) 2003 Robert Ham <rah@bash.sh>
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

#include <string.h>
#include <lash/lash.h>

#include "client.h"
#include "common/safety.h"

client_t *
client_new(uuid_t id)
{
	client_t *client = lash_calloc(1, sizeof(client_t));
	uuid_copy(client->id, id);
	return client;
}

void
client_destroy(client_t *client)
{
	if (client) {
		lash_free(&client->name);
		lash_free(&client->jack_client_name);
		free(client);
	}
}

const char *
client_get_identity(client_t * client)
{
	static char *identity = NULL;
	static size_t identity_size = sizeof(char[37]);

	if (!identity)
		identity = lash_malloc(1, identity_size);

	if (client->name) {
		size_t name_size;

		name_size = strlen(client->name) + 1;
		if (name_size > identity_size) {
			identity_size = name_size;
			identity = lash_realloc(identity, 1, identity_size);
		}

		strcpy(identity, client->name);
	} else
		uuid_unparse(client->id, identity);

	return identity;
}

/* EOF */
