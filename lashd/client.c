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

#include "../config.h"

#include <dbus/dbus.h>

#include "common/safety.h"
#include "common/debug.h"

#include "client.h"
#include "server.h"
#include "client_dependency.h"
#include "project.h"
#include "jack_patch.h"
#include "alsa_patch.h"
#include "store.h"
#include "dbus_iface_control.h"
#include "file.h"

client_t *
client_new(void)
{
	client_t *client;

	client = lash_calloc(1, sizeof(client_t));

	INIT_LIST_HEAD(&client->dependencies);
	INIT_LIST_HEAD(&client->unsatisfied_deps);

	return client;
}

void
client_destroy(client_t *client)
{
	if (client) {
		lash_free(&client->name);
		lash_free(&client->jack_client_name);
		lash_free(&client->class);
		lash_free(&client->working_dir);
		lash_free(&client->data_path);
		dbus_free_string_array(client->argv);
		if (client->store)
			store_destroy(client->store);
		client_dependency_remove_all(&client->dependencies);
		client_dependency_remove_all(&client->unsatisfied_deps);
		lash_free(&client);
	}
}

void
client_disconnected(client_t *client)
{
	if (!client)
		return;

	lash_debug("Client '%s' disconnected", client_get_identity(client));

	list_del(&client->siblings);

	if (client->project) {
		project_lose_client(client->project, client);
	} else {
		lashd_dbus_signal_emit_client_disappeared(client->id_str, "");
		client_destroy(client);
	}
}

const char *
client_get_identity(client_t *client)
{
	if (client) {
		return (const char *)
		  (client->name ? client->name : client->id_str);
	}

	return NULL;
}

bool
client_store_open(client_t   *client,
                  const char *dir)
{
	if (client->store) {
		store_write(client->store);
		store_destroy(client->store);
	} else {
		lash_create_dir(dir);
	}

	client->store = store_new();
	lash_strset(&client->store->dir, dir);

	if (!store_open(client->store)) {
		lash_error("Cannot open store for client '%s'",
		           client_get_identity(client));
		return false;
	}

	return true;
}

bool
client_store_close(client_t *client)
{
	bool retval;

	if (!client->store)
		return true;

	retval = store_write(client->store);

	store_destroy(client->store);
	client->store = NULL;

	return retval;
}

void
client_task_progressed(client_t *client,
                       uint8_t   percentage)
{
	project_t *project = client->project;

	//lash_info("%s:%s task progressed to %u", project->name, client->name, (unsigned int)percentage);
	
	if (!project) {
		lash_error("Client's project pointer is NULL");
		return;
	}

	if (percentage == 0) {
		/* Task failed */
		client_task_completed(client, false);
		return;
	} else if (percentage == 255) {
		/* Task completed succesfully */
		client_task_completed(client, true);
		return;
	} else if (percentage <= client->task_progress) {
		/* Discard a percentage too small */
		return;
	} else if (percentage > 99) {
		/* Truncate a percentage too big */
		percentage = 99;
	}

	/* From here on percentage is always 1...99
	   and greater than client->task_progress */

	/* Calculate new project task progress reading, update client's
	   progress reading and send progress notification to controllers */
	project_client_progress(project, client, percentage);
}

void
client_task_completed(client_t *client,
                      bool      was_succesful)
{
	project_t *project = client->project;

	if (!project) {
		lash_error("Client's project pointer is NULL");
		goto end;
	}

	//lash_info("%s:%s task completed %s", project->name, client->name, was_succesful ? "successfully" : "with fail");

	switch (client->task_type) {
	case LASH_Save_Data_Set:
		if (was_succesful) {
			if (store_write(client->store))
				client->flags |= LASH_Saved;
			else
				lash_error("Client '%s' could not write data "
				           "to disk (task %llu)",
				           client_get_identity(client),
				           client->pending_task);
		}
		break;
	case LASH_Save_File:
		if (was_succesful)
			client->flags |= LASH_Saved;
		break;
	case LASH_Restore_File:
	case LASH_Restore_Data_Set:
		if (was_succesful)
			project_satisfy_client_dependency(project, client);
		break;
	default:
		lash_error("Unknown task type %d", client->task_type);
		goto end;
	}

	if (was_succesful)
		lash_debug("Client '%s' completed task %llu",
		           client_get_identity(client), client->pending_task);
	else
		lash_error("Client '%s' failed to complete task %llu",
		           client_get_identity(client), client->pending_task);

	/* Signal progress to controllers */
	project_client_task_completed(project, client);

end:
	client->pending_task = 0;
	client->task_type = 0;
	client->task_progress = 0;
}

void
client_parse_xml(project_t  *project,
                 client_t   *client,
                 xmlNodePtr  parent)
{
	xmlNodePtr xmlnode, argnode;
	xmlChar *content;
	uuid_t id;
	jack_patch_t *jack_patch;
#ifdef HAVE_ALSA
	alsa_patch_t *alsa_patch;
#endif

	lash_debug("Parsing client XML data");

	for (xmlnode = parent->children; xmlnode; xmlnode = xmlnode->next) {
		if (strcmp((const char*) xmlnode->name, "class") == 0) {
			content = xmlNodeGetContent(xmlnode);
			lash_strset(&client->class, (const char *) content);
			xmlFree(content);
		} else if (strcmp((const char*) xmlnode->name, "id") == 0) {
			content = xmlNodeGetContent(xmlnode);
			uuid_parse((char *) content, client->id);
			uuid_unparse(client->id, client->id_str);
			xmlFree(content);
		} else if (strcmp((const char*) xmlnode->name, "name") == 0) {
			content = xmlNodeGetContent(xmlnode);
			lash_strset(&client->name, (const char *) content);
			xmlFree(content);
		} else if (strcmp((const char*) xmlnode->name, "flags") == 0) {
			content = xmlNodeGetContent(xmlnode);
			client->flags = strtoul((const char *) content, NULL, 10);
			xmlFree(content);
		} else if (strcmp((const char*) xmlnode->name,
			          "working_directory") == 0) {
			content = xmlNodeGetContent(xmlnode);
			lash_strset(&client->working_dir, (const char *) content);
			xmlFree(content);
		} else if (strcmp((const char*) xmlnode->name, "arg_set") == 0) {
			for (argnode = xmlnode->children; argnode;
			     argnode = argnode->next)
				if (strcmp((const char *) argnode->name,
				           "arg") == 0) {
					client->argc++;

					content = xmlNodeGetContent(argnode);

					if (!client->argv)
						client->argv =
						  lash_malloc(2, sizeof(char *));
					else
						client->argv =
						  lash_realloc(client->argv,
						               client->argc + 1,
						               sizeof(char *));

					client->argv[client->argc - 1] =
					  lash_strdup((const char *) content);

					/* Make sure that the array is NULL terminated */
					client->argv[client->argc] = NULL;

					xmlFree(content);
				}
		} else if (strcmp((const char*) xmlnode->name,
		                  "jack_patch_set") == 0) {
			for (argnode = xmlnode->children; argnode;
				 argnode = argnode->next)
				if (strcmp((const char *) argnode->name,
				           "jack_patch") == 0) {
					jack_patch = jack_patch_new();
					jack_patch_parse_xml(jack_patch,
					                     argnode);
					client->jack_patches =
					  lash_list_append(client->jack_patches,
					                   jack_patch);
				}
		} else if (strcmp((const char*) xmlnode->name,
		                  "alsa_patch_set") == 0) {
#ifdef HAVE_ALSA
			for (argnode = xmlnode->children; argnode;
			     argnode = argnode->next) {
				if (strcmp((const char *) argnode->name,
				           "alsa_patch") == 0) {
					alsa_patch = alsa_patch_new();
					alsa_patch_parse_xml(alsa_patch,
					                     argnode);
					client->alsa_patches =
					  lash_list_append(client->alsa_patches,
					                   alsa_patch);
				}
			}
#else
			lash_info("Warning: Session contains ALSA information, "
			          "but LASH is built without ALSA support");
#endif
		} else if (strcmp((const char*) xmlnode->name,
		                  "dependencies") == 0) {
			for (argnode = xmlnode->children; argnode;
			     argnode = argnode->next)
				if (strcmp((const char *) argnode->name,
				           "id") == 0) {
					content = xmlNodeGetContent(argnode);
					if (uuid_parse((char *) content,
					               id) == 0) {
						client_dependency_add(&project->lost_clients,
						                      client,
						                      id);
					}
					xmlFree(content);
				}
		}
	}

	lash_debug("Parsed client %s of class %s", client->name, client->class);
}

void
client_maybe_fill_class(
	client_t * client_ptr)
{
	const char * client_name;
#ifdef HAVE_ALSA
	snd_seq_client_info_t * client_info_ptr;

	snd_seq_client_info_alloca(&client_info_ptr);
#endif

	if (client_ptr->class[0] != '\0')
	{
		/* no need to fill class */
		return;
	}

	lash_info("Client class string is empty");
	lash_info("JACK client name '%s'", client_ptr->jack_client_name);
	lash_info("ALSA ID %u", client_ptr->alsa_client_id);

	client_name = NULL;

	if (client_ptr->jack_client_name != NULL &&
	    client_ptr->jack_client_name[0] != '\0')
	{
		client_name = client_ptr->jack_client_name;
	}

#ifdef HAVE_ALSA
	if (client_ptr->alsa_client_id != 0 &&
	    snd_seq_get_any_client_info(g_server->alsa_mgr->seq, client_ptr->alsa_client_id, client_info_ptr) >= 0)
	{
		client_name = snd_seq_client_info_get_name(client_info_ptr);
	}
#endif

	if (client_name != NULL)
	{
		lash_info("Changing client class and name to '%s'", client_name);
		lash_strset(&client_ptr->class, client_name);
		project_rename_client(client_ptr->project, client_ptr, client_name);
	}
}

/* EOF */
