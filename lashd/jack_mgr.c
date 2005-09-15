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

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <stdarg.h>

#include <jack/jack.h>
#include <lash/lash.h>
#include <lash/internal_headers.h>

#include "jack_mgr_client.h"
#include "jack_mgr.h"
#include "server_event.h"
#include "server.h"
#include "jack_fport.h"

#define BACKUP_INTERVAL ((time_t)(30))

static void *jack_mgr_callback_run(void *data);
static void jack_mgr_new_client_port(jack_mgr_t * jack_mgr,
									 const char *port_name_port,
									 jack_mgr_client_t * client);

const char *
get_jack_port_name_only(const char *port_name)
{
	static char *name = NULL;
	char *ptr;

	ptr = strchr(port_name, ':');
	if (!ptr)
		return NULL;
	ptr++;

	if (name)
		free(name);

	name = lash_strdup(ptr);

	return name;
}

const char *
get_jack_client_name_only(const char *port_name)
{
	static char *name = NULL;
	char *ptr;

	if (name)
		free(name);

	name = lash_strdup(port_name);

	ptr = strchr(name, ':');
	if (!ptr)
		return NULL;

	*ptr = '\0';

	return name;
}

static jack_mgr_client_t *
jack_mgr_find_client(jack_mgr_t * jack_mgr, uuid_t id)
{
	lash_list_t *list;
	jack_mgr_client_t *client;

	for (list = jack_mgr->clients; list; list = lash_list_next(list)) {
		client = (jack_mgr_client_t *) list->data;

		if (uuid_compare(id, client->id) == 0)
			return client;
	}

	return NULL;
}

static void
jack_mgr_jack_error(const char *message)
{
	fprintf(stderr, "JACK error: %s\n", message);
}

static void
jack_mgr_shutdown_cb(void *data)
{
	server_t *server;

	server = (server_t *) data;

	fprintf(stderr, "%s: JACK server shut us down; telling server to quit\n",
			__FUNCTION__);

	server->quit = 1;
	pthread_cond_signal(&server->server_event_cond);
}

static void
jack_mgr_port_registration_cb(jack_port_id_t port_id, int registered,
							  void *data)
{
	jack_mgr_t *jack_mgr;
	ssize_t err;
	char port_notify = 1;
	char reg = registered;

	jack_mgr = (jack_mgr_t *) data;

	err =
		write(jack_mgr->callback_write_socket, &port_notify,
			  sizeof(port_notify));
	if (err == -1) {
		fprintf(stderr,
				"%s: could not send data to jack manager callback, aborting: %s\n",
				__FUNCTION__, strerror(errno));
		abort();
	}

	err = write(jack_mgr->callback_write_socket, &port_id, sizeof(port_id));
	if (err == -1) {
		fprintf(stderr,
				"%s: could not send data to jack manager callback, aborting: %s\n",
				__FUNCTION__, strerror(errno));
		abort();
	}

	err = write(jack_mgr->callback_write_socket, &reg, sizeof(reg));
	if (err == -1) {
		fprintf(stderr,
				"%s: could not send data to jack manager callback, aborting: %s\n",
				__FUNCTION__, strerror(errno));
		abort();
	}

}

static int
jack_mgr_graph_reorder_cb(void *data)
{
	jack_mgr_t *jack_mgr;
	ssize_t err;
	char port_notify = 0;

	jack_mgr = (jack_mgr_t *) data;

	err =
		write(jack_mgr->callback_write_socket, &port_notify,
			  sizeof(port_notify));
	if (err == -1) {
		fprintf(stderr,
				"%s: could not send data to jack manager callback, aborting: %s\n",
				__FUNCTION__, strerror(errno));
		abort();
	}

	return 0;
}

static void
jack_mgr_init_jack(jack_mgr_t * jack_mgr)
{
	int err;
	const char *client_name = "LASH_Server";

	jack_set_error_function(jack_mgr_jack_error);

	jack_mgr->jack_client = jack_client_new(client_name);
	if (!jack_mgr->jack_client) {
		fprintf(stderr, "%s: could not connect to jackd, exiting\n",
				__FUNCTION__);
		exit(1);
	}

	printf("Connected to JACK server with client name '%s'\n", client_name);

	err =
		jack_set_port_registration_callback(jack_mgr->jack_client,
											jack_mgr_port_registration_cb,
											jack_mgr);
	if (err) {
		fprintf(stderr, "%s: could not set jack port registration callback\n",
				__FUNCTION__);
		abort();
	}

	err =
		jack_set_graph_order_callback(jack_mgr->jack_client,
									  jack_mgr_graph_reorder_cb, jack_mgr);
	if (err) {
		fprintf(stderr, "%s: could not set jack graph order callback\n",
				__FUNCTION__);
		abort();
	}

	jack_on_shutdown(jack_mgr->jack_client, jack_mgr_shutdown_cb,
					 jack_mgr->server);

	err = jack_activate(jack_mgr->jack_client);
	if (err) {
		fprintf(stderr, "%s: could not activate jack client\n", __FUNCTION__);
		abort();
	}
}

jack_mgr_t *
jack_mgr_new(server_t * server)
{
	jack_mgr_t *jack_mgr;
	int sockets[2];
	int err;

	jack_mgr = lash_malloc(sizeof(jack_mgr_t));
	jack_mgr->server = server;
	jack_mgr->quit = 0;
	jack_mgr->clients = NULL;
	jack_mgr->foreign_ports = NULL;

	pthread_mutex_init(&jack_mgr->lock, NULL);

	err = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	if (err == -1) {
		fprintf(stderr, "%s: could not create unix socket pair: %s\n",
				__FUNCTION__, strerror(errno));
		abort();
	}

	jack_mgr->callback_read_socket = sockets[0];
	jack_mgr->callback_write_socket = sockets[1];

	jack_mgr_init_jack(jack_mgr);

	pthread_create(&jack_mgr->callback_thread, NULL, jack_mgr_callback_run,
				   jack_mgr);

	return jack_mgr;
}

void
jack_mgr_destroy(jack_mgr_t * jack_mgr)
{
	int err;
	char dummy;

	jack_mgr->quit = 1;

	err = jack_deactivate(jack_mgr->jack_client);
	if (err) {
		fprintf(stderr, "%s: could not deactivate jack client\n",
				__FUNCTION__);
	}

	LASH_PRINT_DEBUG("stopping");
	err = write(jack_mgr->callback_write_socket, &dummy, sizeof(dummy));
	if (err == -1) {
		fprintf(stderr,
				"%s: could not send data to jack manager callback, aborting: %s\n",
				__FUNCTION__, strerror(errno));
		abort();
	}

	err = pthread_join(jack_mgr->callback_thread, NULL);
	if (err) {
		fprintf(stderr, "%s: error joining callback thread: %s\n",
				__FUNCTION__, strerror(errno));
		return;
	}
	LASH_PRINT_DEBUG("stopped");

	jack_client_close(jack_mgr->jack_client);

/* FIXME: free all the data */

	free(jack_mgr);
}

void
jack_mgr_lock(jack_mgr_t * jack_mgr)
{
	pthread_mutex_lock(&jack_mgr->lock);
}

void
jack_mgr_unlock(jack_mgr_t * jack_mgr)
{
	pthread_mutex_unlock(&jack_mgr->lock);
}

/* caller must hold clients lock */
void
jack_mgr_check_client_ports(jack_mgr_t * jack_mgr, jack_mgr_client_t * client)
{
	char *port_name;
	lash_list_t *list;
	jack_fport_t *fport;

	for (list = jack_mgr->foreign_ports; list;) {
		fport = (jack_fport_t *) list->data;
		list = lash_list_next(list);

		if (strcmp(client->name, get_jack_client_name_only(fport->name)) == 0) {
			port_name = lash_strdup(get_jack_port_name_only(fport->name));
			LASH_DEBUGARGS("resuming previously foreign port '%s'",
						   fport->name);
			jack_mgr_new_client_port(jack_mgr, port_name, client);
			free(port_name);

			jack_mgr->foreign_ports =
				lash_list_remove(jack_mgr->foreign_ports, fport);
			jack_fport_destroy(fport);
		}
	}
}

void
jack_mgr_add_client(jack_mgr_t * jack_mgr,
					uuid_t id,
					const char *jack_client_name, lash_list_t * jack_patches)
{
	jack_mgr_client_t *client;

	LASH_DEBUGARGS("adding client '%s'", jack_client_name);

	client = jack_mgr_client_new();
	jack_mgr_client_set_id(client, id);
	jack_mgr_client_set_name(client, jack_client_name);
	client->old_patches = jack_patches;

	jack_mgr->clients = lash_list_append(jack_mgr->clients, client);

	/* check if it's registered some ports already */
	jack_mgr_check_client_ports(jack_mgr, client);

	LASH_PRINT_DEBUG("client added");
}

lash_list_t *
jack_mgr_remove_client(jack_mgr_t * jack_mgr, uuid_t id)
{
	jack_mgr_client_t *client;
	lash_list_t *backup_patches;

	LASH_PRINT_DEBUG("removing client");

	client = jack_mgr_find_client(jack_mgr, id);
	if (!client) {
		fprintf(stderr, "%s: unknown client\n", __FUNCTION__);
		return NULL;
	}

	jack_mgr->clients = lash_list_remove(jack_mgr->clients, client);

	LASH_DEBUGARGS("removed client '%s'", client->name);

	backup_patches = client->backup_patches;
	client->backup_patches = NULL;

	jack_mgr_client_destroy(client);

	return backup_patches;
}

lash_list_t *
jack_mgr_get_client_patches(jack_mgr_t * jack_mgr, uuid_t id)
{
	jack_mgr_client_t *client;
	lash_list_t *node;
	lash_list_t *uniq_node;
	lash_list_t *patches;
	lash_list_t *uniq_patches = NULL;
	jack_patch_t *patch;
	jack_patch_t *uniq_patch;

	client = jack_mgr_find_client(jack_mgr, id);
	if (!client) {
		fprintf(stderr, "%s: unknown client\n", __FUNCTION__);
		return NULL;
	}

	patches = jack_mgr_client_dup_patches(client);

	for (node = patches; node; node = lash_list_next(node)) {
		patch = (jack_patch_t *) node->data;

		jack_patch_set(patch, jack_mgr->clients);

		for (uniq_node = uniq_patches; uniq_node;
			 uniq_node = lash_list_next(uniq_node)) {
			uniq_patch = (jack_patch_t *) uniq_node->data;

			if (strcmp(patch->src_client, uniq_patch->src_client) == 0 &&
				strcmp(patch->src_port, uniq_patch->src_port) == 0 &&
				strcmp(patch->dest_client, uniq_patch->dest_client) == 0 &&
				strcmp(patch->dest_port, uniq_patch->dest_port) == 0)
				break;
		}

		if (uniq_node)
			jack_patch_destroy(patch);
		else
			uniq_patches = lash_list_append(uniq_patches, patch);
	}

	lash_list_free(patches);

	return uniq_patches;
}

/* caller must hold client lock */
static void
jack_mgr_add_foreign_port(jack_mgr_t * jack_mgr, jack_fport_t * port)
{
	jack_mgr->foreign_ports = lash_list_append(jack_mgr->foreign_ports, port);
}

static void
jack_mgr_remove_foreign_port(jack_mgr_t * jack_mgr, jack_port_id_t id)
{
	jack_fport_t *port;
	lash_list_t *list;

	for (list = jack_mgr->foreign_ports; list; list = lash_list_next(list)) {
		port = (jack_fport_t *) list->data;

		if (id == port->id) {
			jack_mgr->foreign_ports =
				lash_list_remove(jack_mgr->foreign_ports, port);

			LASH_DEBUGARGS("removed foreign port '%s'", port->name);
			jack_fport_destroy(port);

			return;
		}
	}
}

static int
jack_mgr_resume_patch(jack_mgr_t * jack_mgr, jack_patch_t * patch)
{
	int err;
	char *src;
	char *dest;

	src = lash_strdup(jack_patch_get_src(patch));
	dest = lash_strdup(jack_patch_get_dest(patch));

	err = jack_connect(jack_mgr->jack_client, src, dest);
	if (err) {
		LASH_DEBUGARGS("could not (yet?) resume patch %s",
					   jack_patch_get_desc(patch));
	} else
		printf("Connected JACK port '%s' to '%s'\n", src, dest);

	free(src);
	free(dest);

	return err;
}

/*
 * Here we have to go through each patch, see if it's the specified port,
 * unset it, see if we can connect it, and remove it.  Other clients will
 * have connections for this one if they're not registered yet.
 */
static void
jack_mgr_new_client_port(jack_mgr_t * jack_mgr, const char *port,
						 jack_mgr_client_t * client)
{
	lash_list_t *node;
	jack_patch_t *patch, *unset_patch = NULL;
	int err;

	for (node = client->old_patches; node;) {
		patch = (jack_patch_t *) node->data;
		node = lash_list_next(node);

		LASH_DEBUGARGS("checking patch %s", jack_patch_get_desc(patch));

		unset_patch = jack_patch_dup(patch);
		err = jack_patch_unset(unset_patch, jack_mgr->clients);
		if (err) {
			LASH_DEBUGARGS
				("could not resume jack patch '%s' as the other client hasn't registered yet",
				 jack_patch_get_desc(patch));
			client->old_patches =
				lash_list_remove(client->old_patches, patch);
			jack_patch_destroy(patch);
			jack_patch_destroy(unset_patch);
			continue;
		}

		if ((strcmp(client->name, unset_patch->src_client) == 0 &&
			 strcmp(port, unset_patch->src_port) == 0) ||
			(strcmp(client->name, unset_patch->dest_client) == 0 &&
			 strcmp(port, unset_patch->dest_port) == 0)) {
			LASH_DEBUGARGS("attempting to resume patch %s",
						   jack_patch_get_desc(unset_patch));
			err = jack_mgr_resume_patch(jack_mgr, unset_patch);
			if (!err) {
				client->old_patches =
					lash_list_remove(client->old_patches, patch);
				jack_patch_destroy(patch);
			}
		}

		jack_patch_destroy(unset_patch);
	}
}

static void
jack_mgr_port_notification(jack_mgr_t * jack_mgr, jack_port_id_t port_id)
{
	jack_mgr_client_t *client;
	jack_port_t *jack_port;
	char *client_name;
	lash_list_t *list;
	char *ptr;

	/* get the client name */
	jack_port = jack_port_by_id(jack_mgr->jack_client, port_id);
	if (!jack_port) {
		fprintf(stderr,
				"%s: jack port registration notification for non-existant port\n",
				__FUNCTION__);
		return;
	}

	LASH_DEBUGARGS("recieved port registration notification for port '%u'",
				   port_id);

	client_name = lash_strdup(jack_port_name(jack_port));
	ptr = strchr(client_name, ':');
	if (!ptr) {
		fprintf(stderr,
				"%s: port name '%s' contains no client name/port name seperator\n",
				__FUNCTION__, client_name);
		return;
	}
	*ptr = '\0';

	for (list = jack_mgr->clients; list; list = lash_list_next(list)) {
		client = (jack_mgr_client_t *) list->data;

		if (strcmp(jack_mgr_client_get_name(client), client_name) == 0)
			break;
	}

	if (!list) {
		jack_fport_t *fport;

		LASH_DEBUGARGS("adding foreign port '%s'", jack_port_name(jack_port));
		fport = jack_fport_new_with_id(port_id);
		jack_fport_set_name(fport, jack_port_name(jack_port));

		jack_mgr_add_foreign_port(jack_mgr, fport);

		return;
	}

	ptr++;
	jack_mgr_new_client_port(jack_mgr, ptr, client);

	free(client_name);
}

lash_list_t *
jack_mgr_get_patches_with_type(const char *client_name,
							   jack_client_t * jack_client, int type_flags)
{
	jack_patch_t *patch;
	char *port_name_regex;
	jack_port_t *port;
	char **port_names;
	char **connected_port_names;
	int i, j;
	lash_list_t *list = NULL;

	port_name_regex = lash_malloc(strlen(client_name) + 4);
	sprintf(port_name_regex, "%s:.*", client_name);

	port_names =
		(char **)jack_get_ports(jack_client, port_name_regex, NULL,
								type_flags);
	free(port_name_regex);

	if (!port_names)
		return NULL;

	for (i = 0; port_names[i]; i++) {
		port = jack_port_by_name(jack_client, port_names[i]);

		connected_port_names =
			(char **)jack_port_get_all_connections(jack_client, port);

		if (!connected_port_names)
			continue;

		for (j = 0; connected_port_names[j]; j++) {
			patch = jack_patch_new();

			jack_patch_set_src(patch, port_names[i]);
			jack_patch_set_dest(patch, connected_port_names[j]);
			if (type_flags & JackPortIsInput)
				jack_patch_switch_clients(patch);

			list = lash_list_append(list, patch);
			LASH_DEBUGARGS("found patch: '%s'/'%s'",
						   jack_patch_get_src(patch),
						   jack_patch_get_dest(patch));
		}

/*      free (connected_port_names); */
	}

/*  free (port_names); */

	return list;
}

static void
jack_mgr_get_patches(jack_mgr_t * jack_mgr, jack_mgr_client_t * client)
{
	lash_list_t *patches = NULL;

	jack_mgr_client_free_patches(client);

	patches = jack_mgr_get_patches_with_type(client->name,
											 jack_mgr->jack_client,
											 JackPortIsInput);
	client->patches = patches;

	patches = jack_mgr_get_patches_with_type(client->name,
											 jack_mgr->jack_client,
											 JackPortIsOutput);

	client->patches = lash_list_concat(client->patches, patches);
}

static void
jack_mgr_check_foreign_ports(jack_mgr_t * jack_mgr)
{
	lash_list_t *list;
	jack_fport_t *fport;
	jack_port_t *port;

	for (list = jack_mgr->foreign_ports; list;) {
		fport = (jack_fport_t *) list->data;
		list = lash_list_next(list);

		port =
			jack_port_by_id(jack_mgr->jack_client, jack_fport_get_id(fport));

		if (!port) {
			jack_mgr->foreign_ports =
				lash_list_remove(jack_mgr->foreign_ports, fport);
			LASH_DEBUGARGS("removed foreign port '%s'", fport->name);

			jack_fport_destroy(fport);
		}
	}
}

static void
jack_mgr_graph_reorder_notification(jack_mgr_t * jack_mgr)
{
	lash_list_t *list;
	jack_mgr_client_t *client;

	for (list = jack_mgr->clients; list; list = lash_list_next(list)) {
		client = (jack_mgr_client_t *) list->data;

		jack_mgr_get_patches(jack_mgr, client);
	}

	jack_mgr_check_foreign_ports(jack_mgr);
}

static void
jack_mgr_read_callback(jack_mgr_t * jack_mgr)
{
	char port_notify;
	char registered;
	jack_port_id_t port_id;
	int err;

	err =
		read(jack_mgr->callback_read_socket, &port_notify,
			 sizeof(port_notify));
	if (err == -1) {
		fprintf(stderr,
				"%s: could not read data from callback socket; aborting: %s\n",
				__FUNCTION__, strerror(errno));
		abort();
	}

	if (port_notify) {
		LASH_PRINT_DEBUG("jack port reg callback happened");

		err = read(jack_mgr->callback_read_socket, &port_id, sizeof(port_id));
		if (err == -1) {
			fprintf(stderr,
					"%s: could not read data from callback socket; aborting: %s\n",
					__FUNCTION__, strerror(errno));
			abort();
		}

		err =
			read(jack_mgr->callback_read_socket, &registered,
				 sizeof(registered));
		if (err == -1) {
			fprintf(stderr,
					"%s: could not read data from callback socket; aborting: %s\n",
					__FUNCTION__, strerror(errno));
			abort();
		}

		if (registered)
			jack_mgr_port_notification(jack_mgr, port_id);
		else
			jack_mgr_remove_foreign_port(jack_mgr, port_id);
	} else {
		LASH_PRINT_DEBUG("jack graph order callback happened");

		jack_mgr_graph_reorder_notification(jack_mgr);
	}
}

static void
jack_mgr_backup_patches(jack_mgr_t * jack_mgr)
{
	static time_t last_backup = 0;
	time_t now;
	lash_list_t *list, *patches;
	jack_mgr_client_t *client;
	jack_patch_t *patch;

	now = time(NULL);

	if (now - last_backup < BACKUP_INTERVAL)
		return;

	for (list = jack_mgr->clients; list; list = lash_list_next(list)) {
		client = (jack_mgr_client_t *) list->data;

		jack_mgr_client_free_backup_patches(client);
		client->backup_patches = jack_mgr_client_dup_patches(client);

		for (patches = client->backup_patches; patches;
			 patches = lash_list_next(patches)) {
			patch = (jack_patch_t *) patches->data;

			jack_patch_set(patch, jack_mgr->clients);
		}
	}

	last_backup = now;
}

static void *
jack_mgr_callback_run(void *data)
{
	jack_mgr_t *jack_mgr;
	fd_set socket_set;
	fd_set select_set;
	struct timeval timeout;
	int err;

	jack_mgr = (jack_mgr_t *) data;

	FD_ZERO(&socket_set);
	FD_SET(jack_mgr->callback_read_socket, &socket_set);

	while (!jack_mgr->quit) {
		/* backup timeout */
		timeout.tv_sec = BACKUP_INTERVAL;
		timeout.tv_usec = 0;

		select_set = socket_set;

		err =
			select(jack_mgr->callback_read_socket + 1, &select_set, NULL,
				   NULL, &timeout);

		if (err == -1) {
			if (errno == EINTR)
				continue;

			fprintf(stderr, "%s: error calling select(): %s\n", __FUNCTION__,
					strerror(errno));
		}

		if (jack_mgr->quit)
			break;

		jack_mgr_lock(jack_mgr);

		if (FD_ISSET(jack_mgr->callback_read_socket, &select_set))
			jack_mgr_read_callback(jack_mgr);

		jack_mgr_backup_patches(jack_mgr);

		jack_mgr_unlock(jack_mgr);
	}

	LASH_PRINT_DEBUG("finished");
	return NULL;
}

/* EOF */
