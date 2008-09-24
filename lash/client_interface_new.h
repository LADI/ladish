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

#ifndef __LASH_CLIENT_INTERFACE_NEW_H_
#define __LASH_CLIENT_INTERFACE_NEW_H_

#ifdef __cplusplus
extern "C" {
#endif

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


#ifdef __cplusplus
}
#endif


#endif /* __LASH_CLIENT_INTERFACE_NEW_H_ */
