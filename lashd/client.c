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

#include <lash/internal_headers.h>

#include "config.h"
#include "client.h"
#include "jack_patch.h"
#include "alsa_patch.h"

client_t *
client_new(lash_connect_params_t * params)
{
	client_t *client;

	client = lash_malloc0(sizeof(client_t));

	return client;
}

void
client_destroy(client_t * client)
{
	client_set_name(client, NULL);
	client_set_jack_client_name(client, NULL);
	client_set_class(client, NULL);
	client_set_requested_project(client, NULL);
	client_set_working_dir(client, NULL);
	if (client->store)
		store_destroy(client->store);
	free(client);
}

const char *
client_get_id_str(client_t * client)
{
	static char id_str[37];

	uuid_unparse(client->id, id_str);

	return id_str;
}

const char *
client_get_identity(client_t * client)
{
	static char *identity = NULL;
	static size_t identity_len = 0;

	if (!identity) {
		identity_len = 37;
		identity = lash_malloc(identity_len);
	}

	if (!client->name) {
		uuid_unparse(client->id, identity);
	} else {
		size_t name_len;

		name_len = strlen(client->name) + 1;
		if (name_len > identity_len) {
			identity_len = name_len;
			identity = lash_realloc(identity, identity_len);
		}
		strcpy(identity, client->name);
	}

	return identity;
}

void
client_set_name(client_t * client, const char *name)
{
	set_string_property(client->name, name);
}

void
client_set_conn_id(client_t * client, unsigned long id)
{
	client->conn_id = id;
}

void
client_set_class(client_t * client, const char *class)
{
	set_string_property(client->class, class);
}

void
client_set_working_dir(client_t * client, const char *working_dir)
{
	set_string_property(client->working_dir, working_dir);
}

void
client_set_requested_project(client_t * client, const char *requested_project)
{
	set_string_property(client->requested_project, requested_project);
}

void
client_set_from_connect_params(client_t * client,
							   lash_connect_params_t * params)
{
	uuid_copy(client->id, params->id);
	client->flags = params->flags;
	client_set_class(client, params->class);
	client_set_working_dir(client, params->working_dir);
	client_set_requested_project(client, params->project);
	client->argc = params->argc;
	client->argv = params->argv;

	params->argv = NULL;
}

void
client_set_id(client_t * client, uuid_t id)
{
	uuid_copy(client->id, id);
}

void
client_generate_id(client_t * client)
{
	uuid_generate(client->id);
}

int
client_store_open(client_t * client, const char *dir)
{
	int err;

	if (client->store) {
		store_write(client->store);
		store_destroy(client->store);
	}

	client->store = store_new();
	store_set_dir(client->store, dir);

	err = store_open(client->store);
	if (err)
		fprintf(stderr,
				"%s: WARNING: the store for client '%s' (in directory '%s') could not be read. You should resolve this before saving the project.  You may also wish to restart the client after it has been resolved, as it will not be given any data from the store.\n",
				__FUNCTION__, client_get_id_str(client), client->store->dir);

	return err;
}

lash_config_t *
client_store_get_config(client_t * client, const char *key)
{
	return store_get_config(client->store, key);
}

int
client_store_write(client_t * client)
{
	return store_write(client->store);
}

int
client_store_close(client_t * client)
{
	int err;

	if (!client->store)
		return 0;

	err = store_write(client->store);

	store_destroy(client->store);

	client->store = NULL;

	return err;
}

void
client_set_jack_client_name(client_t * client, const char *name)
{
	set_string_property(client->jack_client_name, name);
}

void
client_set_alsa_client_id(client_t * client, unsigned char id)
{
	client->alsa_client_id = id;
}

void
client_parse_xml(client_t * client, xmlNodePtr parent)
{
	xmlNodePtr xmlnode, argnode;
	xmlChar *content;
	jack_patch_t *jack_patch;
#ifdef HAVE_ALSA
	alsa_patch_t *alsa_patch;
#endif

	LASH_PRINT_DEBUG("parsing client");

	for (xmlnode = parent->children; xmlnode; xmlnode = xmlnode->next) {
		if (strcmp(CAST_BAD(xmlnode->name), "class") == 0) {
			content = xmlNodeGetContent(xmlnode);
			client_set_class(client, CAST_BAD content);
			xmlFree(content);
		} else if (strcmp(CAST_BAD(xmlnode->name), "id") == 0) {
			content = xmlNodeGetContent(xmlnode);
			uuid_parse(CAST_BAD content, client->id);
			xmlFree(content);
		} else if (strcmp(CAST_BAD(xmlnode->name), "flags") == 0) {
			content = xmlNodeGetContent(xmlnode);
			client->flags = strtoul(CAST_BAD content, NULL, 10);
			xmlFree(content);
		} else if (strcmp(CAST_BAD(xmlnode->name), "working_directory") == 0) {
			content = xmlNodeGetContent(xmlnode);
			client_set_working_dir(client, CAST_BAD content);
			xmlFree(content);
		} else if (strcmp(CAST_BAD(xmlnode->name), "arg_set") == 0) {
			for (argnode = xmlnode->children; argnode;
				 argnode = argnode->next)
				if (strcmp(CAST_BAD argnode->name, "arg") == 0) {
					client->argc++;

					content = xmlNodeGetContent(argnode);

					if (!client->argv)
						client->argv = lash_malloc(sizeof(char *));
					else
						client->argv =
							lash_realloc(client->argv,
										 sizeof(char *) * client->argc);

					/* don't like this unsigned/signed char business, so I want a string from strdup */
					client->argv[client->argc - 1] =
						lash_strdup(CAST_BAD content);

					xmlFree(content);
				}
		} else if (strcmp(CAST_BAD(xmlnode->name), "jack_patch_set") == 0) {
			for (argnode = xmlnode->children; argnode;
				 argnode = argnode->next)
				if (strcmp(CAST_BAD argnode->name, "jack_patch") == 0) {
					jack_patch = jack_patch_new();
					jack_patch_parse_xml(jack_patch, argnode);
					client->jack_patches =
						lash_list_append(client->jack_patches, jack_patch);
				}
		} else if (strcmp(CAST_BAD(xmlnode->name), "alsa_patch_set") == 0) {
#ifdef HAVE_ALSA
			for (argnode = xmlnode->children; argnode; argnode = argnode->next) {
				if (strcmp(CAST_BAD argnode->name, "alsa_patch") == 0) {
					alsa_patch = alsa_patch_new();
					alsa_patch_parse_xml(alsa_patch, argnode);
					client->alsa_patches =
						lash_list_append(client->alsa_patches, alsa_patch);
				}
			}
#else
			LASH_PRINT_DEBUG("Warning:  Session contains ALSA information, but LASH"
				" is built without ALSA support.");
#endif
		}
	}

	LASH_DEBUGARGS("parsed client of class %s", client->class);
}

/* EOF */
