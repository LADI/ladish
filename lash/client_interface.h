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

#include <lash/types.h>
#include <lash/protocol.h>

#ifdef __cplusplus
extern "C" {
#endif

#define lash_enabled(client)  ((client) && lash_server_connected (client))

/** extract lash-specific arguments from argc/argv.
 * this should be done before the client checks them, obviously
 */
lash_args_t   * lash_extract_args (int * argc, char *** argv);

/** Destroy a lash_args_t (returned from lash_extract_args).
 */
void lash_args_destroy (lash_args_t* args);

/** open a connection to the server
 * returns NULL on failure
 */
lash_client_t * lash_init (const lash_args_t * args, const char * client_class, int client_flags, lash_protocol_t protocol); 

/** get the hostname of the server
 */
const char * lash_get_server_name (lash_client_t * client);

/** get the number of pending events
 */
unsigned int   lash_get_pending_event_count (lash_client_t * client);

/** retrieve an event
 * the event must be freed using lash_event_destroy
 * returns NULL if there are no events pending
 */
lash_event_t *  lash_get_event (lash_client_t * client);

/** get the number of pending configs
 */
unsigned int   lash_get_pending_config_count (lash_client_t * client);

/** retrieve a config
 * the config must be freed using lash_config_destroy
 * returns NULL if there are no configs pending
 */
lash_config_t * lash_get_config (lash_client_t * client);

/** send an event to the server
 * the event must be created using lash_event_new or lash_event_new_with_type.
 * the program takes over ownership of the memory and it should not be freed
 * by the client.
 */
void lash_send_event  (lash_client_t * client, lash_event_t * event);

/** send some data to the server
 * the config must be created using lash_config_new, lash_config_new_with_key
 * or lash_config_dup.  the program takes over ownership of the memory and
 * it should not be freed by the client.
 */
void lash_send_config (lash_client_t * client, lash_config_t * config);

/** check whether the server is connected
 * returns 1 if the server is still connected or 0 if it isn't
 */
int lash_server_connected (lash_client_t * client);

/** tell the server the client's jack client name */
void lash_jack_client_name (lash_client_t * client, const char * name);

/** tell the server the client's alsa client id */
void lash_alsa_client_id (lash_client_t * client, unsigned char id);

#ifdef __cplusplus
}
#endif


#endif /* __LASH_CLIENT_INTERFACE_H_ */
