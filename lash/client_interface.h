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

#ifndef __LASH_CLIENT_INTERFACE_H_
#define __LASH_CLIENT_INTERFACE_H_

#include <stdint.h>

#include <lash/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define lash_enabled(client)  ((client) && lash_server_connected (client))

lash_client_t *
lash_client_open(const char  *client_class,
                 int          flags,
                 int          argc,
                 char       **argv);

const char *
lash_get_client_name(lash_client_t *client);

const char *
lash_get_project_name(lash_client_t *client);

/**
 * Tell the server the client's JACK client name.
 */
void
lash_jack_client_name(lash_client_t *client,
                      const char    *name);

/**
 * Tell the server the client's ALSA client ID.
 */
void
lash_alsa_client_id(lash_client_t *client,
                    unsigned char  id);

/**
 * Set the Save callback function.
 */
bool
lash_set_save_callback(lash_client_t     *client,
                       LashEventCallback  callback,
                       void              *user_data);

/**
 * Set the Load callback function.
 */
bool
lash_set_load_callback(lash_client_t     *client,
                       LashEventCallback  callback,
                       void              *user_data);

/**
 * Set the SaveDataSet callback function.
 */
bool
lash_set_save_data_set_callback(lash_client_t      *client,
                                LashConfigCallback  callback,
                                void               *user_data);

/**
 * Set the LoadDataSet callback function.
 */
bool
lash_set_load_data_set_callback(lash_client_t      *client,
                                LashConfigCallback  callback,
                                void               *user_data);

/**
 * Set the Quit callback function
 */
bool
lash_set_quit_callback(lash_client_t     *client,
                       LashEventCallback  callback,
                       void              *user_data);

/**
 * Set the ClientNameChanged callback function
 */
bool
lash_set_name_change_callback(lash_client_t     *client,
                              LashEventCallback  callback,
                              void              *user_data);

/**
 * Set the ProjectChange callback function
 */
bool
lash_set_project_change_callback(lash_client_t     *client,
                                 LashEventCallback  callback,
                                 void              *user_data);

/**
 * Set the PathChange callback function
 */
bool
lash_set_path_change_callback(lash_client_t     *client,
                              LashEventCallback  callback,
                              void              *user_data);

void
lash_wait(lash_client_t *client);

void
lash_dispatch(lash_client_t *client);

bool
lash_dispatch_once(lash_client_t *client);

void
lash_notify_progress(lash_client_t *client,
                     uint8_t        percentage);

lash_client_t *
lash_client_open_controller(void);

/**
 * Set the controller callback function.
 */
bool
lash_set_control_callback(lash_client_t       *client,
                          LashControlCallback  callback,
                          void                *user_data);

void
lash_control_load_project_path(lash_client_t *client,
                               const char    *project_path);

void
lash_control_name_project(lash_client_t *client,
                          const char    *project_name,
                          const char    *new_name);

void
lash_control_move_project(lash_client_t *client,
                          const char    *project_name,
                          const char    *new_path);

void
lash_control_save_project(lash_client_t *client,
                          const char    *project_name);

void
lash_control_close_project(lash_client_t *client,
                           const char    *project_name);


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

#ifdef __cplusplus
}
#endif


#endif /* __LASH_CLIENT_INTERFACE_H_ */
