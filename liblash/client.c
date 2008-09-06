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

#include <stdlib.h>

#include "common/safety.h"

#include "client.h"

lash_client_t *
lash_client_new(void)
{
	return lash_calloc(1, sizeof(lash_client_t));
}

void
lash_client_destroy(lash_client_t * client)
{
	if (client) {
		lash_free(&client->class);
		lash_free(&client->project_name);

		if (client->argv) {
			int i;
			for (i = 0; i < client->argc; ++i) {
				free(client->argv[i]);
			}
			free(client->argv);
		}

		free(client);
	}
}

#ifdef LASH_OLD_API
void
lash_client_add_event(lash_client_t *client,
                      lash_event_t  *event)
{
	if (client && event) {
		client->events_in = lash_list_append(client->events_in, event);
		++client->num_events_in;
	}
}

void
lash_client_add_config(lash_client_t *client,
                       lash_config_t *config)
{
	if (client && config) {
		client->configs_in = lash_list_append(client->configs_in, config);
		++client->num_configs_in;
	}
}
#endif

/* EOF */
