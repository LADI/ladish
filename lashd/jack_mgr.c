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

#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <jack/jack.h>

#include "common/safety.h"
#include "common/debug.h"

#include "jack_mgr.h"
#include "jack_mgr_client.h"
#include "server.h"
#include "jack_fport.h"
#include "jack_patch.h"

#define BACKUP_INTERVAL ((time_t)(30))

static void *
jack_mgr_callback_run(void *data);

static void
jack_mgr_new_client_port(jack_mgr_t        *jack_mgr,
                         const char        *port_name_port,
                         jack_mgr_client_t *client);

static void
jack_mgr_jack_error(const char *message)
{
	lash_error("JACK error: %s", message);
}

static void
jack_mgr_shutdown_cb(void *data)
{
	lash_error("JACK server shut us down; telling server to quit");
	g_server->quit = true;
}

static void
jack_mgr_port_registration_cb(jack_port_id_t  port_id,
                              int             registered,
                              void           *data)
{
	jack_mgr_t *jack_mgr = (jack_mgr_t *) data;
	char port_notify = 1, reg = (char) registered;
	int sock = jack_mgr->callback_write_socket;

	if (write(sock, &port_notify, 1) == -1
	    || write(sock, &port_id, sizeof(jack_port_id_t)) == -1
	    || write(sock, &reg, 1) == -1) {
		lash_error("Could not send data to JACK manager callback, "
		           "aborting: %s", strerror(errno));
		abort();
	}
}

static int
jack_mgr_graph_reorder_cb(void *data)
{
	jack_mgr_t *jack_mgr = (jack_mgr_t *) data;
	char port_notify = 0;

	if (write(jack_mgr->callback_write_socket, &port_notify, 1) == -1) {
		lash_error("Could not send data to JACK manager callback, "
		           "aborting: %s", strerror(errno));
		abort();
	}

	return 0;
}

static void
jack_mgr_init_jack(jack_mgr_t * jack_mgr)
{
	jack_set_error_function(jack_mgr_jack_error);

	jack_mgr->jack_client = jack_client_open("LASH_Server", JackNullOption,
	                                         NULL);
	if (!jack_mgr->jack_client) {
		lash_error("Could not connect to JACK, exiting");
		exit(1);
	}

	lash_info("Connected to JACK server with client name '%s'",
	          jack_get_client_name(jack_mgr->jack_client));

	if (jack_set_port_registration_callback(jack_mgr->jack_client,
	                                        jack_mgr_port_registration_cb,
	                                        jack_mgr) != 0) {
		lash_error("Could not set JACK port registration callback");
		abort();
	}

	if (jack_set_graph_order_callback(jack_mgr->jack_client,
	                                  jack_mgr_graph_reorder_cb,
	                                  jack_mgr) != 0) {
		lash_error("Could not set JACK graph order callback");
		abort();
	}

	jack_on_shutdown(jack_mgr->jack_client, jack_mgr_shutdown_cb, g_server);

	if (jack_activate(jack_mgr->jack_client) != 0) {
		lash_error("Could not activate JACK client");
		abort();
	}
}

jack_mgr_t *
jack_mgr_new(void)
{
	jack_mgr_t *jack_mgr;
	int sockets[2];

	jack_mgr = lash_malloc(1, sizeof(jack_mgr_t));
	jack_mgr->quit = 0;
	jack_mgr->clients = NULL;
	jack_mgr->foreign_ports = NULL;

	pthread_mutex_init(&jack_mgr->lock, NULL);

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == -1) {
		lash_error("Could not create Unix socket pair: %s",
		           strerror(errno));
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
jack_mgr_destroy(jack_mgr_t *jack_mgr)
{
	char dummy;

	jack_mgr->quit = 1;

	if (jack_deactivate(jack_mgr->jack_client) != 0) {
		lash_error("Could not deactivate JACK client", strerror(errno));
	}

	lash_debug("Stopping JACK manager");

	if (write(jack_mgr->callback_write_socket, &dummy, 1) == -1) {
		lash_error("Could not send data to JACK manager callback, "
		           "aborting: %s", strerror(errno));
		abort();
	}

	if (pthread_join(jack_mgr->callback_thread, NULL) != 0) {
		lash_error("Error joining callback thread: %s",
		           strerror(errno));
		return;
	}

	lash_debug("JACK manager stopped");

	jack_client_close(jack_mgr->jack_client);

/* FIXME: free all the data */

	free(jack_mgr);
}

void
jack_mgr_lock(jack_mgr_t *jack_mgr)
{
	pthread_mutex_lock(&jack_mgr->lock);
}

void
jack_mgr_unlock(jack_mgr_t *jack_mgr)
{
	pthread_mutex_unlock(&jack_mgr->lock);
}

/* caller must hold clients lock */
void
jack_mgr_check_client_ports(jack_mgr_t        *jack_mgr,
                            jack_mgr_client_t *client)
{
	lash_list_t *list;
	jack_fport_t *fport;

	for (list = jack_mgr->foreign_ports; list;) {
		fport = (jack_fport_t *) list->data;
		list = lash_list_next(list);

		if (strcmp(client->name, fport->client_name) == 0) {
			lash_debug("Resuming previously foreign port '%s'",
			           fport->name);
			jack_mgr_new_client_port(jack_mgr, fport->port_name,
			                         client);

			jack_mgr->foreign_ports =
			  lash_list_remove(jack_mgr->foreign_ports, fport);
			jack_fport_destroy(fport);
		}
	}
}

void
jack_mgr_add_client(jack_mgr_t  *jack_mgr,
                    uuid_t       id,
                    const char  *jack_client_name,
                    lash_list_t *jack_patches)
{
	jack_mgr_client_t *client;

	lash_debug("Adding client '%s'", jack_client_name);

	if ((client = jack_mgr_client_new())) {
		uuid_copy(client->id, id);
		lash_strset(&client->name, jack_client_name);
		client->old_patches = jack_patches;

		jack_mgr->clients = lash_list_append(jack_mgr->clients, client);

		/* check if it's registered some ports already */
		jack_mgr_check_client_ports(jack_mgr, client);
		lash_debug("Client added");
	}
	else
		lash_error("Failed to add client");
}

lash_list_t *
jack_mgr_remove_client(jack_mgr_t *jack_mgr,
                       uuid_t      id)
{
	jack_mgr_client_t *client;
	lash_list_t *backup_patches;

	lash_debug("Removing client");

	client = jack_mgr_client_find_by_id(jack_mgr->clients, id);
	if (!client) {
		lash_error("Unknown client");
		return NULL;
	}

	jack_mgr->clients = lash_list_remove(jack_mgr->clients, client);

	lash_debug("Removed client '%s'", client->name);

	backup_patches = client->backup_patches;
	client->backup_patches = NULL;

	jack_mgr_client_destroy(client);

	return backup_patches;
}

/* Return a list of the given client's patches. */
lash_list_t *
jack_mgr_get_client_patches(jack_mgr_t *jack_mgr,
                            uuid_t      id)
{
	return jack_mgr_client_dup_uniq_patches(jack_mgr->clients, id);
}

/* caller must hold client lock */
static void
jack_mgr_remove_foreign_port(jack_mgr_t     *jack_mgr,
                             jack_port_id_t  id)
{
	jack_fport_t *port;
	lash_list_t *list;

	for (list = jack_mgr->foreign_ports; list;
	     list = lash_list_next(list)) {
		port = (jack_fport_t *) list->data;

		if (id == port->id) {
			jack_mgr->foreign_ports =
			  lash_list_remove(jack_mgr->foreign_ports, port);

			lash_debug("removed foreign port '%s'", port->name);
			jack_fport_destroy(port);

			return;
		}
	}
}

static bool
jack_mgr_resume_patch(jack_mgr_t   *jack_mgr,
                      jack_patch_t *patch)
{
	if (jack_connect(jack_mgr->jack_client,
	                 patch->src_desc,
	                 patch->dest_desc) != 0) {
		lash_debug("Could not (yet?) resume patch '%s' -> '%s'",
		           patch->src_desc, patch->dest_desc);
		return false;
	}

	lash_info("Connected JACK port '%s' -> '%s'",
	          patch->src_desc, patch->dest_desc);

	return true;
}

/*
 * Here we have to go through each patch, see if it's the specified port,
 * unset it, see if we can connect it, and remove it.  Other clients will
 * have connections for this one if they're not registered yet.
 */
static void
jack_mgr_new_client_port(jack_mgr_t        *jack_mgr,
                         const char        *port,
                         jack_mgr_client_t *client)
{
	lash_list_t *node;
	jack_patch_t *patch, *unset_patch = NULL;

	for (node = client->old_patches; node;) {
		patch = (jack_patch_t *) node->data;
		node = lash_list_next(node);

		lash_debug("Checking patch '%s' -> '%s'",
		           patch->src_desc, patch->dest_desc);

		unset_patch = jack_patch_dup(patch);

		if (!jack_patch_unset(unset_patch, jack_mgr->clients)) {
			lash_debug("Could not resume jack patch '%s' -> '%s' "
			           "as the other client hasn't registered yet",
			           patch->src_desc, patch->dest_desc);
			client->old_patches =
			  lash_list_remove(client->old_patches, patch);
			jack_patch_destroy(patch);
			jack_patch_destroy(unset_patch);
			continue;
		}

		if ((strcmp(client->name, unset_patch->src_client) == 0
		     && strcmp(port, unset_patch->src_port) == 0)
		    || (strcmp(client->name, unset_patch->dest_client) == 0
		        && strcmp(port, unset_patch->dest_port) == 0)) {
			lash_debug("Attempting to resume patch '%s' -> '%s'",
			           unset_patch->src_desc,
			           unset_patch->dest_desc);
			if (jack_mgr_resume_patch(jack_mgr, unset_patch)) {
				client->old_patches =
				  lash_list_remove(client->old_patches, patch);
				jack_patch_destroy(patch);
			}
		}

		jack_patch_destroy(unset_patch);
	}
}

static void
jack_mgr_port_notification(jack_mgr_t     *jack_mgr,
                           jack_port_id_t  port_id)
{
	jack_mgr_client_t *client;
	jack_port_t *jack_port;
	char *client_name, *ptr;
	lash_list_t *list;

	/* get the client name */
	jack_port = jack_port_by_id(jack_mgr->jack_client, port_id);
	if (!jack_port) {
		lash_error("Received JACK port registration notification "
		           "for nonexistent port");
		return;
	}

	lash_debug("Received port registration notification for port %u",
	           port_id);

	client_name = lash_strdup(jack_port_name(jack_port));
	ptr = strchr(client_name, ':');
	if (!ptr) {
		lash_error("Port name '%s' contains no client name/port "
		           "name separator", client_name);
		return;
	}
	*ptr = '\0';

	for (list = jack_mgr->clients; list; list = lash_list_next(list)) {
		client = (jack_mgr_client_t *) list->data;

		if (strcmp(client->name, client_name) == 0)
			break;
	}

	if (!list) {
		jack_fport_t *fport;

		lash_debug("Adding foreign port '%s'",
		           jack_port_name(jack_port));
		fport = jack_fport_new(port_id, jack_port_name(jack_port));

		jack_mgr->foreign_ports =
		  lash_list_append(jack_mgr->foreign_ports,
		                   fport);

		return;
	}

	ptr++;
	jack_mgr_new_client_port(jack_mgr, ptr, client);

	free(client_name);
}

lash_list_t *
jack_mgr_get_patches_with_type(const char    *client_name,
                               jack_client_t *jack_client,
                               int            type_flags)
{
	jack_patch_t *patch;
	char *port_name_regex, **port_names, **connected_port_names;
	jack_port_t *port;
	int i, j;
	lash_list_t *list = NULL;

	port_name_regex = lash_malloc(1, strlen(client_name) + 4);
	sprintf(port_name_regex, "%s:.*", client_name);

	port_names =
	  (char **) jack_get_ports(jack_client, port_name_regex, NULL,
	                           type_flags);
	free(port_name_regex);

	if (!port_names)
		return NULL;

	for (i = 0; port_names[i]; i++) {
		port = jack_port_by_name(jack_client, port_names[i]);

		connected_port_names =
		  (char **) jack_port_get_all_connections(jack_client, port);

		if (!connected_port_names)
			continue;

		for (j = 0; connected_port_names[j]; j++) {
			patch = jack_patch_new();

			jack_patch_set_src(patch, port_names[i]);
			jack_patch_set_dest(patch, connected_port_names[j]);
			if (type_flags & JackPortIsInput)
				jack_patch_switch_clients(patch);

			list = lash_list_append(list, patch);
			lash_debug("Found patch: '%s' -> '%s'",
			           patch->src_desc, patch->dest_desc);
		}

/*      free (connected_port_names); */
	}

/*  free (port_names); */

	return list;
}

static void
jack_mgr_get_patches(jack_mgr_t        *jack_mgr,
                     jack_mgr_client_t *client)
{
	lash_list_t *patches;

	patches = jack_mgr_get_patches_with_type(client->name,
	                                         jack_mgr->jack_client,
	                                         JackPortIsInput);

	jack_mgr_client_free_patch_list(client->patches);
	client->patches = patches;

	patches = jack_mgr_get_patches_with_type(client->name,
	                                         jack_mgr->jack_client,
	                                         JackPortIsOutput);

	client->patches = lash_list_concat(client->patches, patches);
}

static void
jack_mgr_check_foreign_ports(jack_mgr_t *jack_mgr)
{
	lash_list_t *list;
	jack_fport_t *fport;
	jack_port_t *port;

	for (list = jack_mgr->foreign_ports; list;) {
		fport = (jack_fport_t *) list->data;
		list = lash_list_next(list);

		port = jack_port_by_id(jack_mgr->jack_client,
		                       fport->id);

		if (!port) {
			jack_mgr->foreign_ports =
			  lash_list_remove(jack_mgr->foreign_ports, fport);
			lash_debug("Removed foreign port '%s'", fport->name);

			jack_fport_destroy(fport);
		}
	}
}

static void
jack_mgr_graph_reorder_notification(jack_mgr_t *jack_mgr)
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
jack_mgr_read_callback(jack_mgr_t *jack_mgr)
{
	char port_notify, registered;
	jack_port_id_t port_id;
	int sock = jack_mgr->callback_read_socket;

	if (read(sock, &port_notify, 1) == -1) {
		lash_error("Could not read data from callback "
		           "socket, aborting: %s", strerror(errno));
		abort();
	}

	if (port_notify) {
		lash_debug("JACK port reg callback happened");

		if (read(sock, &port_id, sizeof(jack_port_id_t)) == -1
		    || read(sock, &registered, 1) == -1) {
			lash_error("Could not read data from callback "
			           "socket, aborting: %s", strerror(errno));
			abort();
		}

		if (registered)
			jack_mgr_port_notification(jack_mgr, port_id);
		else
			jack_mgr_remove_foreign_port(jack_mgr, port_id);
	} else {
		lash_debug("JACK graph reorder callback happened");

		jack_mgr_graph_reorder_notification(jack_mgr);
	}
}

static void
jack_mgr_backup_patches(jack_mgr_t *jack_mgr)
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

		jack_mgr_client_free_patch_list(client->backup_patches);
		client->backup_patches = jack_mgr_client_dup_patch_list(client->patches);

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
	jack_mgr_t *jack_mgr = (jack_mgr_t *) data;
	fd_set socket_set, select_set;
	struct timeval timeout;
	int sock = jack_mgr->callback_read_socket;

	FD_ZERO(&socket_set);
	FD_SET(sock, &socket_set);

	while (!jack_mgr->quit) {
		/* backup timeout */
		timeout.tv_sec = BACKUP_INTERVAL;
		timeout.tv_usec = 0;

		select_set = socket_set;

		if (select(sock + 1, &select_set, NULL, NULL, &timeout) == -1) {
			if (errno == EINTR)
				continue;

			lash_error("Error calling select(): %s",
			           strerror(errno));
		}

		if (jack_mgr->quit)
			break;

		jack_mgr_lock(jack_mgr);

		if (FD_ISSET(sock, &select_set))
			jack_mgr_read_callback(jack_mgr);

		jack_mgr_backup_patches(jack_mgr);

		jack_mgr_unlock(jack_mgr);
	}

	lash_debug("JACK manager callback finished");
	return NULL;
}

/* EOF */
