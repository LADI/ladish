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

#define _GNU_SOURCE

#include <unistd.h>
#include <errno.h>

#include <lash/lash.h>
#include <lash/loader.h>
#include <lash/internal_headers.h>

#include "client_event.h"

#ifndef MAXHOSTNAMELEN
#  define MAXHOSTNAMELEN  64
#endif

void
server_lash_event_client_name(project_t * project, client_t * client,
							  client_t * query_client, const char *name)
{
	if (name)
		project_name_client(project, client, name);
	else {
		lash_event_t *event;

		event = lash_event_new_with_type(LASH_Client_Name);
		lash_event_set_string(event, client->name);
		lash_event_set_project(event, project->name);
		lash_event_set_client_id(event, client->id);

		conn_mgr_send_client_lash_event(project->server->conn_mgr,
										query_client->conn_id, event);
	}
}

void
server_lash_event_jack_client_name(project_t * project, client_t * client,
								   client_t * query_client, const char *name)
{
	if (name) {
		LASH_DEBUGARGS("setting jack client name for client '%s' to '%s'",
					   client_get_id_str(client), name);

		if (client->jack_client_name) {
			fprintf(stderr,
					"%s client '%s' sent more than one jack client name\n",
					__FUNCTION__, client_get_id_str(client));
			return;
		}

		client_set_jack_client_name(client, name);

		LASH_DEBUGARGS
			("adding client '%s' to jack manager with client name '%s'",
			 client_get_id_str(client), name);
		jack_mgr_lock(project->server->jack_mgr);
		jack_mgr_add_client(project->server->jack_mgr, client->id, name,
							client->jack_patches);
		jack_mgr_unlock(project->server->jack_mgr);
		client->jack_patches = NULL;
		LASH_PRINT_DEBUG("added client");

		server_notify_interfaces(project, client, LASH_Jack_Client_Name,
								 name);
	} else {
		lash_event_t *event;

		event = lash_event_new_with_type(LASH_Jack_Client_Name);
		lash_event_set_string(event, client->jack_client_name);
		lash_event_set_project(event, project->name);
		lash_event_set_client_id(event, client->id);
		conn_mgr_send_client_lash_event(project->server->conn_mgr,
										query_client->conn_id, event);
	}
}

void
server_lash_event_alsa_client_id(project_t * project, client_t * client,
								 client_t * query_client, const char *string)
{
	if (string) {
		unsigned char alsa_id;

		LASH_DEBUGARGS("setting alsa client id for client '%s' to %d",
					   client_get_id_str(client), (unsigned char)string[0]);

		if (client->alsa_client_id) {
			fprintf(stderr,
					"%s: client '%s' sent more than one alsa client id\n",
					__FUNCTION__, client_get_id_str(client));
			return;
		}

		alsa_id = (unsigned char)string[0];
		client_set_alsa_client_id(client, alsa_id);

		alsa_mgr_lock(project->server->alsa_mgr);
		alsa_mgr_add_client(project->server->alsa_mgr, client->id, alsa_id,
							client->alsa_patches);
		alsa_mgr_unlock(project->server->alsa_mgr);
		client->alsa_patches = NULL;

		server_notify_interfaces(project, client, LASH_Alsa_Client_ID,
								 string);
	} else {
		lash_event_t *event;
		char id[2];

		event = lash_event_new_with_type(LASH_Alsa_Client_ID);
		lash_event_set_project(event, project->name);
		lash_event_set_client_id(event, client->id);

		if (client->alsa_client_id) {
			id[0] = client->alsa_client_id;
			id[1] = '\0';
			lash_event_set_string(event, id);
		}

		conn_mgr_send_client_lash_event(project->server->conn_mgr,
										query_client->conn_id, event);
	}
}

void
server_lash_event_restore_data_set(project_t * project, client_t * client)
{
}

void
server_lash_event_save_data_set(project_t * project, client_t * client)
{
	project_data_set_complete(project, client);
}

void
server_lash_event_save_file(project_t * project, client_t * client)
{
	project_file_complete(project, client);
}

void
server_lash_event_project_add(server_t * server, const char *dir)
{
	project_t *project;
	client_t *client;
	lash_exec_params_t *exec_params;
	lash_list_t *node;
	char server_name[MAXHOSTNAMELEN];
	int err;

	project = project_restore(server, dir);
	if (!project) {
		fprintf(stderr, "%s: could not restore project in dir '%s'\n",
				__FUNCTION__, dir);
		return;
	}

	if (project_name_exists(server->projects, project->name)) {
		const char *name;

		name = server_create_new_project_name(server);
		fprintf(stderr,
				"%s: changing project name for project in dir '%s' to '%s' to avoid clashing with existing project name '%s'\n",
				__FUNCTION__, dir, name, project->name);
		project_set_name(project, name);
	}

	err = gethostname(server_name, MAXHOSTNAMELEN);
	if (err == -1) {
		fprintf(stderr,
				"%s: could not get local host name, using 'localhost' instead: %s\n",
				__FUNCTION__, strerror(errno));
		strcpy(server_name, "localhost");
	}

	server->projects = lash_list_append(server->projects, project);

	server_notify_interfaces(project, NULL, LASH_Project_Add, project->name);
	server_notify_interfaces(project, NULL, LASH_Project_Dir,
							 project->directory);

	for (node = project->lost_clients; node; node = lash_list_next(node)) {
		client = (client_t *) node->data;

		exec_params = lash_exec_params_new();

		lash_exec_params_set_working_dir(exec_params, client->working_dir);
		lash_exec_params_set_server(exec_params, server_name);
		lash_exec_params_set_project(exec_params, project->name);
		lash_exec_params_set_args(exec_params, client->argc,
								  (const char *const *)client->argv);
		exec_params->flags = client->flags;
		uuid_copy(exec_params->id, client->id);

		loader_load(server->loader, exec_params);

		lash_exec_params_destroy(exec_params);

		for (err = 0; err < client->argc; err++)
			free(client->argv[err]);
		free(client->argv);
		client->argv = NULL;
		client->argc = 0;
	}

}

void
server_lash_event_project_dir(project_t * project, client_t * client,
							  const char *dir)
{
	if (dir)
		project_move(project, dir);
	else {
		lash_event_t *event;

		event = lash_event_new_with_type(LASH_Project_Dir);
		lash_event_set_string(event, project->directory);
		lash_event_set_project(event, project->name);

		conn_mgr_send_client_lash_event(project->server->conn_mgr,
										client->conn_id, event);
	}
}

void
server_lash_event_project_name(project_t * project, const char *name)
{
	server_notify_interfaces(project, NULL, LASH_Project_Name, name);
	project_set_name(project, name);
}

void
server_lash_event_client_remove(project_t * project, client_t * client)
{
	project_remove_client(project, client);
}

/* EOF */
