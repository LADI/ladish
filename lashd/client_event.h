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

#ifndef __LASHD_CLIENT_EVENT_H__
#define __LASHD_CLIENT_EVENT_H__

#include <lash/lash.h>

#include "server.h"
#include "server_event.h"
#include "client.h"
#include "project.h"

void server_lash_event_client_name      (project_t * project, client_t * client, client_t * query_client, const char * name);
void server_lash_event_jack_client_name (project_t * project, client_t * client, client_t * query_client, const char * name);
void server_lash_event_alsa_client_id   (project_t * project, client_t * client, client_t * query_client, const char * string);
void server_lash_event_restore_data_set (project_t * project, client_t * client);
void server_lash_event_save_data_set    (project_t * project, client_t * client);
void server_lash_event_save_file        (project_t * project, client_t * client);
void server_lash_event_save             (project_t * project, client_t * client);

void server_lash_event_project_add (server_t * server, const char * dir);
void server_lash_event_project_dir (project_t * project, client_t * client, const char * dir);
void server_lash_event_project_name (project_t * project, const char * name);
void server_lash_event_client_remove (project_t * project, client_t * client);

#endif /* __LASHD_CLIENT_EVENT_H__ */
