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

#include <lash/lash.h>
#include <lash/internal_headers.h>
#include <uuid/uuid.h>

#include "jack_patch.h"
#include "jack_mgr_client.h"
#include "jack_mgr.h"

jack_patch_t *
jack_patch_new()
{
	jack_patch_t *patch;

	patch = lash_malloc0(sizeof(jack_patch_t));
	uuid_clear(patch->src_client_id);
	uuid_clear(patch->dest_client_id);
	return patch;
}

jack_patch_t *
jack_patch_dup(const jack_patch_t * other)
{
	jack_patch_t *patch;

	patch = jack_patch_new();

	jack_patch_set_src_client(patch, other->src_client);
	jack_patch_set_src_port(patch, other->src_port);
	jack_patch_set_dest_client(patch, other->dest_client);
	jack_patch_set_dest_port(patch, other->dest_port);

	uuid_copy(patch->src_client_id, other->src_client_id);
	uuid_copy(patch->dest_client_id, other->dest_client_id);

	return patch;
}

void
jack_patch_destroy(jack_patch_t * patch)
{
	jack_patch_set_src_client(patch, NULL);
	jack_patch_set_dest_client(patch, NULL);
	jack_patch_set_src_port(patch, NULL);
	jack_patch_set_dest_port(patch, NULL);
	free(patch);
}

typedef void (*jack_patch_set_str_func) (jack_patch_t *, const char *);

static void
jack_patch_set_client_and_port(jack_patch_t * patch,
							   const char *jack_port_name,
							   jack_patch_set_str_func set_client_func,
							   jack_patch_set_str_func set_port_func)
{
	char *port_name;
	char *ptr;

	port_name = lash_strdup(jack_port_name);

	ptr = strchr(port_name, ':');
	*ptr = '\0';
	ptr++;

	set_client_func(patch, port_name);
	set_port_func(patch, ptr);

	free(port_name);
}

void
jack_patch_set_src(jack_patch_t * patch, const char *src)
{
	jack_patch_set_client_and_port(patch, src, jack_patch_set_src_client,
								   jack_patch_set_src_port);
}

void
jack_patch_set_dest(jack_patch_t * patch, const char *dest)
{
	jack_patch_set_client_and_port(patch, dest, jack_patch_set_dest_client,
								   jack_patch_set_dest_port);
}

void
jack_patch_set_src_client(jack_patch_t * patch, const char *src_client)
{
	set_string_property(patch->src_client, src_client);
}

void
jack_patch_set_src_port(jack_patch_t * patch, const char *src_port)
{
	set_string_property(patch->src_port, src_port);
}

void
jack_patch_set_dest_client(jack_patch_t * patch, const char *dest_client)
{
	set_string_property(patch->dest_client, dest_client);
}

void
jack_patch_set_dest_port(jack_patch_t * patch, const char *dest_port)
{
	set_string_property(patch->dest_port, dest_port);
}

static const char *
jack_patch_recreate_port_name(const char *client, const char *port)
{
	static char *port_name = NULL;
	static size_t port_name_size = 0;
	size_t new_port_name_size;

	new_port_name_size = strlen(client) + 1 + strlen(port) + 1;

	if (port_name_size < new_port_name_size) {
		port_name_size = new_port_name_size;

		if (!port_name)
			port_name = lash_malloc(port_name_size);
		else
			port_name = lash_realloc(port_name, port_name_size);
	}

	sprintf(port_name, "%s:%s", client, port);

	return port_name;
}

const char *
jack_patch_get_src(jack_patch_t * patch)
{
	if (!patch->src_client) {
		char id[37];

		uuid_unparse(patch->src_client_id, id);
		return jack_patch_recreate_port_name(id, patch->src_port);
	} else
		return jack_patch_recreate_port_name(patch->src_client,
											 patch->src_port);
}

const char *
jack_patch_get_dest(jack_patch_t * patch)
{
	if (!patch->dest_client) {
		char id[37];

		uuid_unparse(patch->dest_client_id, id);
		return jack_patch_recreate_port_name(id, patch->dest_port);
	} else
		return jack_patch_recreate_port_name(patch->dest_client,
											 patch->dest_port);
}

const char *
jack_patch_get_desc(jack_patch_t * patch)
{
	static char *desc = NULL;
	static size_t desc_size = 0;
	char *src;
	char *dest;
	size_t new_desc_size;

	src = lash_strdup(jack_patch_get_src(patch));
	dest = lash_strdup(jack_patch_get_dest(patch));

	new_desc_size = strlen(src) + 4 + strlen(dest) + 1;

	if (desc_size < new_desc_size) {
		desc_size = new_desc_size;

		if (!desc)
			desc = lash_malloc(desc_size);
		else
			desc = lash_realloc(desc, desc_size);
	}

	sprintf(desc, "%s -> %s", src, dest);

	free(src);
	free(dest);

	return desc;
}

static void
jack_patch_set_client_id(lash_list_t * jack_mgr_clients,
						 const char *patch_client_name, uuid_t id)
{
	jack_mgr_client_t *client;

	/* find the client */
	for (; jack_mgr_clients;
		 jack_mgr_clients = lash_list_next(jack_mgr_clients)) {
		client = (jack_mgr_client_t *) jack_mgr_clients->data;

		if (strcmp(client->name, patch_client_name) == 0) {
			uuid_copy(id, client->id);
			return;
		}
	}
}

void
jack_patch_set(jack_patch_t * patch, lash_list_t * jack_mgr_clients)
{
	jack_patch_set_client_id(jack_mgr_clients, patch->src_client,
							 patch->src_client_id);
	jack_patch_set_client_id(jack_mgr_clients, patch->dest_client,
							 patch->dest_client_id);
}

static int
jack_patch_unset_client_id(jack_patch_t * patch,
						   lash_list_t * jack_mgr_clients,
						   uuid_t patch_client_id,
						   jack_patch_set_str_func set_client_name_func)
{
	jack_mgr_client_t *client;

	/* find the client */
	for (; jack_mgr_clients;
		 jack_mgr_clients = lash_list_next(jack_mgr_clients)) {
		client = (jack_mgr_client_t *) jack_mgr_clients->data;

		if (uuid_compare(patch_client_id, client->id) == 0)
			break;
	}

	if (!jack_mgr_clients)
		return 1;

	set_client_name_func(patch, client->name);

	return 0;
}

int
jack_patch_unset(jack_patch_t * patch, lash_list_t * jack_mgr_clients)
{
	int err;
	int not_found = 0;

	if (!uuid_is_null(patch->src_client_id)) {
		err = jack_patch_unset_client_id(patch, jack_mgr_clients,
										 patch->src_client_id,
										 jack_patch_set_src_client);
		if (err)
			not_found++;
	}

	if (!uuid_is_null(patch->dest_client_id)) {
		err = jack_patch_unset_client_id(patch, jack_mgr_clients,
										 patch->dest_client_id,
										 jack_patch_set_dest_client);
		if (err)
			not_found++;
	}

	return not_found;
}

void
jack_patch_create_xml(jack_patch_t * patch, xmlNodePtr parent)
{
	xmlNodePtr patchxml;

	patchxml = xmlNewChild(parent, NULL, BAD_CAST "jack_patch", NULL);

	if (uuid_is_null(patch->src_client_id))
		xmlNewChild(patchxml, NULL, BAD_CAST "src_client",
					BAD_CAST patch->src_client);
	else {
		char id[37];

		uuid_unparse(patch->src_client_id, id);
		xmlNewChild(patchxml, NULL, BAD_CAST "src_client_id", BAD_CAST id);
	}

	xmlNewChild(patchxml, NULL, BAD_CAST "src_port",
				BAD_CAST patch->src_port);

	if (uuid_is_null(patch->dest_client_id))
		xmlNewChild(patchxml, NULL, BAD_CAST "dest_client",
					BAD_CAST patch->dest_client);
	else {
		char id[37];

		uuid_unparse(patch->dest_client_id, id);
		xmlNewChild(patchxml, NULL, BAD_CAST "dest_client_id", BAD_CAST id);
	}

	xmlNewChild(patchxml, NULL, BAD_CAST "dest_port",
				BAD_CAST patch->dest_port);
}

void
jack_patch_parse_xml(jack_patch_t * patch, xmlNodePtr parent)
{
	xmlNodePtr xmlnode;
	xmlChar *content;

	for (xmlnode = parent->children; xmlnode; xmlnode = xmlnode->next) {
		if (strcmp(CAST_BAD(xmlnode->name), "src_client") == 0) {
			content = xmlNodeGetContent(xmlnode);
			jack_patch_set_src_client(patch, CAST_BAD content);
			xmlFree(content);
		} else if (strcmp(CAST_BAD(xmlnode->name), "src_client_id") == 0) {
			content = xmlNodeGetContent(xmlnode);
			uuid_parse(CAST_BAD content, patch->src_client_id);
			xmlFree(content);
		} else if (strcmp(CAST_BAD(xmlnode->name), "src_port") == 0) {
			content = xmlNodeGetContent(xmlnode);
			jack_patch_set_src_port(patch, CAST_BAD content);
			xmlFree(content);
		} else if (strcmp(CAST_BAD(xmlnode->name), "dest_client") == 0) {
			content = xmlNodeGetContent(xmlnode);
			jack_patch_set_dest_client(patch, CAST_BAD content);
			xmlFree(content);
		} else if (strcmp(CAST_BAD(xmlnode->name), "dest_client_id") == 0) {
			content = xmlNodeGetContent(xmlnode);
			uuid_parse(CAST_BAD content, patch->dest_client_id);
			xmlFree(content);
		} else if (strcmp(CAST_BAD(xmlnode->name), "dest_port") == 0) {
			content = xmlNodeGetContent(xmlnode);
			jack_patch_set_dest_port(patch, CAST_BAD content);
			xmlFree(content);
		}
	}
}

#define switch_str(a, b) \
  str = a; \
  a = b; \
  b = str;

void
jack_patch_switch_clients(jack_patch_t * patch)
{
	char *str;
	uuid_t id;

	switch_str(patch->src_client, patch->dest_client);
	switch_str(patch->src_port, patch->dest_port);

	uuid_copy(id, patch->src_client_id);
	uuid_copy(patch->src_client_id, patch->dest_client_id);
	uuid_copy(patch->dest_client_id, id);

}

/* EOF */
