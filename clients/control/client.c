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

#include <lash/lash.h>
#include <lash/internal_headers.h>

#include "client.h"

client_t *
client_new()
{
	client_t *client;

	client = lash_malloc0(sizeof(client_t));
	uuid_clear(client->id);
	return client;
}

void
client_destroy(client_t * client)
{
	client_set_name(client, NULL);
	client_set_jack_client_name(client, NULL);
	free(client);
}

void
client_set_name(client_t * client, const char *name)
{
	set_string_property(client->name, name);
}

void
client_set_jack_client_name(client_t * client, const char *name)
{
	set_string_property(client->jack_client_name, name);
}

const char *
client_get_identity(client_t * client)
{
	static char *identity = NULL;
	static size_t identity_size = sizeof(char[37]);

	if (!identity)
		identity = lash_malloc(identity_size);

	if (client->name) {
		size_t name_size;

		name_size = strlen(client->name) + 1;
		if (name_size > identity_size) {
			identity_size = name_size;
			identity = lash_realloc(identity, identity_size);
		}

		strcpy(identity, client->name);
	} else
		uuid_unparse(client->id, identity);

	return identity;
}

/* EOF */
