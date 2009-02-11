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

#define _GNU_SOURCE

#include "../config.h"

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
#include <unistd.h>
#include <uuid/uuid.h>
#include <dbus/dbus.h>
#include <jack/jack.h>

#include "project.h"
#include "client.h"
#include "client_dependency.h"
#include "store.h"
#include "file.h"
#include "jack_patch.h"
#include "server.h"
#include "loader.h"
#include "dbus_iface_control.h"
#include "common/safety.h"
#include "common/debug.h"

#ifdef HAVE_JACK_DBUS
# include "jackdbus_mgr.h"
#else
# include "jack_mgr.h"
#endif

#ifdef HAVE_ALSA
#include <alsa/asoundlib.h>
#include "alsa_patch.h"
#endif

static const char *
project_get_client_config_dir(project_t *project,
                              client_t  *client);

project_t *
project_new(void)
{
	project_t *project;

	project = lash_calloc(1, sizeof(project_t));

	INIT_LIST_HEAD(&project->clients);
	INIT_LIST_HEAD(&project->lost_clients);

	return project;
}

static client_t *
project_get_client_by_name(project_t  *project,
                           const char *name)
{
	client_t *client;
	if ((client = client_find_by_name(&project->clients, name))
	    || (client = client_find_by_name(&project->lost_clients, name)))
		return client;
	return NULL;
}

static char *
project_get_unique_client_name(project_t *project,
                               client_t  *client)
{
	uint8_t i;
	char *str = lash_malloc(1, strlen(client->class) + 4);

	lash_debug("Creating a unique name for client %s based on its class",
	           client->id_str);

	if (!project_get_client_by_name(project, client->class)) {
		strcpy(str, client->class);
		return str;
	}

	for (i = 1; i < 100; ++i) {
		sprintf(str, "%s %02u", client->class, i);
		if (!project_get_client_by_name(project, str)) {
			return str;
		}
	}

	lash_error("Could not create a unique name for client %s. Do you have "
	           "100 clients of class %s open?", client->id_str,
	           client->class);

	lash_free(&str);

	return NULL;
}

client_t *
project_get_client_by_id(struct list_head *client_list,
                         uuid_t            id)
{
	struct list_head *node;
	client_t *client;

	list_for_each(node, client_list) {
		client = list_entry(node, client_t, siblings);

		if (uuid_compare(client->id, id) == 0)
			return client;
	}

	return NULL;
}

/* Set modified_status to new_status, emit signal if status changed */
void
project_set_modified_status(project_t *project,
                            bool       new_status)
{
	if (project->modified_status == new_status)
		return;

	dbus_bool_t value = new_status;
	project->modified_status = new_status;

	signal_new_valist(g_server->dbus_service,
	                  "/", "org.nongnu.LASH.Control",
	                  "ProjectModifiedStatusChanged",
	                  DBUS_TYPE_STRING, &project->name,
	                  DBUS_TYPE_BOOLEAN, &value,
	                  DBUS_TYPE_INVALID);
}

void
project_name_client(project_t  *project,
                    client_t   *client)
{
	char *name;

	if (!(name = project_get_unique_client_name(project, client))) {
		lash_error("Cannot get unique client name, using empty string");
		name = lash_strdup("");
	}

	if (client->name)
		free(client->name);
	client->name = name;

	lash_info("Client %s set its name to '%s'", client->id_str, client->name);
}

void
project_new_client(project_t *project,
                   client_t  *client)
{
	uuid_generate(client->id);
	uuid_unparse(client->id, client->id_str);

	/* Set the client's data path */
	lash_strset(&client->data_path,
	            project_get_client_dir(project, client));

	lash_debug("New client now has id %s", client->id_str);

	if (CLIENT_CONFIG_DATA_SET(client))
		client_store_open(client,
		                  project_get_client_config_dir(project,
		                                                client));

	client->project = project;
	list_add(&client->siblings, &project->clients);

	lash_info("Added client %s of class '%s' to project '%s'",
	          client->id_str, client->class, project->name);

	lash_create_dir(client->data_path);

	/* Give the client a unique name */
	project_name_client(project, client);

	project_set_modified_status(project, true);

	// TODO: Swap 2nd and 3rd parameter of this signal
	lashd_dbus_signal_emit_client_appeared(client->id_str, project->name,
	                                       client->name);
}

void
project_satisfy_client_dependency(project_t *project,
                                  client_t  *client)
{
	struct list_head *node;
	client_t *lost_client;

	list_for_each(node, &project->lost_clients) {
		lost_client = list_entry(node, client_t, siblings);

		if (!list_empty(&lost_client->unsatisfied_deps)) {
			client_dependency_remove(&lost_client->unsatisfied_deps,
			                         client->id);

			if (list_empty(&lost_client->unsatisfied_deps)) {
				lash_debug("Dependencies for client '%s' "
				           "are now satisfied",
				           lost_client->name);
				project_launch_client(project, lost_client);
			}
		}
	}
}

void
project_load_file(project_t *project,
                  client_t  *client)
{
	lash_info("Requesting client '%s' to load data from disk",
	           client_get_identity(client));

	client->pending_task = (++g_server->task_iter);
	client->task_type = LASH_Restore_File;
	client->task_progress = 0;

	method_call_new_valist(g_server->dbus_service, NULL,
	                       method_default_handler, false,
	                       client->dbus_name,
	                       "/org/nongnu/LASH/Client",
	                       "org.nongnu.LASH.Client",
	                       "Load",
	                       DBUS_TYPE_UINT64, &client->pending_task,
	                       DBUS_TYPE_STRING, &client->data_path,
	                       DBUS_TYPE_INVALID);
}

/* Send a LoadDataSet method call to the client */
void
project_load_data_set(project_t *project,
                      client_t  *client)
{
	if (!client_store_open(client, project_get_client_config_dir(project,
	                                                             client))) {
		lash_error("Could not open client's store; "
		           "not sending data set");
		return;
	}

	lash_info("Sending client '%s' its data set",
	           client_get_identity(client));

	if (list_empty(&client->store->keys)) {
		lash_debug("No data found in store");
		return;
	}

	method_msg_t new_call;
	DBusMessageIter iter, array_iter;
	dbus_uint64_t task_id;

	if (!method_call_init(&new_call, g_server->dbus_service,
	                      NULL,
	                      method_default_handler,
	                      client->dbus_name,
	                      "/org/nongnu/LASH/Client",
	                      "org.nongnu.LASH.Client",
	                      "LoadDataSet")) {
		lash_error("Failed to initialise LoadDataSet method call");
		return;
	}

	dbus_message_iter_init_append(new_call.message, &iter);

	task_id = (++g_server->task_iter);

	if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT64, &task_id)) {
		lash_error("Failed to write task ID");
		goto fail;
	}

	if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &array_iter)) {
		lash_error("Failed to open config array container");
		goto fail;
	}

	if (!store_create_config_array(client->store, &array_iter)) {
		lash_error("Failed to create config array");
		goto fail;
	}

	if (!dbus_message_iter_close_container(&iter, &array_iter)) {
		lash_error("Failed to close config array container");
		goto fail;
	}

	if (!method_send(&new_call, false)) {
		lash_error("Failed to send LoadDataSet method call");
		/* method_send has unref'd the message for us */
		return;
	}

	client->pending_task = task_id;
	client->task_type = LASH_Restore_Data_Set;
	client->task_progress = 0;

	return;

fail:
	dbus_message_unref(new_call.message);
}

void
project_launch_client(project_t *project,
                      client_t  *client)
{
	lash_debug("Launching client %s (flags 0x%08X)", client->id_str, client->flags);
	loader_execute(client, client->flags & LASH_Terminal);
}

client_t *
project_find_lost_client_by_class(project_t  *project,
                                  const char *class)
{
	struct list_head *node;
	list_for_each (node, &project->lost_clients) {
		client_t *client = list_entry(node, client_t, siblings);
		if (strcmp(client->class, class) == 0)
			return client;
	}
	return NULL;
}

const char *
project_get_client_dir(project_t *project,
                       client_t  *client)
{
	get_store_and_return_fqn(lash_get_fqn(project->directory,
	                                      PROJECT_ID_DIR),
	                         client->id_str);
}

static const char *
project_get_client_config_dir(project_t *project,
                              client_t  *client)
{
	get_store_and_return_fqn(project_get_client_dir(project, client),
	                         PROJECT_CONFIG_DIR);
}

// TODO: - Needs to check for errors so that we can
//         report failures back to the control app
//       - Needs to be less fugly
void
project_move(project_t  *project,
             const char *new_dir)
{
	struct list_head *node;
	client_t *client;
	DIR *dir = NULL;

	if (!new_dir || !new_dir[0]
	    || strcmp(new_dir, project->directory) == 0)
		return;

	/* Check to be sure directory is acceptable
	 * FIXME: thorough enough error checking? */
	dir = opendir(new_dir);

	if (dir || errno == ENOTDIR) {
		lash_error("Cannot move project to %s: Target exists",
		           new_dir);
		closedir(dir);
		return;
	} else if (errno == ENOENT) {
		/* This is what we want... */
		/*printf("Directory %s does not exist, creating.\n", new_dir);*/
	}

	/* close all the clients' stores */
	list_for_each (node, &project->clients) {
		client = list_entry(node, client_t, siblings);
		client_store_close(client);
		/* FIXME: check for errors */
	}

	/* move the directory */

	if (rename(project->directory, new_dir)) {
		lash_error("Cannot move project to %s: %s",
		           new_dir, strerror(errno));
	} else {
		lash_info("Project '%s' moved from '%s' to '%s'",
		          project->name, project->directory, new_dir);

		lash_strset(&project->directory, new_dir);
		lashd_dbus_signal_emit_project_path_changed(project->name, new_dir);

		/* open all the clients' stores again */
		list_for_each (node, &project->clients) {
			client = list_entry(node, client_t, siblings);
			client_store_open(client,
			                  project_get_client_config_dir(project,
			                                                client));
			/* FIXME: check for errors */
		}
	}
}

#if 0
/* This is the handler to use when calling a client's Save method.
   At the moment it isn't used but it will be, so don't delete. */
static void
project_save_client_handler(DBusPendingCall *pending,
                            void            *data)
{
	DBusMessage *msg = dbus_pending_call_steal_reply(pending);
	client_t *client = data;

	if (msg) {
		const char *err_str;

		if (!method_return_verify(msg, &err_str)) {
			lash_error("Client save request failed: %s", err_str);
			client->pending_task = 0;
			client->task_type = 0;
			client->task_progress = 0;
		} else {
			lash_debug("Client save request succeeded");
			++client->project->client_tasks_total;
			/* Now we can start waiting for the client to send
			   a success or failure report of the save.
			   We will not reset pending_task until the report
			   arrives. */
		}
		dbus_message_unref(msg);
	} else {
		lash_error("Cannot get method return from pending call");
		client->pending_task = 0;
		client->task_type = 0;
		client->task_progress = 0;
	}

	dbus_pending_call_unref(pending);
}
#endif

static __inline__ void
project_save_clients(project_t *project)
{
	struct list_head *node;
	client_t *client;

	list_for_each (node, &project->clients) {
		client = list_entry(node, client_t, siblings);
		if (client->pending_task) {
			lash_error("Clients have pending tasks, not sending "
			           "save request");
			return;
		}
	}

	project->task_type = LASH_TASK_SAVE;
	project->client_tasks_total = 0;
	project->client_tasks_progress = 0;
	++g_server->task_iter;

	lash_debug("Signaling all clients of project '%s' to save (task %llu)",
	           project->name, g_server->task_iter);
	signal_new_valist(g_server->dbus_service,
	                  "/", "org.nongnu.LASH.Server", "Save",
	                  DBUS_TYPE_STRING, &project->name,
	                  DBUS_TYPE_UINT64, &g_server->task_iter,
	                  DBUS_TYPE_INVALID);

	list_for_each (node, &project->clients) {
		client = list_entry(node, client_t, siblings);

		if (CLIENT_HAS_INTERNAL_STATE(client))
		{
			client->pending_task = g_server->task_iter;
			client->task_type = (CLIENT_CONFIG_FILE(client)) ? LASH_Save_File : LASH_Save_Data_Set;
			client->task_progress = 0;
			++project->client_tasks_total;
		}
	}

	project->client_tasks_pending = project->client_tasks_total;
	if (project->client_tasks_total == 0)
	{
		project->task_type = 0;
	}
}

static void
project_create_client_jack_patch_xml(project_t  *project,
                                     client_t   *client,
                                     xmlNodePtr  clientxml)
{
	xmlNodePtr jack_patch_set;
	struct list_head *node, *next;
	jack_patch_t *patch;

	LIST_HEAD(patches);

#ifdef HAVE_JACK_DBUS
	if (!lashd_jackdbus_mgr_get_client_patches(g_server->jackdbus_mgr,
	                                           client->id, &patches)) {
#else
	jack_mgr_lock(g_server->jack_mgr);
	jack_mgr_get_client_patches(g_server->jack_mgr, client->id, &patches);
	jack_mgr_unlock(g_server->jack_mgr);
	if (list_empty(&patches)) {
#endif
		lash_info("client '%s' has no patches to save", client_get_identity(client));
		return;
	}

	jack_patch_set =
		xmlNewChild(clientxml, NULL, BAD_CAST "jack_patch_set", NULL);

	list_for_each_safe (node, next, &patches) {
		patch = list_entry(node, jack_patch_t, siblings);

		lash_info("Saving client '%s' patch %s:%s -> %s:%s", client_get_identity(client), patch->src_client, patch->src_port, patch->dest_client, patch->dest_port);
		jack_patch_create_xml(patch, jack_patch_set);

		jack_patch_destroy(patch);
	}
}

#ifdef HAVE_ALSA
static void
project_create_client_alsa_patch_xml(project_t  *project,
                                     client_t   *client,
                                     xmlNodePtr  clientxml)
{
	xmlNodePtr alsa_patch_set;
	struct list_head *node, *next;
	alsa_patch_t *patch;

	LIST_HEAD(patches);

	alsa_mgr_lock(g_server->alsa_mgr);
	alsa_mgr_get_client_patches(g_server->alsa_mgr, client->id, &patches);
	alsa_mgr_unlock(g_server->alsa_mgr);
	if (list_empty(&patches))
		return;

	alsa_patch_set =
		xmlNewChild(clientxml, NULL, BAD_CAST "alsa_patch_set", NULL);

	list_for_each_safe (node, next, &patches) {
		patch = list_entry(node, alsa_patch_t, siblings);
		alsa_patch_create_xml(patch, alsa_patch_set);
		alsa_patch_destroy(patch);
	}
}
#endif

static void
project_create_client_dependencies_xml(client_t   *client,
                                       xmlNodePtr  parent)
{
	struct list_head *node;
	client_dependency_t *dep;
	xmlNodePtr deps_xml;
	char id_str[37];

	deps_xml = xmlNewChild(parent, NULL, BAD_CAST "dependencies", NULL);

	list_for_each (node, &client->dependencies) {
		dep = list_entry(node, client_dependency_t, siblings);
		uuid_unparse(dep->client_id, id_str);
		xmlNewChild(deps_xml, NULL, BAD_CAST "id",
		            BAD_CAST id_str);
	}
}

static xmlDocPtr
project_create_xml(project_t *project)
{
	xmlDocPtr doc;
	xmlNodePtr lash_project, clientxml, arg_set;
	struct list_head *node;
	client_t *client;
	char num[16];
	int i;

	doc = xmlNewDoc(BAD_CAST XML_DEFAULT_VERSION);

	/* DTD */
	xmlCreateIntSubset(doc, BAD_CAST "lash_project", NULL,
	                   BAD_CAST "http://www.nongnu.org/lash/lash-project-1.0.dtd");

	/* Root node */
	lash_project = xmlNewDocNode(doc, NULL, BAD_CAST "lash_project", NULL);
	xmlAddChild((xmlNodePtr) doc, lash_project);

	xmlNewChild(lash_project, NULL, BAD_CAST "version",
	            BAD_CAST PROJECT_XML_VERSION);

	xmlNewChild(lash_project, NULL, BAD_CAST "name",
	            BAD_CAST project->name);

	xmlNewChild(lash_project, NULL, BAD_CAST "description",
	            BAD_CAST project->description);

	list_for_each (node, &project->clients) {
		client = list_entry(node, client_t, siblings);

		clientxml = xmlNewChild(lash_project, NULL, BAD_CAST "client",
		                        NULL);

		xmlNewChild(clientxml, NULL, BAD_CAST "class",
		            BAD_CAST client->class);

		xmlNewChild(clientxml, NULL, BAD_CAST "id",
		            BAD_CAST client->id_str);

		xmlNewChild(clientxml, NULL, BAD_CAST "name",
		            BAD_CAST client->name);

		sprintf(num, "%d", client->flags);
		xmlNewChild(clientxml, NULL, BAD_CAST "flags", BAD_CAST num);

		xmlNewChild(clientxml, NULL, BAD_CAST "working_directory",
		            BAD_CAST client->working_dir);

		arg_set = xmlNewChild(clientxml, NULL, BAD_CAST "arg_set", NULL);
		for (i = 0; i < client->argc; i++)
			xmlNewChild(arg_set, NULL, BAD_CAST "arg",
			            BAD_CAST client->argv[i]);

		if (client->jack_client_name)
			project_create_client_jack_patch_xml(project, client,
			                                     clientxml);
		else
		{
			lash_info("client '%s' (%p) has no jack client name", client_get_identity(client), client);
		}
#ifdef HAVE_ALSA
		if (client->alsa_client_id)
			project_create_client_alsa_patch_xml(project, client,
			                                     clientxml);
#endif
		if (!list_empty(&client->dependencies))
			project_create_client_dependencies_xml(client,
			                                       clientxml);
	}

	return doc;
}

static __inline__ bool
project_write_info(project_t *project)
{
	xmlDocPtr doc;
	const char *filename;

	doc = project_create_xml(project);

	if (project->doc)
		xmlFree(project->doc);
	project->doc = doc;

	filename = lash_get_fqn(project->directory, PROJECT_INFO_FILE);

	if (xmlSaveFormatFile(filename, doc, 1) == -1) {
		lash_error("Cannot save project data to file %s: %s",
		           filename, strerror(errno));
		return false;
	}

	return true;
}

static void
project_clear_lost_clients(project_t *project)
{
	struct list_head *head, *node;
	client_t *client;

	head = &project->lost_clients;
	node = head->next;

	while (node != head) {
		client = list_entry(node, client_t, siblings);
		node = node->next;

		if (lash_dir_exists(client->data_path))
			lash_remove_dir(client->data_path);

		list_del(&client->siblings);
		client_destroy(client);
	}
}

static __inline__ void
project_load_notes(project_t *project)
{
	char *filename;
	char *data;

	filename = lash_dup_fqn(project->directory, PROJECT_NOTES_FILE);

	if (!lash_read_text_file((const char *) filename, &data))
		lash_error("Failed to read project notes from '%s'", filename);
	else {
		project->notes = data;
		lash_debug("Project notes: \"%s\"", data);
	}

	free(filename);
}

static __inline__ bool
project_save_notes(project_t *project)
{
	char *filename;
	FILE *file;
	size_t size;
	bool ret;

	ret = true;

	filename = lash_dup_fqn(project->directory, PROJECT_NOTES_FILE);

	file = fopen(filename, "w");
	if (!file) {
		lash_error("Failed to open '%s' for writing: %s",
		           filename, strerror(errno));
		ret = false;
		goto exit;
	}

	if (project->notes) {
		size = strlen(project->notes);

		if (fwrite(project->notes, size, 1, file) != 1) {
			lash_error("Failed to write %ld bytes of data "
			           "to file '%s'", size, filename);
			ret = false;
		}
	}

	fclose(file);

exit:
	free(filename);
	return ret;
}

static
__inline__
bool
project_update_last_modify_time(
	project_t * project_ptr)
{
	struct stat st;

	if (stat(project_ptr->directory, &st) != 0)
	{
		lash_error("failed to stat '%s', error is %d", project_ptr->directory, errno);
		return false;
	}

	project_ptr->last_modify_time = st.st_mtime;
	return true;
}

static
__inline__
void
project_clients_save_complete(
	project_t * project_ptr)
{
	bool success;

	success = project_write_info(project_ptr);

	lashd_dbus_signal_emit_project_saved(project_ptr->name);
	project_update_last_modify_time(project_ptr);

	if (success)
	{
		lash_info("Project '%s' saved.", project_ptr->name);
	}
	else
	{
		lash_error("Error writing info file for project '%s'", project_ptr->name);
	}

	project_set_modified_status(project_ptr, false);

	/* Reset the controllers' progress display */
	lashd_dbus_signal_emit_progress(0);
}

static
__inline__
void
project_loaded(
	project_t * project_ptr)
{
	lashd_dbus_signal_emit_project_loaded(project_ptr->name);
	lash_info("Project '%s' loaded.", project_ptr->name);

	project_set_modified_status(project_ptr, false);

	/* Reset the controllers' progress display */
	lashd_dbus_signal_emit_progress(0);
}

void
project_save(project_t *project)
{
	if (project->task_type) {
		lash_error("Another task (type %d) is in progress, cannot save right now", project->task_type);
		lash_error("%" PRIu32 " pending client tasks.", project->client_tasks_pending);
		return;
	}

	lash_info("Saving project '%s' ...", project->name);

	/* Initialise the controllers' progress display */
	lashd_dbus_signal_emit_progress(0);

	if (list_empty(&project->siblings_all)) {
		/* this is first save for new project, add it to available for loading list */
		list_add_tail(&project->siblings_all, &g_server->all_projects);
	}

	if (!lash_dir_exists(project->directory)) {
		lash_create_dir(project->directory);
		lash_info("Created project directory %s", project->directory);
	}

	project_save_clients(project);

#ifdef HAVE_JACK_DBUS
	lashd_jackdbus_mgr_get_graph(g_server->jackdbus_mgr);
#endif

	if (!project_save_notes(project))
		lash_error("Error writing notes file for project '%s'", project->name);

	project_clear_lost_clients(project);

	/* in project_save_clients we tell clients to save and save completes when there are no pending tasks,
	   as detected in project_client_task_completed()
	   However, when there are not clients with internal state, project_client_task_completed() is not called at all.
	   OTOH save is complete at this point.
	*/
	if (project->client_tasks_total == 0) /* check for project with stateless clients only */
	{
		project_clients_save_complete(project);
	}
}

bool
project_load(project_t *project)
{
	xmlNodePtr projectnode, xmlnode;
	xmlChar *content = NULL;

	for (projectnode = project->doc->children; projectnode;
	     projectnode = projectnode->next) {
		if (projectnode->type == XML_ELEMENT_NODE
		    && strcmp((const char *) projectnode->name, "lash_project") == 0)
			break;
	}

	if (!projectnode) {
		lash_error("No root node in project XML document");
		return false;
	}

	for (xmlnode = projectnode->children; xmlnode;
	     xmlnode = xmlnode->next) {
		if (strcmp((const char *) xmlnode->name, "version") == 0) {
			/* FIXME: check version */
		} else if (strcmp((const char *) xmlnode->name, "name") == 0) {
			content = xmlNodeGetContent(xmlnode);
			lash_strset(&project->name, (const char *) content);
			xmlFree(content);
		} else if (strcmp((const char *) xmlnode->name, "client") == 0) {
			client_t *client;

			client = client_new();
			client->project = project;
			client_parse_xml(project, client, xmlnode);

			// TODO: reject client if its data doesn't contain
			//       the basic stuff (id, class, etc.)

			list_add(&client->siblings, &project->lost_clients);
		}
	}

	if (!project->name) {
		lash_error("No name node in project XML document");
		project_unload(project);
		return false;
	}

	project_load_notes(project);

#ifdef LASH_DEBUG
	{
		struct list_head *node, *node2;
		client_t *client;
		int i;

		lash_debug("Restored project with:");
		lash_debug("  directory: '%s'", project->directory);
		lash_debug("  name:      '%s'", project->name);
		lash_debug("  clients:");
		list_for_each (node, &project->lost_clients) {
			client = list_entry(node, client_t, siblings);

			lash_debug("  ------");
			lash_debug("    id:          '%s'", client->id_str);
			lash_debug("    name:        '%s'", client->name);
			lash_debug("    working dir: '%s'", client->working_dir);
			lash_debug("    flags:       %d", client->flags);
			lash_debug("    argc:        %d", client->argc);
			lash_debug("    args:");
			for (i = 0; i < client->argc; i++) {
				lash_debug("      %d: '%s'", i, client->argv[i]);
			}
#ifdef HAVE_ALSA
			if (!list_empty(&client->alsa_patches)) {
				lash_debug("    alsa patches:");
				list_for_each (node2, &client->alsa_patches) {
					lash_debug("      %s",
					           alsa_patch_get_desc(list_entry(node2, alsa_patch_t, siblings)));
				}
			} else
				lash_debug("    no alsa patches");
#endif

			if (!list_empty(&client->jack_patches)) {
				lash_debug("    jack patches:");
				list_for_each (node2, &client->jack_patches) {
					jack_patch_t *p = list_entry(node2, jack_patch_t, siblings);
					lash_debug("      '%s' -> '%s'", p->src_desc, p->dest_desc);
				}
			} else
				lash_debug("    no jack patches");
		}
	}
#endif

	project->modified_status = false;

	return true;
}

project_t *
project_new_from_disk(const char *parent_dir,
                      const char *project_dir)
{
	project_t *project;
	char *filename = NULL;
	xmlNodePtr projectnode, xmlnode;
	xmlChar *content = NULL;

	project = project_new();

	INIT_LIST_HEAD(&project->siblings_loaded);

	lash_strset(&project->name, project_dir);

	project->directory = lash_dup_fqn(parent_dir, project_dir);

	if (!project_update_last_modify_time(project))
	{
		goto fail;
	}

	filename = lash_dup_fqn(project->directory, PROJECT_INFO_FILE);
	if (!lash_file_exists(filename)) {
		lash_error("Project '%s' has no info file", project->name);
		goto fail;
	}

	project->doc = xmlParseFile(filename);
	if (project->doc == NULL) {
		lash_error("Could not parse file %s", filename);
		goto fail;
	}

	lash_free(&filename);

	for (projectnode = project->doc->children; projectnode;
	     projectnode = projectnode->next) {
		if (projectnode->type == XML_ELEMENT_NODE
		    && strcmp((const char *) projectnode->name, "lash_project") == 0)
			break;
	}

	if (!projectnode) {
		lash_error("No root node in project XML document");
		goto fail;
	}

	for (xmlnode = projectnode->children; xmlnode;
	     xmlnode = xmlnode->next) {
		if (strcmp((const char *) xmlnode->name, "version") == 0) {
			/* FIXME: check version */
		} else if (strcmp((const char *) xmlnode->name, "name") == 0) {
			content = xmlNodeGetContent(xmlnode);
			lash_strset(&project->name, (const char *) content);
			xmlFree(content);
		} else if (strcmp((const char *) xmlnode->name, "description") == 0) {
			content = xmlNodeGetContent(xmlnode);
			lash_strset(&project->description, (const char *)content);
			xmlFree(content);
		}
	}

	if (!project->name) {
		lash_error("No name node in project XML document");
		goto fail;
	}

	return project;

fail:
	lash_free(&filename);
	project_destroy(project);
	return NULL;
}

bool
project_is_loaded(project_t *project)
{
	return !list_empty(&project->siblings_loaded);
}

void
project_lose_client(project_t *project,
                    client_t  *client)
{
	LIST_HEAD(patches);

	lash_info("Losing client '%s'", client_get_identity(client));

	if (CLIENT_CONFIG_DATA_SET(client) && client->store) {
		if (!list_empty(&client->store->keys))
			store_write(client->store);
		else if (lash_dir_exists(client->store->dir))
			lash_remove_dir(client->store->dir);
	}

	if (CLIENT_CONFIG_DATA_SET(client) || CLIENT_CONFIG_FILE(client)) {
		const char *dir = (const char *) client->data_path;
		if (lash_dir_exists(dir) && lash_dir_empty(dir))
			lash_remove_dir(dir);

		dir = lash_get_fqn(project->directory, PROJECT_ID_DIR);
		if (lash_dir_exists(dir) && lash_dir_empty(dir))
			lash_remove_dir(dir);
	}

	if (client->jack_client_name) {
#ifdef HAVE_JACK_DBUS
		if (lashd_jackdbus_mgr_remove_client(g_server->jackdbus_mgr,
		                                     client->id, &patches)) {
#else
		jack_mgr_lock(g_server->jack_mgr);
		jack_mgr_remove_client(g_server->jack_mgr, client->id, &patches);
		jack_mgr_unlock(g_server->jack_mgr);
		if (!list_empty(&patches)) {
#endif
			list_splice(&patches, &client->jack_patches);
#ifdef LASH_DEBUG
			lash_debug("Backed-up JACK patches:");
			jack_patch_list(&client->jack_patches);
#endif
		}
	}

#ifdef HAVE_ALSA
	if (client->alsa_client_id) {
		alsa_mgr_lock(g_server->alsa_mgr);
		INIT_LIST_HEAD(&patches);
		alsa_mgr_remove_client(g_server->alsa_mgr, client->id, &patches);
		alsa_mgr_unlock(g_server->alsa_mgr);
		if (!list_empty(&patches))
			list_splice(&patches, &client->alsa_patches);
	}
#endif

	/* Pid is only stored for clients who were recently launched so that
	   lashd can tell launched clients from recovering ones. All lost
	   clients must have valid project pointers. */
	client->pid = 0;
	client->project = project;

	list_add(&client->siblings, &project->lost_clients);
	project_set_modified_status(project, true);
	lashd_dbus_signal_emit_client_disappeared(client->id_str, project->name);
}

void
project_unload(project_t *project)
{
	struct list_head *node, *next, *node2, *next2;
	client_t *client;

	LIST_HEAD(patches);

	list_del_init(&project->siblings_loaded);

	lash_debug("Signaling all clients of project '%s' to quit",
	           project->name);

	signal_new_single(g_server->dbus_service,
	                  "/", "org.nongnu.LASH.Server", "Quit",
	                  DBUS_TYPE_STRING, &project->name);

	list_for_each_safe (node, next, &project->clients) {
		client = list_entry(node, client_t, siblings);

		if (client->jack_client_name) {
#ifdef HAVE_JACK_DBUS
			lashd_jackdbus_mgr_remove_client(g_server->jackdbus_mgr,
			                                 client->id, NULL);
#else
			jack_mgr_lock(g_server->jack_mgr);
			jack_mgr_remove_client(g_server->jack_mgr,
			                       client->id, NULL);
			jack_mgr_unlock(g_server->jack_mgr);
#endif
		}

#ifdef HAVE_ALSA
		if (client->alsa_client_id) {
			alsa_mgr_lock(g_server->alsa_mgr);
			alsa_mgr_remove_client(g_server->alsa_mgr, client->id, NULL);
			alsa_mgr_unlock(g_server->alsa_mgr);
		}
#endif

		list_del(&client->siblings);
		client_destroy(client);
	}

	list_for_each_safe (node, next, &project->lost_clients) {
		client = list_entry(node, client_t, siblings);
		list_del(&client->siblings);
		// TODO: Do lost clients also need to have their
		//       JACK and ALSA patches destroyed?
		client_destroy(client);
	}

	if (project->move_on_close)
	{
		char * project_dir;
		char * char_ptr;

		project_dir = lash_dup_fqn(g_server->projects_dir, project->name);

		/* replace bad chars so we dont save in other project dir for example */
		/* i.e. make sure we do rename, not move into other dir */
		char_ptr = project_dir + strlen(g_server->projects_dir) + 1;
		while ((char_ptr = strpbrk(char_ptr, "/")) != NULL)
		{
			*char_ptr = ' ';
			char_ptr++;
		}
		
		project_move(project, project_dir);
		free(project_dir);
	}

	lash_info("Project '%s' unloaded", project->name);

	if (project->doc == NULL && lash_dir_exists(project->directory))
	{
		lash_info("Removing directory '%s' of closed newborn project '%s'", project->directory, project->name);
		lash_remove_dir(project->directory);
	}
}

void
project_destroy(project_t *project)
{
	if (project) {
		lash_debug("Destroying project '%s'", project->name);

		if (project_is_loaded(project))
			project_unload(project);

		if (project->doc)
			xmlFree(project->doc);

		lash_free(&project->name);
		lash_free(&project->directory);
		lash_free(&project->description);
		lash_free(&project->notes);

		// TODO: Free client lists

		free(project);
	}
}

/* Calculate a new overall progress reading for the project's current task */
void
project_client_progress(project_t *project,
                        client_t  *client,
                        uint8_t    percentage)
{
	if (client->task_progress)
		project->client_tasks_progress -= client->task_progress;

	project->client_tasks_progress += percentage;
	client->task_progress = percentage;

	/* Prevent divide by 0 */
	if (!project->client_tasks_total)
		return;

	uint8_t p = project->client_tasks_progress / project->client_tasks_total;
	lashd_dbus_signal_emit_progress(p > 99 ? 99 : p);
}

/* Send the appropriate signal(s) to signify that a client completed a task */
void
project_client_task_completed(project_t *project,
                              client_t  *client)
{
	/* Calculate new progress reading and send Progress signal */
	project_client_progress(project, client, 100);

	/* If the project task is finished emit the appropriate signals */
	if (project->client_tasks_pending && (--project->client_tasks_pending) == 0) {
		project->task_type = 0;
		project->client_tasks_total = 0;
		project->client_tasks_progress = 0;

		/* Send ProjectSaved or ProjectLoaded signal, or return if the task was neither */
		switch (client->task_type) {
		case LASH_Save_Data_Set: case LASH_Save_File:
			project_clients_save_complete(project);
			break;
		case LASH_Restore_File: case LASH_Restore_Data_Set:
			project_loaded(project);
			break;
		default:
			return;
		}
	}
}

void
project_rename(project_t  *project,
               const char *new_name)
{
	char *old_name = project->name;

	if (strcmp(project->name, new_name) != 0)
	{
		project->move_on_close = true;
	}

	project->name = lash_strdup(new_name);

	lashd_dbus_signal_emit_project_name_changed(old_name, new_name);

	project_set_modified_status(project, true);

	lash_free(&old_name);
}

void
project_set_description(project_t  *project,
                        const char *description)
{
	lash_strset(&project->description, description);
	lashd_dbus_signal_emit_project_description_changed(project->name, description);

	project_set_modified_status(project, true);
}

void
project_set_notes(project_t  *project,
                  const char *notes)
{
	lash_strset(&project->notes, notes);
	lashd_dbus_signal_emit_project_notes_changed(project->name, notes);

	project_set_modified_status(project, true);
}

void
project_rename_client(project_t  *project,
                      client_t   *client,
                      const char *name)
{
	if (strcmp(client->name, name) == 0)
		return;

	lash_strset(&client->name, name);
	lashd_dbus_signal_emit_client_name_changed(client->id_str, client->name);
}

void
project_clear_id_dir(project_t *project)
{
	if (project) {
		const char *dir = lash_get_fqn(project->directory,
		                               PROJECT_ID_DIR);
		if (lash_dir_exists(dir))
			lash_remove_dir(dir);
	}
}

/* EOF */
