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

#ifndef __LASH_CLIENT_INTERFACE_H_
#define __LASH_CLIENT_INTERFACE_H_

#include <stdint.h>

#include <lash/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define lash_enabled(client)  ((client) && lash_server_connected (client))

/**
 * Extract lash-specific arguments from argc/argv.
 * This should be done before the client checks them, obviously
 */
lash_args_t *
lash_extract_args(int    *argc,
                  char ***argv);

/**
 * Destroy a lash_args_t (returned from lash_extract_args).
 */
void
lash_args_destroy(lash_args_t *args);

/**
 * Open a connection to the server.
 * Returns NULL on failure.
 */
lash_client_t *
lash_init(const lash_args_t *args,
          const char        *client_class,
          int                client_flags,
          lash_protocol_t    protocol);

/**
 * Get the hostname of the server.
 */
const char *
lash_get_server_name(lash_client_t *client);

/**
 * Get the number of pending events.
 */
unsigned int
lash_get_pending_event_count(lash_client_t *client);

/**
 * Retrieve an event.
 * The event must be freed using lash_event_destroy.
 * Returns NULL if there are no events pending.
 */
lash_event_t *
lash_get_event(lash_client_t *client);

/**
 * Get the number of pending configs.
 */
unsigned int
lash_get_pending_config_count(lash_client_t *client);

/**
 * Retrieve a config.
 * The config must be freed using lash_config_destroy.
 * Returns NULL if there are no configs pending.
 */
lash_config_t *
lash_get_config(lash_client_t *client);

/**
 * Send an event to the server.
 * The event must be created using lash_event_new or lash_event_new_with_type.
 * The program takes over ownership of the memory and it should not be freed
 * by the client.
 */
void
lash_send_event(lash_client_t *client,
                lash_event_t  *event);

/**
 * Send some data to the server.
 * The config must be created using lash_config_new, lash_config_new_with_key
 * or lash_config_dup.  The program takes over ownership of the memory and
 * it should not be freed by the client.
 */
void
lash_send_config(lash_client_t *client,
                 lash_config_t *config);

/**
 * Check whether the server is connected.
 * Returns 1 if the server is still connected or 0 if it isn't.
 */
int
lash_server_connected(lash_client_t *client);

/**
 * Tell the server the client's JACK client name.
 */
void
lash_jack_client_name(lash_client_t *client,
                      const char    *name);

void
lash_alsa_client_id(lash_client_t *client,
                    unsigned char  id);

#ifdef __cplusplus
}
#endif


#endif /* __LASH_CLIENT_INTERFACE_H_ */
