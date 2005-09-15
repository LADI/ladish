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

#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>

#include <jack/jack.h>
#include <alsa/asoundlib.h>
#include <libxml/tree.h>

#include <lash/lash.h>
#include <lash/internal_headers.h>

#include "project.h"
#include "client.h"
#include "store.h"
#include "jack_patch.h"
#include "alsa_patch.h"
#include "globals.h"
#include "server.h"
#include "jack_mgr.h"

#define ID_DIR      ".id"
#define CONFIG_DIR  ".config"
#define INFO_FILE   ".lash_info"
#define PROJECT_XML_VERSION "1.0"

project_t *
project_new(server_t * server)
{
	project_t *project;

	project = lash_malloc0(sizeof(project_t));

	project->server = server;

	return project;
}

void
project_set_directory(project_t * project, const char *directory)
{
	set_string_property(project->directory, directory);
}

void
project_set_name(project_t * project, const char *name)
{
	set_string_property(project->name, name);
}

int
project_name_exists(lash_list_t * projects, const char *name)
{
	for (; projects; projects = lash_list_next(projects)) {
		if (strcmp(name, ((project_t *) projects->data)->name) == 0)
			return 1;
	}

	return 0;
}

client_t *
project_get_client_by_id(project_t * project, uuid_t id)
{
	lash_list_t *list;
	client_t *client;

	for (list = project->clients; list; list = lash_list_next(list)) {
		client = (client_t *) list->data;

		if (uuid_compare(client->id, id) == 0)
			return client;
	}

	return NULL;
}

client_t *
project_get_lost_client_by_id(project_t * project, uuid_t id)
{
	lash_list_t *list;
	client_t *client;

	for (list = project->lost_clients; list; list = lash_list_next(list)) {
		client = (client_t *) list->data;

		if (uuid_compare(client->id, id) == 0)
			return client;
	}

	return NULL;
}

/*************************************
 ************ operations *************
 *************************************/

void
project_new_client(project_t * project, client_t * client)
{
	if (uuid_is_null(client->id))
		client_generate_id(client);

	LASH_DEBUGARGS("client on connection %ld now has id '%s'",
				   client->conn_id, client_get_id_str(client));

	if (CLIENT_CONFIG_DATA_SET(client))
		client_store_open(client,
						  project_get_client_config_dir(project, client));

	project->clients = lash_list_append(project->clients, client);

	printf("Added client %s of class %s to project %s\n",
		   client_get_id_str(client), client->class, project->name);

	lash_create_dir(project_get_client_id_dir(project, client));

	server_notify_interfaces(project, client, LASH_Client_Add, NULL);
}

void
project_name_client(project_t * project, client_t * client, const char *name)
{
	lash_list_t *list;
	client_t *p_client;
	int err;
	int linked = 0;

	LASH_DEBUGARGS("naming client '%s' with name '%s'",
				   client_get_id_str(client), name);

	/* check the client doesn't already have the name */
	if (client->name && strcmp(name, client->name) == 0) {
		LASH_DEBUGARGS("client '%s' already has name '%s'; not setting",
					   client_get_id_str(client), name);
		return;
	}

	/* check the name doesn't already exist */
	for (list = project->clients; list; list = lash_list_next(list)) {
		p_client = (client_t *) list->data;

		if (p_client->name && strcmp(p_client->name, name) == 0) {
			fprintf(stderr,
					"%s: a client '%s' already has the name '%s'; not setting name for client '%s'\n",
					__FUNCTION__, client_get_id_str(p_client), name,
					client_get_id_str(client));
			return;
		}
	}

	if (client->name && (CLIENT_CONFIG_DATA_SET(client) || CLIENT_CONFIG_FILE(client))) {	/* link should already exist */
		char *old_link;

		old_link = lash_strdup(project_get_client_name_dir(project, client));

		client_set_name(client, name);

		LASH_DEBUGARGS("moving old link '%s' to '%s'", old_link,
					   project_get_client_name_dir(project, client));

		err = rename(old_link, project_get_client_name_dir(project, client));
		if (err == -1) {
			fprintf(stderr,
					"%s: could not rename name link for client '%s': %s\n",
					__FUNCTION__, client_get_id_str(client), strerror(errno));
		} else
			linked = 1;

		free(old_link);
	}

	if (!linked &&
		(CLIENT_CONFIG_DATA_SET(client) || CLIENT_CONFIG_FILE(client))) {
		char *id_dir;

		client_set_name(client, name);
		id_dir = lash_malloc(strlen(ID_DIR) + 1 + sizeof(char[37]) + 1);
		sprintf(id_dir, "%s/%s", ID_DIR, client_get_id_str(client));

		LASH_DEBUGARGS("linking id dir '%s' to name dir '%s", id_dir,
					   project_get_client_name_dir(project, client));

		err = symlink(id_dir, project_get_client_name_dir(project, client));
		if (err == -1)
			fprintf(stderr,
					"%s: could not create name link for client '%s': %s\n",
					__FUNCTION__, client_get_id_str(client), strerror(errno));

		free(id_dir);
	} else
		client_set_name(client, name);

	printf("Client %s set its name to '%s'\n", client_get_id_str(client),
		   client->name);
	server_notify_interfaces(project, client, LASH_Client_Name, name);
}

void
project_resume_client(project_t * project, client_t * client,
					  client_t * lost_client)
{
	lash_event_t *event;
	int err;

	/*
	 * get all the necessary data from the lost client
	 */
	client_set_name(client, lost_client->name);
	client->alsa_patches = lost_client->alsa_patches;
	lost_client->alsa_patches = NULL;
	client->jack_patches = lost_client->jack_patches;
	lost_client->jack_patches = NULL;
	client->flags = lost_client->flags;
	uuid_copy(client->id, lost_client->id);

	/*
	 * kill the lost client
	 */
	project->lost_clients =
		lash_list_remove(project->lost_clients, lost_client);
	client_destroy(lost_client);

	LASH_DEBUGARGS("resuming client '%s'", client_get_id_str(client));

	if (client->name) {
		char *name;

		name = lash_strdup(client->name);
		client_set_name(client, NULL);
		project_name_client(project, client, name);
		free(name);
	}

	/*
	 * resume the client
	 */
	if (CLIENT_CONFIG_FILE(client) || CLIENT_CONFIG_DATA_SET(client))
		lash_create_dir(project_get_client_id_dir(project, client));

	/* tell the client to load its files */
	if (CLIENT_CONFIG_FILE(client) && CLIENT_SAVED(client)) {
		event = lash_event_new_with_type(LASH_Restore_File);
		lash_event_set_string(event,
							  project_get_client_id_dir(project, client));
		conn_mgr_send_client_lash_event(project->server->conn_mgr,
										client->conn_id, event);
	}

	if (CLIENT_CONFIG_DATA_SET(client)) {
		err =
			client_store_open(client,
							  project_get_client_config_dir(project, client));
		if (err)
			fprintf(stderr,
					"%s: could not open client's store; not restoring its data set\n",
					__FUNCTION__);
		else {
			if (CLIENT_SAVED(client))
				project_restore_data_set(project, client);
		}
	}

	project->clients = lash_list_append(project->clients, client);

	printf("Resumed client %s of class %s in project %s\n",
		   client_get_id_str(client), client->class, project->name);
	server_notify_interfaces(project, client, LASH_Client_Add, NULL);
}

void
project_add_client(project_t * project, client_t * client)
{
	lash_list_t *list;
	client_t *lost_client;
	int no_client_id;

	if (CLIENT_NO_AUTORESUME(client)) {
		project_new_client(project, client);
		return;
	}

	no_client_id = uuid_is_null(client->id);

	LASH_DEBUGARGS("attempting to resume client on connection %ld",
				   client->conn_id);

	/* try and find a client we can resume */
	list = project->lost_clients;
	while (list) {
		lost_client = (client_t *) list->data;

#ifdef LASH_DEBUG
		{
			char *lost_str;

			lost_str = lash_strdup(client_get_id_str(lost_client));
			LASH_DEBUGARGS
				("checking client '%s','%s' against lost client '%s','%s'",
				 client_get_id_str(client), client->class, lost_str,
				 lost_client->class);
			free(lost_str);
		}
#endif /* LASH_DEBUG */

		if ((no_client_id && strcmp(client->class, lost_client->class) == 0)
			|| uuid_compare(client->id, lost_client->id) == 0) {
			LASH_DEBUGARGS
				("resuming client '%s' of class '%s' with client on connection %ld",
				 client_get_id_str(client), lost_client->class,
				 client->conn_id);
			project_resume_client(project, client, lost_client);
			return;
		}

		list = list->next;
	}

	LASH_DEBUGARGS("could not resume client on connection %ld",
				   client->conn_id);
	project_new_client(project, client);
}

/**************************************
 ************ store stuff *************
 **************************************/

const char *
project_get_client_id_dir(project_t * project, client_t * client)
{
	get_store_and_return_fqn(lash_get_fqn(project->directory, ID_DIR),
							 client_get_id_str(client));
}

const char *
project_get_client_name_dir(project_t * project, client_t * client)
{
	get_store_and_return_fqn(project->directory, client->name);
}

const char *
project_get_client_config_dir(project_t * project, client_t * client)
{
	get_store_and_return_fqn(client->name
							 ? project_get_client_name_dir(project, client)
							 : project_get_client_id_dir(project, client),
							 CONFIG_DIR);
}

const char *
project_get_client_file_dir(project_t * project, client_t * client)
{
	return client->name ? project_get_client_name_dir(project, client)
		: project_get_client_id_dir(project, client);
}

char *
escape_file_name(const char *fn)
{
	char *escfn;
	size_t escfn_size;
	size_t fn_size;
	ptrdiff_t *escchars = NULL;
	size_t escchars_size = 0;
	const char *ptr;
	unsigned int i, j;

	ptr = fn - 1;

	while ((ptr = strpbrk(ptr + 1, " |&;()<>"))) {
		if (!escchars) {
			escchars_size = 1;
			escchars = malloc(escchars_size * sizeof(ptrdiff_t));
		} else {
			escchars_size++;
			escchars = realloc(escchars, escchars_size * sizeof(ptrdiff_t));
		}

		escchars[escchars_size - 1] = ptr - fn;
	}

	if (!escchars)
		return strdup(fn);

	fn_size = strlen(fn);
	escfn_size = fn_size + escchars_size + 1;
	escfn = malloc(escfn_size);
	strncpy(escfn, fn, fn_size + 1);

	for (i = 0; i < escchars_size; i++) {
		for (j = escfn_size - 1; (escfn + j) > (escfn + escchars[i] + i); j--)
			escfn[j] = escfn[j - 1];

		*(escfn + escchars[i] + i) = '\\';
	}

	return escfn;
}

void
project_move(project_t * project, const char *new_dir)
{
	lash_list_t *list = NULL;
	client_t *client = NULL;
	char *esc_proj_dir = NULL;
	char *esc_new_proj_dir = NULL;

	if (strcmp(new_dir, project->directory) == 0)
		return;

	/* Check to be sure directory is acceptable
	 * FIXME: thorough enough error checking? */
	DIR *dir = opendir(new_dir);

	if (dir != NULL) {
		fprintf(stderr,
				"Warning: directory %s exists, files may be overwritten.\n",
				new_dir);
		closedir(dir);
	} else if (dir == NULL && errno == ENOTDIR) {
		fprintf(stderr,
				"Can not move project directory to %s - exists but is not a directory\n",
				new_dir);
		return;
	} else if (dir == NULL && errno == ENOENT) {
		printf("Directory %s does not exist, and will be created.\n",
			   new_dir);
	}

	/* close all the clients' stores */
	for (list = project->clients; list; list = lash_list_next(list)) {
		client = (client_t *) list->data;
		client_store_close(client);
		/* FIXME: check for errors */
	}

	/* move the directory */
	esc_proj_dir = escape_file_name(project->directory);
	esc_new_proj_dir = escape_file_name(new_dir);

	if (rename(esc_proj_dir, esc_new_proj_dir)) {
		fprintf(stderr, "Unable to move project directory to %s (%s)",
				new_dir, strerror(errno));
	} else {
		printf("Project %s moved from %s to %s\n", project->name,
			   project->directory, new_dir);
		project_set_directory(project, new_dir);
		server_notify_interfaces(project, client, LASH_Project_Dir, new_dir);

		/* open all the clients' stores again */
		for (list = project->clients; list; list = lash_list_next(list)) {
			client = (client_t *) list->data;
			client_store_open(client,
							  project_get_client_config_dir(project, client));
			/* FIXME: check for errors */
		}
	}

	free(esc_proj_dir);
	free(esc_new_proj_dir);
}

void
project_restore_data_set(project_t * project, client_t * client)
{

	if (CLIENT_CONFIG_DATA_SET(client)) {
		char *key;
		lash_config_t *config;
		lash_list_t *list;
		lash_event_t *event;

		LASH_DEBUGARGS("restoring data set for client '%s'",
					   client_get_id_str(client));

		list = store_get_keys(client->store);

		/* send the event to the client */
		if (list) {
			event = lash_event_new_with_type(LASH_Restore_Data_Set);
			conn_mgr_send_client_lash_event(project->server->conn_mgr,
											client->conn_id, event);
		}

		for (; list; list = lash_list_next(list)) {
			key = (char *)list->data;

			config = client_store_get_config(client, key);

			if (config)
				conn_mgr_send_client_lash_config(project->server->conn_mgr,
												 client->conn_id, config);
		}
	}
}

void
project_save_clients(project_t * project)
{
	lash_list_t *list;
	client_t *client;
	lash_event_t *event;

	for (list = project->clients; list; list = lash_list_next(list)) {
		client = (client_t *) list->data;

		if (CLIENT_CONFIG_FILE(client)) {
			LASH_DEBUGARGS("telling client %s to save files",
						   client_get_identity(client));

			event = lash_event_new_with_type(LASH_Save_File);

			lash_event_set_string(event,
								  project_get_client_file_dir(project,
															  client));
			conn_mgr_send_client_lash_event(project->server->conn_mgr,
											client->conn_id, event);

			project->saves++;

			client->flags |= LASH_Saved;
		}

		if (CLIENT_CONFIG_DATA_SET(client)) {
			LASH_DEBUGARGS("telling client %s to save data set",
						   client_get_identity(client));

			event = lash_event_new_with_type(LASH_Save_Data_Set);
			conn_mgr_send_client_lash_event(project->server->conn_mgr,
											client->conn_id, event);
			project->saves++;

			client->flags |= LASH_Saved;
		}
	}

	project->pending_saves = project->saves;
}

void
project_create_client_jack_patch_xml(project_t * project, client_t * client,
									 xmlNodePtr clientxml)
{
	xmlNodePtr jack_patch_set;
	lash_list_t *patches, *node;
	jack_patch_t *patch;

	jack_mgr_lock(project->server->jack_mgr);
	patches =
		jack_mgr_get_client_patches(project->server->jack_mgr, client->id);
	jack_mgr_unlock(project->server->jack_mgr);

	if (!patches)
		return;

	jack_patch_set =
		xmlNewChild(clientxml, NULL, BAD_CAST "jack_patch_set", NULL);

	for (node = patches; node; node = lash_list_next(node)) {
		patch = (jack_patch_t *) node->data;

		jack_patch_create_xml(patch, jack_patch_set);

		jack_patch_destroy(patch);
	}

	lash_list_free(patches);
}

void
project_create_client_alsa_patch_xml(project_t * project, client_t * client,
									 xmlNodePtr clientxml)
{
	xmlNodePtr alsa_patch_set;
	lash_list_t *patches, *node;
	alsa_patch_t *patch;

	alsa_mgr_lock(project->server->alsa_mgr);
	patches =
		alsa_mgr_get_client_patches(project->server->alsa_mgr, client->id);
	alsa_mgr_unlock(project->server->alsa_mgr);

	if (!patches)
		return;

	alsa_patch_set =
		xmlNewChild(clientxml, NULL, BAD_CAST "alsa_patch_set", NULL);

	for (node = patches; node; node = lash_list_next(node)) {
		patch = (alsa_patch_t *) node->data;

		alsa_patch_create_xml(patch, alsa_patch_set);

		alsa_patch_destroy(patch);
	}

	lash_list_free(patches);
}

static xmlDocPtr
project_create_xml(project_t * project)
{
	xmlDocPtr doc;
	xmlNodePtr lash_project, clientxml, arg_set;
	lash_list_t *clnode;
	client_t *client;
	char num[16];
	int i;

	doc = xmlNewDoc(BAD_CAST XML_DEFAULT_VERSION);

	/* dtd */
	xmlCreateIntSubset(doc, BAD_CAST "lash_project", NULL,
					   BAD_CAST
					   "http://purge.bash.sh/~rah/lash-project-1.0.dtd");

	/* root node */
	lash_project = xmlNewDocNode(doc, NULL, BAD_CAST "lash_project", NULL);
	xmlAddChild((xmlNodePtr) doc, lash_project);

	xmlNewChild(lash_project, NULL, BAD_CAST "version",
				BAD_CAST PROJECT_XML_VERSION);
	xmlNewChild(lash_project, NULL, BAD_CAST "name", BAD_CAST project->name);

	for (clnode = project->clients; clnode; clnode = lash_list_next(clnode)) {
		client = (client_t *) clnode->data;

		clientxml = xmlNewChild(lash_project, NULL, BAD_CAST "client", NULL);

		xmlNewChild(clientxml, NULL, BAD_CAST "class",
					BAD_CAST client->class);
		xmlNewChild(clientxml, NULL, BAD_CAST "id",
					BAD_CAST client_get_id_str(client));
		sprintf(num, "%d", client->flags);
		xmlNewChild(clientxml, NULL, BAD_CAST "flags", BAD_CAST num);
		xmlNewChild(clientxml, NULL, BAD_CAST "working_directory",
					BAD_CAST client->working_dir);

		arg_set = xmlNewChild(clientxml, NULL, BAD_CAST "arg_set", NULL);
		for (i = 0; i < client->argc; i++)
			xmlNewChild(arg_set, NULL, BAD_CAST "arg",
						BAD_CAST client->argv[i]);

		if (client->jack_client_name)
			project_create_client_jack_patch_xml(project, client, clientxml);

		if (client->alsa_client_id)
			project_create_client_alsa_patch_xml(project, client, clientxml);
	}

	return doc;
}

static int
project_write_info(project_t * project)
{
	xmlDocPtr doc;
	const char *filename;
	int err;

	doc = project_create_xml(project);

	filename = lash_get_fqn(project->directory, INFO_FILE);

	err = xmlSaveFormatFile(filename, doc, 1);
	if (err == -1) {
		fprintf(stderr, "%s: could not save project xml to file %s: %s",
				__FUNCTION__, filename, strerror(errno));
	} else
		err = 0;

	return err;
}

void
project_clear_lost_clients(project_t * project)
{
	lash_list_t *list;
	client_t *client;
	const char *client_dir;

	for (list = project->lost_clients; list; list = lash_list_next(list)) {
		client = (client_t *) list->data;

		client_dir = project_get_client_id_dir(project, client);
		if (lash_dir_exists(client_dir))
			lash_remove_dir(client_dir);

		client_destroy(client);
	}

	lash_list_free(project->lost_clients);
	project->lost_clients = NULL;
}

void
project_save(project_t * project)
{
	char num[16];
	int err;

	if (project->pending_saves) {
		LASH_PRINT_DEBUG("a save is in progress; cannot save at this time");
		return;
	}

	/* initialise the interfaces' progress display */
	sprintf(num, "%d", 0);
	server_notify_interfaces(project, NULL, LASH_Percentage, num);

	project_save_clients(project);

	err = project_write_info(project);
	if (err) {
		fprintf(stderr,
				"%s: error writing info file for project '%s'; aborting save\n",
				__FUNCTION__, project->name);
		return;
	}

	project_clear_lost_clients(project);
}

project_t *
project_new_from_xml(server_t * server, xmlDocPtr doc)
{
	xmlNodePtr projectnode, xmlnode;
	xmlChar *content;

	for (projectnode = doc->children; projectnode;
		 projectnode = projectnode->next)
		if (projectnode->type == XML_ELEMENT_NODE
			&& strcmp(CAST_BAD projectnode->name, "lash_project") == 0)
			break;

	if (!projectnode) {
		LASH_PRINT_DEBUG("no lash_project node in doc");
		return NULL;
	}

	project_t *project;

	project = project_new(server);

	for (xmlnode = projectnode->children; xmlnode; xmlnode = xmlnode->next) {
		if (strcmp(CAST_BAD xmlnode->name, "version") == 0) {
			/* FIXME: check version */
		} else if (strcmp(CAST_BAD xmlnode->name, "name") == 0) {
			content = xmlNodeGetContent(xmlnode);
			project_set_name(project, CAST_BAD content);
			xmlFree(content);
		} else if (strcmp(CAST_BAD xmlnode->name, "client") == 0) {
			client_t *client;

			client = client_new();
			client_parse_xml(client, xmlnode);

			project->lost_clients =
				lash_list_append(project->lost_clients, client);
		}
	}

	if (!project->name) {
		project_destroy(project);
		return NULL;
	}

	return project;
}

project_t *
project_restore(server_t * server, const char *dir)
{
	lash_list_t *list;
	project_t *project;
	const char *filename;
	xmlDocPtr doc;

	LASH_DEBUGARGS("attempting to restore project in dir '%s'", dir);

	/* check if we've already got it open */
	list = server->projects;
	while (list) {
		project = (project_t *) list->data;

		if (strcmp(project->directory, dir) == 0) {
			fprintf(stderr,
					"%s: cannot restore project from directory '%s': a project, '%s', is already active with this directory\n",
					__FUNCTION__, dir, project->name);
			return NULL;
		}

		list = list->next;
	}

	filename = lash_get_fqn(dir, INFO_FILE);

	doc = xmlParseFile(filename);
	if (!doc) {
		fprintf(stderr, "%s: could not parse file %s\n", __FUNCTION__,
				filename);
		return NULL;
	}

	project = project_new_from_xml(server, doc);
	if (!project) {
		fprintf(stderr, "%s: could not recreate project from xml\n",
				__FUNCTION__);
		return NULL;
	}

	project_set_directory(project, dir);

#ifdef LASH_DEBUG
	{
		lash_list_t *client_list;
		client_t *client;
		lash_list_t *list;
		int i;

		LASH_DEBUG("resored project with:");
		LASH_DEBUGARGS("  directory: '%s'", project->directory);
		LASH_DEBUGARGS("  name:      '%s'", project->name);
		LASH_DEBUG("  clients:");
		for (client_list = project->lost_clients; client_list;
			 client_list = client_list->next) {
			client = (client_t *) client_list->data;

			LASH_DEBUG("  ------");
			LASH_DEBUGARGS("    id:          '%s'",
						   client_get_id_str(client));
			LASH_DEBUGARGS("    working dir: '%s'", client->working_dir);
			LASH_DEBUGARGS("    flags:       %d", client->flags);
			LASH_DEBUGARGS("    argc:        %d", client->argc);
			LASH_DEBUG("    args:");
			for (i = 0; i < client->argc; i++) {
				LASH_DEBUGARGS("      %d: '%s'", i, client->argv[i]);
			}
			if (client->alsa_patches) {
				LASH_PRINT_DEBUG("    alsa patches:");
				for (list = client->alsa_patches; list; list = list->next) {
					LASH_DEBUGARGS("      %s",
								   alsa_patch_get_desc((alsa_patch_t *) list->
													   data));
				}
			} else
				LASH_PRINT_DEBUG("    no alsa patches");

			if (client->jack_patches) {
				LASH_PRINT_DEBUG("    jack patches:");
				for (list = client->jack_patches; list; list = list->next) {
					LASH_DEBUGARGS("      %s",
								   jack_patch_get_desc((jack_patch_t *) list->
													   data));
				}
			} else
				LASH_PRINT_DEBUG("    no jack patches");
		}
	}
#endif

	return project;
}

void
project_remove_client(project_t * project, client_t * client)
{
	conn_mgr_send_client_lash_event(project->server->conn_mgr,
									client->conn_id,
									lash_event_new_with_type(LASH_Quit));
}

void
project_lose_client(project_t * project, client_t * client,
					lash_list_t * jack_patches, lash_list_t * alsa_patches)
{
	int i;

	LASH_DEBUGARGS("losing client '%s' from project '%s'",
				   client_get_id_str(client), project->name);
	project->clients = lash_list_remove(project->clients, client);

	if (CLIENT_CONFIG_DATA_SET(client)) {
		store_t *store;

		store = client->store;

		if (store) {
			if (store_get_keys(store))
				store_write(store);
			else if (lash_dir_exists(store->dir))
				lash_remove_dir(store->dir);
		}
	}

	if (CLIENT_CONFIG_DATA_SET(client) || CLIENT_CONFIG_FILE(client)) {
		const char *dir;

		dir = project_get_client_id_dir(project, client);

		if (lash_dir_exists(dir) && lash_dir_empty(dir))
			lash_remove_dir(dir);

		dir = lash_get_fqn(project->directory, ID_DIR);
		if (lash_dir_exists(dir) && lash_dir_empty(dir))
			lash_remove_dir(dir);

		if (client->name)
			unlink(project_get_client_name_dir(project, client));

	}

	client->jack_patches =
		lash_list_concat(client->jack_patches, jack_patches);
	client->alsa_patches =
		lash_list_concat(client->alsa_patches, alsa_patches);
	project->lost_clients = lash_list_append(project->lost_clients, client);

	/* don't need these any more */
	for (i = 0; i < client->argc; i++)
		free(client->argv[i]);
	free(client->argv);
	client->argc = 0;
	client->argv = NULL;

	printf("Client %s removed from project %s\n", client_get_identity(client),
		   project->name);
	server_notify_interfaces(project, client, LASH_Client_Remove, NULL);
}

void
project_destroy(project_t * project)
{
	lash_list_t *node;
	lash_list_t *patches, *pnode;
	client_t *client;
	lash_event_t *lash_event;
	server_event_t *server_event;

	for (node = project->clients; node; node = lash_list_next(node)) {
		client = (client_t *) node->data;

		if (client->jack_client_name) {
			jack_mgr_lock(project->server->jack_mgr);
			patches =
				jack_mgr_remove_client(project->server->jack_mgr, client->id);
			jack_mgr_unlock(project->server->jack_mgr);

			for (pnode = patches; pnode; pnode = lash_list_next(pnode))
				jack_patch_destroy((jack_patch_t *) pnode->data);
			lash_list_free(patches);
		}

		if (client->alsa_client_id) {
			alsa_mgr_lock(project->server->alsa_mgr);
			patches =
				alsa_mgr_remove_client(project->server->alsa_mgr, client->id);
			alsa_mgr_unlock(project->server->alsa_mgr);

			for (pnode = patches; pnode; pnode = lash_list_next(pnode))
				alsa_patch_destroy((alsa_patch_t *) pnode->data);
			lash_list_free(patches);
		}

		/* remove the client name links */
		if (CLIENT_CONFIG_DATA_SET(client) || CLIENT_CONFIG_FILE(client))
			if (client->name)
				unlink(project_get_client_name_dir(project, client));

		lash_event = lash_event_new_with_type(LASH_Quit);
		conn_mgr_send_client_lash_event(project->server->conn_mgr,
										client->conn_id, lash_event);

		server_event = server_event_new();
		server_event->type = Client_Disconnect;
		server_event->conn_id = client->conn_id;
		conn_mgr_send_client_event(project->server->conn_mgr, server_event);

		client_destroy(client);
	}
	lash_list_free(project->clients);

	for (node = project->lost_clients; node; node = lash_list_next(node))
		client_destroy((client_t *) node->data);
	lash_list_free(project->lost_clients);

	printf("Project %s removed\n", project->name);

	project_set_name(project, NULL);

	if (access(lash_get_fqn(project->directory, INFO_FILE), F_OK) != 0)
		lash_remove_dir(project->directory);

	project_set_directory(project, NULL);

	free(project);
}

static void
project_notify_percentage(project_t * project)
{
	char num[16];
	int percentage;

	percentage =
		(((float)(project->saves - project->pending_saves)) /
		 ((float)project->saves)
		 * 100.0);

	if (percentage > 100)
		percentage = 100;

	sprintf(num, "%d", percentage);
	server_notify_interfaces(project, NULL, LASH_Percentage, num);

	if (!project->pending_saves) {
		sprintf(num, "%d", 0);
		server_notify_interfaces(project, NULL, LASH_Percentage, num);

		project->saves = 0;
	}
}

void
project_file_complete(project_t * project, client_t * client)
{
	project->pending_saves--;
	project_notify_percentage(project);
}

void
project_data_set_complete(project_t * project, client_t * client)
{
	int err;

	if (!CLIENT_CONFIG_DATA_SET(client))
		return;

	err = client_store_write(client);
	if (err)
		fprintf(stderr, "%s: could not write client '%s's data to disk!"
				"You should attempt to ascertain why, resolve the situation, and save the project again.\n",
				__FUNCTION__, client_get_id_str(client));

	project->pending_saves--;
	project_notify_percentage(project);
}

/* EOF */
