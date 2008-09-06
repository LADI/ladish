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

#include "alsa_patch.h"

#ifdef HAVE_ALSA

# include <stdlib.h>

# include "client.h"
# include "alsa_client.h"
# include "common/safety.h"
# include "common/debug.h"

void
alsa_patch_init(alsa_patch_t * patch)
{
	int err;

	err = snd_seq_port_subscribe_malloc(&patch->sub);
	if (err)
		abort();

	uuid_clear(patch->src_id);
	uuid_clear(patch->dest_id);
}

void
alsa_patch_free(alsa_patch_t * patch)
{
	snd_seq_port_subscribe_free(patch->sub);
}

alsa_patch_t *
alsa_patch_new()
{
	alsa_patch_t *patch;

	patch = lash_malloc(1, sizeof(alsa_patch_t));
	alsa_patch_init(patch);
	return patch;
}

alsa_patch_t *
alsa_patch_new_with_sub(const snd_seq_port_subscribe_t * sub)
{
	alsa_patch_t *patch;

	patch = alsa_patch_new();
	alsa_patch_set_sub(patch, sub);
	return patch;
}

alsa_patch_t *
alsa_patch_new_with_query(const snd_seq_query_subscribe_t * query)
{
	alsa_patch_t *patch;

	patch = alsa_patch_new();
	alsa_patch_set_sub_from_query(patch, query);
	return patch;
}

void
alsa_patch_destroy(alsa_patch_t * patch)
{
	alsa_patch_free(patch);
	free(patch);
}

const snd_seq_port_subscribe_t *
alsa_patch_get_sub(const alsa_patch_t * patch)
{
	return patch->sub;
}

int
alsa_patch_src_id_is_null(alsa_patch_t * patch)
{
	return uuid_is_null(patch->src_id);
}

int
alsa_patch_to_dest_is_null(alsa_patch_t * patch)
{
	return uuid_is_null(patch->dest_id);
}

void
alsa_patch_set_sub(alsa_patch_t * patch, const snd_seq_port_subscribe_t * sub)
{
	if (sub)
		snd_seq_port_subscribe_copy(patch->sub, sub);
}

void
alsa_patch_set_sub_from_query(alsa_patch_t * patch,
							  const snd_seq_query_subscribe_t * query)
{
	if (query)
		alsa_patch_set_sub(patch, query_to_sub(query));
}

static void
alsa_patch_set_client_id(lash_list_t * alsa_mgr_clients,
						 unsigned char alsa_client_id, uuid_t patch_client_id)
{
	alsa_client_t *client;

	for (; alsa_mgr_clients;
		 alsa_mgr_clients = lash_list_next(alsa_mgr_clients)) {
		client = (alsa_client_t *) alsa_mgr_clients->data;

		if (client->client_id == alsa_client_id) {
			uuid_copy(patch_client_id, client->id);
			return;
		}
	}
}

void
alsa_patch_set(alsa_patch_t * patch, lash_list_t * alsa_mgr_clients)
{
	const snd_seq_addr_t *addr;

	addr = snd_seq_port_subscribe_get_sender(patch->sub);
	alsa_patch_set_client_id(alsa_mgr_clients, addr->client, patch->src_id);

	addr = snd_seq_port_subscribe_get_dest(patch->sub);
	alsa_patch_set_client_id(alsa_mgr_clients, addr->client, patch->dest_id);
}

typedef const snd_seq_addr_t *(*alsa_patch_get_sub_addr_func) (const
															   snd_seq_port_subscribe_t
															   * info);
typedef void (*alsa_patch_set_sub_addr_func) (snd_seq_port_subscribe_t * info,
											  const snd_seq_addr_t * addr);

int
alsa_patch_unset_client_id(alsa_patch_t * patch,
						   lash_list_t * alsa_mgr_clients,
						   uuid_t patch_client_id,
						   alsa_patch_get_sub_addr_func get_sub_addr,
						   alsa_patch_set_sub_addr_func set_sub_addr)
{
	alsa_client_t *client;

	for (; alsa_mgr_clients;
		 alsa_mgr_clients = lash_list_next(alsa_mgr_clients)) {
		client = (alsa_client_t *) alsa_mgr_clients->data;

		if (uuid_compare(patch_client_id, client->id) == 0) {
			snd_seq_addr_t addr;

# ifdef LASH_DEBUG
			{
				char id[37];

				uuid_unparse(client->id, id);
				lash_debug("found alsa mgr client with id '%s'", id);
			}
# endif

			addr.client = client->client_id;
			addr.port = get_sub_addr(patch->sub)->port;
			set_sub_addr(patch->sub, &addr);

			return 0;
		}
	}

# ifdef LASH_DEBUG
	{
		char id[37];

		uuid_unparse(client->id, id);
		lash_debug("could not find alsa mgr client with id '%s'", id);
	}
# endif

	return 1;
}

int
alsa_patch_unset(alsa_patch_t * patch, lash_list_t * alsa_mgr_clients)
{
	int err;
	int not_found = 0;

	if (!uuid_is_null(patch->src_id)) {
		err = alsa_patch_unset_client_id(patch,
										 alsa_mgr_clients,
										 patch->src_id,
										 snd_seq_port_subscribe_get_sender,
										 snd_seq_port_subscribe_set_sender);
		if (err)
			not_found++;
	}

	if (!uuid_is_null(patch->dest_id)) {
		err = alsa_patch_unset_client_id(patch,
										 alsa_mgr_clients,
										 patch->dest_id,
										 snd_seq_port_subscribe_get_dest,
										 snd_seq_port_subscribe_set_dest);
		if (err)
			not_found++;
	}

	return not_found;
}

const snd_seq_port_subscribe_t *
query_to_sub(const snd_seq_query_subscribe_t * query)
{
	static snd_seq_port_subscribe_t *sub = NULL;

	if (!query)
		return NULL;

	if (!sub) {
		int err;

		err = snd_seq_port_subscribe_malloc(&sub);
		if (err)
			abort();
	}

	snd_seq_port_subscribe_set_sender(sub,
									  snd_seq_query_subscribe_get_root
									  (query));
	snd_seq_port_subscribe_set_dest(sub,
									snd_seq_query_subscribe_get_addr(query));
	snd_seq_port_subscribe_set_exclusive(sub,
										 snd_seq_query_subscribe_get_exclusive
										 (query));
	snd_seq_port_subscribe_set_time_update(sub,
										   snd_seq_query_subscribe_get_time_update
										   (query));
	snd_seq_port_subscribe_set_time_real(sub,
										 snd_seq_query_subscribe_get_time_real
										 (query));
	snd_seq_port_subscribe_set_queue(sub,
									 snd_seq_query_subscribe_get_queue
									 (query));

	return sub;
}

void
alsa_patch_switch_clients(alsa_patch_t * patch)
{
	unsigned char from_client, from_port, to_client, to_port;
	snd_seq_addr_t addr;
	const snd_seq_addr_t *addrptr;
	uuid_t id;

	addrptr = snd_seq_port_subscribe_get_sender(patch->sub);
	from_client = addrptr->client;
	from_port = addrptr->port;

	addrptr = snd_seq_port_subscribe_get_dest(patch->sub);
	to_client = addrptr->client;
	to_port = addrptr->port;

	addr.client = to_client;
	addr.port = to_port;
	snd_seq_port_subscribe_set_sender(patch->sub, &addr);

	addr.client = from_client;
	addr.port = from_port;
	snd_seq_port_subscribe_set_dest(patch->sub, &addr);

	uuid_copy(id, patch->src_id);
	uuid_copy(patch->src_id, patch->dest_id);
	uuid_copy(patch->dest_id, id);
}

alsa_patch_t *
alsa_patch_dup(const alsa_patch_t * other_patch)
{
	alsa_patch_t *patch;

	patch = alsa_patch_new_with_sub(alsa_patch_get_sub(other_patch));

	uuid_copy(patch->src_id, other_patch->src_id);
	uuid_copy(patch->dest_id, other_patch->dest_id);

	return patch;
}

unsigned char
alsa_patch_get_src_client(const alsa_patch_t * patch)
{
	const snd_seq_addr_t *addr;

	addr = snd_seq_port_subscribe_get_sender(patch->sub);

	return addr->client;
}

unsigned char
alsa_patch_get_src_port(const alsa_patch_t * patch)
{
	const snd_seq_addr_t *addr;

	addr = snd_seq_port_subscribe_get_sender(patch->sub);

	return addr->port;
}

unsigned char
alsa_patch_get_dest_client(const alsa_patch_t * patch)
{
	const snd_seq_addr_t *addr;

	addr = snd_seq_port_subscribe_get_dest(patch->sub);

	return addr->client;
}
unsigned char
alsa_patch_get_dest_port(const alsa_patch_t * patch)
{
	const snd_seq_addr_t *addr;

	addr = snd_seq_port_subscribe_get_dest(patch->sub);

	return addr->port;
}

void
alsa_patch_create_xml(alsa_patch_t * patch, xmlNodePtr parent)
{
	xmlNodePtr patchxml;
	char str[37];

	patchxml = xmlNewChild(parent, NULL, BAD_CAST "alsa_patch", NULL);

	if (uuid_is_null(patch->src_id)) {
		sprintf(str, "%d",
				(int)snd_seq_port_subscribe_get_sender(patch->sub)->client);
		xmlNewChild(patchxml, NULL, BAD_CAST "src_client", BAD_CAST str);
	} else {
		uuid_unparse(patch->src_id, str);
		xmlNewChild(patchxml, NULL, BAD_CAST "src_client_id", BAD_CAST str);
	}

	sprintf(str, "%d",
			(int)snd_seq_port_subscribe_get_sender(patch->sub)->port);
	xmlNewChild(patchxml, NULL, BAD_CAST "src_port", BAD_CAST str);

	if (uuid_is_null(patch->dest_id)) {
		sprintf(str, "%d",
				(int)snd_seq_port_subscribe_get_dest(patch->sub)->client);
		xmlNewChild(patchxml, NULL, BAD_CAST "dest_client", BAD_CAST str);
	} else {
		uuid_unparse(patch->dest_id, str);
		xmlNewChild(patchxml, NULL, BAD_CAST "dest_client_id", BAD_CAST str);
	}

	sprintf(str, "%d",
			(int)snd_seq_port_subscribe_get_dest(patch->sub)->port);
	xmlNewChild(patchxml, NULL, BAD_CAST "dest_port", BAD_CAST str);

	sprintf(str, "%d", snd_seq_port_subscribe_get_queue(patch->sub));
	xmlNewChild(patchxml, NULL, BAD_CAST "queue", BAD_CAST str);

	sprintf(str, "%d", snd_seq_port_subscribe_get_exclusive(patch->sub));
	xmlNewChild(patchxml, NULL, BAD_CAST "exclusive", BAD_CAST str);

	sprintf(str, "%d", snd_seq_port_subscribe_get_time_update(patch->sub));
	xmlNewChild(patchxml, NULL, BAD_CAST "time_update", BAD_CAST str);

	sprintf(str, "%d", snd_seq_port_subscribe_get_time_real(patch->sub));
	xmlNewChild(patchxml, NULL, BAD_CAST "time_real", BAD_CAST str);
}

void
alsa_patch_parse_xml(alsa_patch_t * patch, xmlNodePtr parent)
{
	xmlNodePtr xmlnode;
	xmlChar *content;
	snd_seq_addr_t addr;
	const snd_seq_addr_t *addrptr;

	for (xmlnode = parent->children; xmlnode; xmlnode = xmlnode->next) {
		if (strcmp((const char*) xmlnode->name, "src_client") == 0) {
			content = xmlNodeGetContent(xmlnode);
			addrptr = snd_seq_port_subscribe_get_sender(patch->sub);
			addr.client = strtoul((const char*) content, NULL, 10);
			addr.port = addrptr->port;
			snd_seq_port_subscribe_set_sender(patch->sub, &addr);
			xmlFree(content);
		} else if (strcmp((const char*) xmlnode->name, "src_client_id") == 0) {
			content = xmlNodeGetContent(xmlnode);
			uuid_parse((char *) content, patch->src_id);
			xmlFree(content);
		} else if (strcmp((const char*) xmlnode->name, "src_port") == 0) {
			content = xmlNodeGetContent(xmlnode);
			addrptr = snd_seq_port_subscribe_get_sender(patch->sub);
			addr.port = strtoul((const char *) content, NULL, 10);
			addr.client = addrptr->client;
			snd_seq_port_subscribe_set_sender(patch->sub, &addr);
			xmlFree(content);
		} else if (strcmp((const char*) xmlnode->name, "dest_client") == 0) {
			content = xmlNodeGetContent(xmlnode);
			addrptr = snd_seq_port_subscribe_get_dest(patch->sub);
			addr.client = strtoul((const char *) content, NULL, 10);
			addr.port = addrptr->port;
			snd_seq_port_subscribe_set_dest(patch->sub, &addr);
			xmlFree(content);
		} else if (strcmp((const char*) xmlnode->name, "dest_client_id") == 0) {
			content = xmlNodeGetContent(xmlnode);
			uuid_parse((char *) content, patch->dest_id);
			xmlFree(content);
		} else if (strcmp((const char*) xmlnode->name, "dest_port") == 0) {
			content = xmlNodeGetContent(xmlnode);
			addrptr = snd_seq_port_subscribe_get_dest(patch->sub);
			addr.port = strtoul((const char *) content, NULL, 10);
			addr.client = addrptr->client;
			snd_seq_port_subscribe_set_dest(patch->sub, &addr);
			xmlFree(content);
		} else if (strcmp((const char*) xmlnode->name, "queue") == 0) {
			content = xmlNodeGetContent(xmlnode);
			snd_seq_port_subscribe_set_queue(patch->sub,
											 strtoul((const char *) content, NULL,
													 10));
			xmlFree(content);
		} else if (strcmp((const char*) xmlnode->name, "exclusive") == 0) {
			content = xmlNodeGetContent(xmlnode);
			snd_seq_port_subscribe_set_exclusive(patch->sub,
												 strtoul((const char *) content,
														 NULL, 10));
			xmlFree(content);
		} else if (strcmp((const char*) xmlnode->name, "time_update") == 0) {
			content = xmlNodeGetContent(xmlnode);
			snd_seq_port_subscribe_set_time_update(patch->sub,
												   strtoul((const char *) content,
														   NULL, 10));
			xmlFree(content);
		} else if (strcmp((const char*) xmlnode->name, "time_real") == 0) {
			content = xmlNodeGetContent(xmlnode);
			snd_seq_port_subscribe_set_time_real(patch->sub,
												 strtoul((const char *) content,
														 NULL, 10));
			xmlFree(content);
		}
	}
}

const char *
alsa_patch_get_desc(alsa_patch_t * patch)
{
	static char desc[256];
	char src_client[37];
	char dest_client[37];

	if (uuid_is_null(patch->src_id))
		sprintf(src_client, "%d",
				(int)snd_seq_port_subscribe_get_sender(patch->sub)->client);
	else
		uuid_unparse(patch->src_id, src_client);

	if (uuid_is_null(patch->dest_id))
		sprintf(dest_client, "%d",
				(int)snd_seq_port_subscribe_get_dest(patch->sub)->client);
	else
		uuid_unparse(patch->dest_id, dest_client);

	sprintf(desc, "%s:%d -> %s:%d <queue=%d%s%s%s>",
			src_client,
			(int)snd_seq_port_subscribe_get_sender(patch->sub)->port,
			dest_client,
			(int)snd_seq_port_subscribe_get_dest(patch->sub)->port,
			snd_seq_port_subscribe_get_queue(patch->sub),
			snd_seq_port_subscribe_get_exclusive(patch->
												 sub) ? ",exclusive" : "",
			snd_seq_port_subscribe_get_time_update(patch->
												   sub) ? ",time_update" : "",
			snd_seq_port_subscribe_get_time_real(patch->
												 sub) ? ",time_real" : "");

	return desc;
}

int
alsa_patch_compare(alsa_patch_t * a, alsa_patch_t * b)
{
	const snd_seq_addr_t *addr_a, *addr_b;

	addr_a = snd_seq_port_subscribe_get_sender(a->sub);
	addr_b = snd_seq_port_subscribe_get_sender(b->sub);
	if (addr_a->client != addr_b->client || addr_a->port != addr_b->port)
		return 1;

	addr_a = snd_seq_port_subscribe_get_dest(a->sub);
	addr_b = snd_seq_port_subscribe_get_dest(b->sub);
	if (addr_a->client != addr_b->client || addr_a->port != addr_b->port)
		return 1;

	if ((snd_seq_port_subscribe_get_queue(a->sub) !=
		 snd_seq_port_subscribe_get_queue(b->sub)) ||
		(snd_seq_port_subscribe_get_exclusive(a->sub) !=
		 snd_seq_port_subscribe_get_exclusive(b->sub)) ||
		(snd_seq_port_subscribe_get_time_update(a->sub) !=
		 snd_seq_port_subscribe_get_time_update(b->sub)) ||
		(snd_seq_port_subscribe_get_time_real(a->sub) !=
		 snd_seq_port_subscribe_get_time_real(b->sub)))
		return 1;

	return 0;
}

#endif /* HAVE_ALSA */
