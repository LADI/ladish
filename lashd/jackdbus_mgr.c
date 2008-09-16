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

#include <stdbool.h>
#include <string.h>
#include <dbus/dbus.h>

#include "common/safety.h"
#include "common/debug.h"

#include "jackdbus_mgr.h"
#include "jack_patch.h"
#include "jack_mgr_client.h"
#include "server.h"
#include "client.h"

static lashd_jackdbus_mgr_t *g_jack_mgr_ptr = NULL;

static DBusHandlerResult
lashd_jackdbus_handler(DBusConnection *connection,
                       DBusMessage    *message,
                       void           *data);

static bool
lashd_jackdbus_mgr_get_client_data(jack_mgr_client_t *client);

lashd_jackdbus_mgr_t *
lashd_jackdbus_mgr_new(server_t *server)
{
	if (!server || !server->dbus_service
	    || !server->dbus_service->connection) {
		lash_error("NULL server pointer or no D-Bus connection");
		return NULL;
	}

	lashd_jackdbus_mgr_t *mgr;
	DBusError err;

	if (!(mgr = lash_calloc(1, sizeof(lashd_jackdbus_mgr_t)))) {
		lash_error("Failed to allocate memory for "
		           "lashd_jackdbus_mgr_t");
		return NULL;
	}

	dbus_error_init(&err);

	dbus_bus_add_match(server->dbus_service->connection,
			   "type='signal'"
			   ",sender='org.jackaudio.service'"
			   ",path='/org/jackaudio/Controller'"
			   ",interface='org.jackaudio.JackPatchbay'"
			   ",member='ClientAppeared'",
			   &err);
	if (dbus_error_is_set(&err)) {
		lash_error("Failed to add D-Bus match rule: %s", err.message);
		dbus_error_free(&err);
		goto fail;
	}

	dbus_bus_add_match(server->dbus_service->connection,
	                   "type='signal'"
	                   ",sender='org.jackaudio.service'"
	                   ",path='/org/jackaudio/Controller'"
	                   ",interface='org.jackaudio.JackPatchbay'"
	                   ",member='PortAppeared'",
	                   &err);
	if (dbus_error_is_set(&err)) {
		lash_error("Failed to add D-Bus match rule: %s", err.message);
		dbus_error_free(&err);
		goto fail;
	}

	dbus_bus_add_match(server->dbus_service->connection,
			   "type='signal'"
			   ",sender='org.jackaudio.service'"
			   ",path='/org/jackaudio/Controller'"
			   ",interface='org.jackaudio.JackPatchbay'"
			   ",member='PortsConnected'",
			   &err);
	if (dbus_error_is_set(&err)) {
		lash_error("Failed to add D-Bus match rule: %s", err.message);
		dbus_error_free(&err);
		goto fail;
	}

	if (!dbus_connection_add_filter(server->dbus_service->connection,
	                                lashd_jackdbus_handler,
	                                NULL, NULL)) {
		lash_error("Failed to add D-Bus filter");
		goto fail;
	}

	g_jack_mgr_ptr = mgr;
	return mgr;

fail:
	free(mgr);
	return NULL;
}

static void
lashd_jackdbus_mgr_graph_free(void)
{
	g_jack_mgr_ptr->graph_version = 0;

	if (g_jack_mgr_ptr->graph) {
		dbus_message_unref(g_jack_mgr_ptr->graph);
		g_jack_mgr_ptr->graph = NULL;
	}
}

void
lashd_jackdbus_mgr_destroy(lashd_jackdbus_mgr_t *mgr)
{
	if (mgr) {
		// TODO: also destroy the objects from the list
		lash_list_free(mgr->clients);
		lashd_jackdbus_mgr_graph_free();
		free(mgr);
		g_jack_mgr_ptr = NULL;
	}
}

static void
lashd_jackdbus_connect_return_handler(DBusPendingCall *pending,
                                      void            *data)
{
	DBusMessage *msg = dbus_pending_call_steal_reply(pending);

	if (msg) {
		const char *err_str;

		if (!method_return_verify(msg, &err_str))
			lash_error("Failed to connect ports: %s", err_str);
		else
			lash_debug("Ports connected");

		dbus_message_unref(msg);
	} else
		lash_error("Cannot get method return from pending call");

	dbus_pending_call_unref(pending);
}

static void
lashd_jackdbus_mgr_connect_ports(const char *client1_name,
                                 const char *port1_name,
                                 const char *client2_name,
                                 const char *port2_name)
{
	lash_info("Attempting to resume patch '%s:%s' -> '%s:%s'",
	           client1_name, port1_name, client2_name, port2_name);

	/* Send a port connect request */
	method_call_new_valist(g_server->dbus_service,
	                       NULL,
	                       lashd_jackdbus_connect_return_handler,
	                       false,
	                       "org.jackaudio.service",
	                       "/org/jackaudio/Controller",
	                       "org.jackaudio.JackPatchbay",
	                       "ConnectPortsByName",
	                       DBUS_TYPE_STRING,
	                       &client1_name,
	                       DBUS_TYPE_STRING,
	                       &port1_name,
	                       DBUS_TYPE_STRING,
	                       &client2_name,
	                       DBUS_TYPE_STRING,
	                       &port2_name,
	                       DBUS_TYPE_INVALID);
}

static
void
lashd_jackdbus_get_client_pid_return_handler(
	DBusPendingCall *pending,
	void            *data)
{
	DBusMessage *msg = dbus_pending_call_steal_reply(pending);

	if (msg) {
		const char *err_str;

		if (!method_return_verify(msg, &err_str))
			lash_error("Failed to get client pid: %s", err_str);
		else
		{
			DBusError err;
			dbus_error_init(&err);

			if (!dbus_message_get_args(msg, &err,
						   DBUS_TYPE_INT64,
						   data,
						   DBUS_TYPE_INVALID)) {
				lash_error("Cannot get message argument: %s", err.message);
				dbus_error_free(&err);
			}
		}

		dbus_message_unref(msg);
	} else
		lash_error("Cannot get method return from pending call");

	dbus_pending_call_unref(pending);
}

static
dbus_int64_t
lashd_jackdbus_get_client_pid(
	dbus_uint64_t client_id)
{
	dbus_int64_t pid = 0;

	if (!method_call_new_valist(
		g_server->dbus_service,
		&pid,
		lashd_jackdbus_get_client_pid_return_handler,
		true,
		"org.jackaudio.service",
		"/org/jackaudio/Controller",
		"org.jackaudio.JackPatchbay",
		"GetClientPID",
		DBUS_TYPE_UINT64,
		&client_id,
		DBUS_TYPE_INVALID))
	{
		return 0;
	}

	return pid;
}

static
void
lashd_jackdbus_on_client_appeared(
	const char * client_name,
	dbus_uint64_t client_id)
{
	dbus_int64_t pid;
	client_t * client_ptr;
	jack_mgr_client_t * jack_client_ptr;

	pid = lashd_jackdbus_get_client_pid(client_id);

	lash_info("client '%s' (%llu) appeared. pid is %lld", client_name, (unsigned long long)client_id, (long long)pid);

	client_ptr = server_find_client_by_pid(g_server, pid);
	lash_debug("client: %p", client_ptr);

	if (client_ptr == NULL)
	{
		/* TODO: we need to create liblash-less client object here */
		lash_info("Ignoring liblash-less client '%s'", client_name);
		return;
	}


	jack_client_ptr = jack_mgr_client_new();

	uuid_copy(jack_client_ptr->id, client_ptr->id);
	jack_client_ptr->name = lash_strdup(client_name);
	//client_ptr->jack_client_id = client_id;
	jack_client_ptr->old_patches = client_ptr->jack_patches;
	jack_client_ptr->backup_patches = jack_mgr_client_dup_patch_list(client_ptr->jack_patches);
	client_ptr->jack_patches = NULL;

	g_jack_mgr_ptr->clients = lash_list_append(g_jack_mgr_ptr->clients, jack_client_ptr);

	/* Get the graph and extract the new client's data */
	lashd_jackdbus_mgr_get_graph(g_jack_mgr_ptr);
	if (!lashd_jackdbus_mgr_get_client_data(jack_client_ptr))
		lash_error("Problem extracting client data from graph");

	lash_debug("Client added");
}

static void
lashd_jackdbus_mgr_new_foreign_port(const char *client_name,
                                    const char *port_name)
{
	lash_debug("new foreign port '%s:%s'", client_name, port_name);

	lash_list_t *node, *node2;
	jack_mgr_client_t *client;
	jack_patch_t *patch;
	const char *foreign_client, *foreign_port;
	bool port_is_input;
	const char *client1_name, *port1_name, *client2_name, *port2_name;

	for (node = g_jack_mgr_ptr->clients; node;
	     node = lash_list_next(node)) {
		client = node->data;

		for (node2 = client->old_patches; node2;) {
		     patch = node2->data;
		     node2 = lash_list_next(node2);

			if (uuid_compare(patch->src_client_id,
			                 client->id) == 0) {
				if (!patch->dest_client)
					continue;

				foreign_client = patch->dest_client;
				foreign_port = patch->dest_port;
				port_is_input = false;
			} else {
				if (!patch->src_client)
					continue;

				foreign_client = patch->src_client;
				foreign_port = patch->src_port;
				port_is_input = true;
			}

			if (strcmp(foreign_client, client_name) == 0
			    && strcmp(foreign_port, port_name) == 0) {

				if (port_is_input) {
					client1_name = foreign_client;
					port1_name = foreign_port;
					client2_name = client->name;
					port2_name = patch->dest_port;
				} else {
					client1_name = client->name;
					port1_name = patch->src_port;
					client2_name = foreign_client;
					port2_name = foreign_port;
				}

				lashd_jackdbus_mgr_connect_ports(client1_name,
				                                 port1_name,
				                                 client2_name,
				                                 port2_name);
			}
		}
	}
}

static void
lashd_jackdbus_mgr_new_client_port(jack_mgr_client_t *client,
                                   const char        *client_name,
                                   const char        *port_name)
{
	lash_info("new client port '%s:%s'", client_name, port_name);

	jack_mgr_client_t *other_client;
	lash_list_t *node;
	jack_patch_t *patch;
	bool port_is_input;
	uuid_t *other_id;
	const char *client1_name, *port1_name, *client2_name, *port2_name;

	/* Iterate the client's old patches and try to connect those which
	   contain the new port */
	for (node = client->old_patches; node;) {
		patch = (jack_patch_t *) node->data;
		node = lash_list_next(node);

		/* See which port the client owns, and check
		   whether it's the one that just appeared */
		if (uuid_compare(patch->src_client_id, client->id) == 0) {
			/* The client owns the patch's source port */
			if (strcmp(patch->src_port, port_name) == 0) {
				/* New port is the patch's source port */
				client1_name = client->name;
				port1_name = port_name;
				port2_name = patch->dest_port;
				other_id = &patch->dest_client_id;
				port_is_input = false;
			} else /* Not the correct port */
				continue;
		} else {
			/* The client owns the patch's destination port */
			if (strcmp(patch->dest_port, port_name) == 0) {
				/* New port is the patch's destination port */
				client2_name = client->name;
				port1_name = patch->src_port;
				port2_name = port_name;
				other_id = &patch->src_client_id;
				port_is_input = true;
			} else /* Not the correct port */
				continue;
		}

		/* Check if the patch's other port belongs to a client */
		if (!uuid_is_null(*other_id))
			other_client = jack_mgr_client_find_by_id(g_jack_mgr_ptr->clients,
			                                          *other_id);
		else
			other_client = NULL;

		/* Set the correct name for the other client. If the name is
		   NULL it means that the other client isn't ready yet. */
		if (port_is_input) {
			client1_name = other_client ? other_client->name
			                            : patch->src_client;
			if (!client1_name)
				continue;
		} else {
			client2_name = other_client ? other_client->name
			                            : patch->dest_client;
			if (!client2_name)
				continue;
		}

		lashd_jackdbus_mgr_connect_ports(client1_name, port1_name,
		                                 client2_name, port2_name);
	}
}

static void
lashd_jackdbus_mgr_check_patches(jack_mgr_client_t *client,
                                 dbus_uint64_t      client1_id,
                                 const char        *client1_name,
                                 const char        *port1_name,
                                 dbus_uint64_t      client2_id,
                                 const char        *client2_name,
                                 const char        *port2_name);

static void
lashd_jackdbus_mgr_ports_connected(dbus_uint64_t  client1_id,
                                   const char    *client1_name,
                                   const char    *port1_name,
                                   dbus_uint64_t  client2_id,
                                   const char    *client2_name,
                                   const char    *port2_name)
{
	jack_mgr_client_t *client;

	client = jack_mgr_client_find_by_jackdbus_id(g_jack_mgr_ptr->clients,
	                                             client1_id);
	if (client)
		lashd_jackdbus_mgr_check_patches(client, client1_id,
		                                 client1_name, port1_name,
		                                 client2_id, client2_name,
		                                 port2_name);

	client = jack_mgr_client_find_by_jackdbus_id(g_jack_mgr_ptr->clients,
	                                             client2_id);
	if (client)
		lashd_jackdbus_mgr_check_patches(client, client1_id,
		                                 client1_name, port1_name,
		                                 client2_id, client2_name,
		                                 port2_name);
}

static DBusHandlerResult
lashd_jackdbus_handler(DBusConnection *connection,
                       DBusMessage    *message,
                       void           *data)
{
	/* If the message isn't a signal the object path handler may use it */
	if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	const char *member, *client1_name, *port1_name;
	dbus_uint64_t dummy, client1_id;
	DBusError err;

	if (!(member = dbus_message_get_member(message))) {
		lash_error("Received JACK signal with NULL member");
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	dbus_error_init(&err);

	if (strcmp(member, "ClientAppeared") == 0) {
		jack_mgr_client_t *client;

		lash_debug("Received ClientAppeared signal");

		if (!dbus_message_get_args(message, &err,
		                           DBUS_TYPE_UINT64, &dummy,
		                           DBUS_TYPE_UINT64, &client1_id,
		                           DBUS_TYPE_STRING, &client1_name,
		                           DBUS_TYPE_INVALID)) {
			goto fail;
		}

		lashd_jackdbus_on_client_appeared(client1_name, client1_id);
	} else if (strcmp(member, "PortAppeared") == 0) {
		jack_mgr_client_t *client;

		lash_debug("Received PortAppeared signal");

		if (!dbus_message_get_args(message, &err,
		                           DBUS_TYPE_UINT64, &dummy,
		                           DBUS_TYPE_UINT64, &client1_id,
		                           DBUS_TYPE_STRING, &client1_name,
		                           DBUS_TYPE_UINT64, &dummy,
		                           DBUS_TYPE_STRING, &port1_name,
		                           DBUS_TYPE_INVALID)) {
			goto fail;
		}

		/* Check if the new port belongs to a known client */
		client = jack_mgr_client_find_by_jackdbus_id(g_jack_mgr_ptr->clients,
			                                             client1_id);
		if (client && client->old_patches)
			lashd_jackdbus_mgr_new_client_port(client,
			                                   client1_name,
			                                   port1_name);
		else
			lashd_jackdbus_mgr_new_foreign_port(client1_name,
			                                    port1_name);

	} else if (strcmp(member, "PortsConnected") == 0) {
		const char *client2_name, *port2_name;
		dbus_uint64_t client2_id;

		lash_debug("Received PortsConnected signal");

		if (!dbus_message_get_args(message, &err,
		                           DBUS_TYPE_UINT64, &dummy,
		                           DBUS_TYPE_UINT64, &client1_id,
		                           DBUS_TYPE_STRING, &client1_name,
		                           DBUS_TYPE_UINT64, &dummy,
		                           DBUS_TYPE_STRING, &port1_name,
		                           DBUS_TYPE_UINT64, &client2_id,
		                           DBUS_TYPE_STRING, &client2_name,
		                           DBUS_TYPE_UINT64, &dummy,
		                           DBUS_TYPE_STRING, &port2_name,
		                           DBUS_TYPE_INVALID)) {
			goto fail;
		}

		lashd_jackdbus_mgr_ports_connected(client1_id,
		                                   client1_name,
		                                   port1_name,
		                                   client2_id,
		                                   client2_name,
		                                   port2_name);
	}

	return DBUS_HANDLER_RESULT_HANDLED;

fail:
	lash_error("Cannot get message arguments: %s",
	           err.message);
	dbus_error_free(&err);

	return DBUS_HANDLER_RESULT_HANDLED;
}

// TODO: This is messy, there's gotta be something that can be done. This is
//       also used by lashd_jackdbus_mgr_ports_connected(), and there's some
//       redundancy going on.
static void
lashd_jackdbus_mgr_check_patches(jack_mgr_client_t *client,
                                 dbus_uint64_t      client1_id,
                                 const char        *client1_name,
                                 const char        *port1_name,
                                 dbus_uint64_t      client2_id,
                                 const char        *client2_name,
                                 const char        *port2_name)
{
	bool port_is_input;
	dbus_uint64_t other_id;
	jack_mgr_client_t *other_client;
	const char *other_client_name;
	const char *this_port_name, *other_port_name;
	lash_list_t *node;
	jack_patch_t *patch;
	uuid_t *uuid_ptr;
	char *name, *this_port, *that_port;

	if (client1_id == client->jackdbus_id) {
		if (client2_id == client->jackdbus_id)
			/* Patch is an internal connection */
			return;

		/* An output port patch */
		other_id = client2_id;
		this_port_name = port1_name;
		other_port_name = port2_name;
		port_is_input = false;
	} else if (client2_id == client->jackdbus_id) {
		/* An input port patch */
		other_id = client1_id;
		this_port_name = port2_name;
		other_port_name = port1_name;
		port_is_input = true;
	} else
		/* Patch does not involve the client */
		return;

	// TODO: Perhaps find out the "other client" beforehand and pass its
	//       pointer to us?
	other_client =
	  jack_mgr_client_find_by_jackdbus_id(g_jack_mgr_ptr->clients,
	                                      other_id);
	if (other_client) {
		other_client_name = NULL;
	} else {
		if (port_is_input)
			other_client_name = client1_name;
		else
			other_client_name = client2_name;
	}

	for (node = client->old_patches; node;) {
		patch = (jack_patch_t *) node->data;
		node = lash_list_next(node);

		uuid_ptr = NULL;
		name = NULL;

		if (port_is_input) {
			this_port = patch->dest_port;
			that_port = patch->src_port;
			if (patch->src_client)
				name = patch->src_client;
			else
				uuid_ptr = &patch->src_client_id;
		} else {
			this_port = patch->src_port;
			that_port = patch->dest_port;
			if (patch->dest_client)
				name = patch->dest_client;
			else
				uuid_ptr = &patch->dest_client_id;
		}

		if (other_client) {
			if (!uuid_ptr || uuid_compare(*uuid_ptr,
			                              other_client->id) != 0)
				continue;
		} else if (!name || strcmp(name, other_client_name) != 0)
			continue;

		if (strcmp(this_port, this_port_name) != 0
		    || strcmp(that_port, other_port_name) != 0)
			continue;

		lash_debug("Connected patch '%s:%s' -> '%s:%s' is an old patch "
		           "of client '%s'; removing from list", client1_name,
		           port1_name, client2_name, port2_name, client->name);

		client->old_patches = lash_list_remove(client->old_patches,
		                                       patch);
		jack_patch_destroy(patch);
#ifdef LASH_DEBUG
		if (!client->old_patches)
			lash_debug("All old patches of client '%s' have "
			           "now been connected", client->name);
#endif
		break;
	}
}

static bool
lashd_jackdbus_mgr_get_client_data(jack_mgr_client_t *client)
{
	if (!client) {
		lash_error("Client pointer is NULL");
		return false;
	}

	lash_debug("Getting data from graph for client '%s'", client->name);

	DBusMessageIter iter, array_iter, struct_iter;
	dbus_uint64_t client1_id, client2_id;
	const char *client1_name, *port1_name, *client2_name, *port2_name;

	if (!g_jack_mgr_ptr->graph) {
		lash_error("Cannot find graph");
		return false;
	}

	/* The graph message has already been checked, this should be safe */
	dbus_message_iter_init(g_jack_mgr_ptr->graph, &iter);

	/* Skip over the graph version argument */
	dbus_message_iter_next(&iter);

	/* Check that we're getting the client array as expected */
	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
		lash_error("Cannot find client array in graph");
		goto fail;
	}

	/* Iterate the client array and grab the client's ID */
	dbus_message_iter_recurse(&iter, &array_iter);
	client->jackdbus_id = 0;
	while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_STRUCT) {
		dbus_message_iter_recurse(&array_iter, &struct_iter);

		if (!method_iter_get_args(&struct_iter,
		                          DBUS_TYPE_UINT64, &client1_id,
		                          DBUS_TYPE_STRING, &client1_name,
		                          DBUS_TYPE_INVALID)) {
			lash_error("Failed to parse client array in graph");
			goto fail;
		}

		/* See if we found the client */
		if (strcmp(client1_name, client->name) == 0) {
			lash_debug("Assigning jackdbus ID %llu to client '%s'",
			           client1_id, client1_name);
			client->jackdbus_id = client1_id;
			break;
		}

		dbus_message_iter_next(&array_iter);
	}

	if (!client->jackdbus_id) {
		lash_error("Cannot find client jackdbus ID in graph");
		goto fail;
	}

	if (!client->old_patches) {
		lash_debug("Client '%s' has no old patches to check",
		           client->name);
		return true;
	}

	/* Check that we're getting the port array as expected */
	if (dbus_message_iter_get_arg_type(&struct_iter) != DBUS_TYPE_ARRAY) {
		lash_error("Cannot find port array for client '%s' in graph",
		           client1_name);
		goto fail;
	}

	/* Iterate the client's port array and connect pending old_patches */
	dbus_message_iter_recurse(&struct_iter, &array_iter);
	while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_STRUCT) {
		dbus_message_iter_recurse(&array_iter, &struct_iter);

		if (!method_iter_get_args(&struct_iter,
		                          DBUS_TYPE_UINT64, NULL,
		                          DBUS_TYPE_STRING, &port1_name,
		                          DBUS_TYPE_INVALID)) {
			lash_error("Failed to parse port array for client '%s' in graph",
			           client1_name);
			goto fail;
		}

		//lashd_jackdbus_mgr_new_client_port(client, client1_name, port1_name);

		dbus_message_iter_next(&array_iter);
	}

	/* Check that we're getting the patch array as expected */
	if (!dbus_message_iter_next(&iter)
	    || dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
		lash_error("Cannot find patch array in graph");
		goto fail;
	}

	/* Iterate the patch array and grab the new client's patches */
	dbus_message_iter_recurse(&iter, &array_iter);
	while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_STRUCT) {
		dbus_message_iter_recurse(&array_iter, &struct_iter);

		if (!method_iter_get_args(&struct_iter,
		                          DBUS_TYPE_UINT64, &client1_id,
		                          DBUS_TYPE_STRING, &client1_name,
		                          DBUS_TYPE_UINT64, NULL,
		                          DBUS_TYPE_STRING, &port1_name,
		                          DBUS_TYPE_UINT64, &client2_id,
		                          DBUS_TYPE_STRING, &client2_name,
		                          DBUS_TYPE_UINT64, NULL,
		                          DBUS_TYPE_STRING, &port2_name,
		                          DBUS_TYPE_INVALID)) {
			lash_error("Failed to parse patch array in graph");
			goto fail;
		}

		lashd_jackdbus_mgr_check_patches(client, client1_id,
		                                 client1_name, port1_name,
		                                 client2_id, client2_name,
		                                 port2_name);

		dbus_message_iter_next(&array_iter);
	}

	return true;

fail:
	/* If we're here there was something rotten in the graph message,
	   we should remove it */
	lashd_jackdbus_mgr_graph_free();
	return false;
}

void
lashd_jackdbus_mgr_remove_client(lashd_jackdbus_mgr_t  *mgr,
                                 uuid_t                 id,
                                 lash_list_t          **backup_patches)
{
	jack_mgr_client_t *client;

	client = jack_mgr_client_find_by_id(mgr->clients, id);
	if (!client) {
		lash_error("Unknown client");
		return;
	}

	lash_debug("Removing client '%s'", client->name);

	mgr->clients = lash_list_remove(mgr->clients, client);

	if (backup_patches) {
		*backup_patches = client->backup_patches;
		client->backup_patches = NULL;
	}

	jack_mgr_client_destroy(client);
}

/* Return a list of the given client's patches. */
lash_list_t *
lashd_jackdbus_mgr_get_client_patches(lashd_jackdbus_mgr_t *mgr,
                                      uuid_t                id)
{
	if (!mgr) {
		lash_error("JACK manager pointer is NULL");
		return NULL;
	}

	jack_mgr_client_t *client, *other_client;
	jack_patch_t *patch;
	lash_list_t *patches = NULL;
	DBusMessageIter iter, array_iter, struct_iter;
	dbus_uint64_t client1_id, client2_id, other_id;
	const char *client1_name, *port1_name, *client2_name, *port2_name, *other_name;
	bool port_is_input;
	uuid_t *client1_uuid, *client2_uuid, *other_uuid;

	if (!mgr->graph) {
		lash_error("Cannot find graph");
		return NULL;
	}

	client = jack_mgr_client_find_by_id(mgr->clients, id);
	if (!client) {
		char id_str[37];
		uuid_unparse(id, id_str);
		lash_error("Cannot find client %s", id_str);
		return NULL;
	}

	lash_debug("Getting patches for client '%s'", client->name);

	/* The graph message has already been checked, this should be safe */
	dbus_message_iter_init(mgr->graph, &iter);

	/* Skip over the graph version and client array arguments */
	dbus_message_iter_next(&iter);
	dbus_message_iter_next(&iter);

	/* Check that we're getting the patch array as expected */
	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
		lash_error("Cannot find patch array in graph");
		goto fail;
	}

	/* Iterate the patch array and get the client's patches */
	dbus_message_iter_recurse(&iter, &array_iter);
	while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_STRUCT) {
		dbus_message_iter_recurse(&array_iter, &struct_iter);

		/* Get the patch description  */
		if (!method_iter_get_args(&struct_iter,
		                          DBUS_TYPE_UINT64, &client1_id,
		                          DBUS_TYPE_STRING, &client1_name,
		                          DBUS_TYPE_UINT64, NULL,
		                          DBUS_TYPE_STRING, &port1_name,
		                          DBUS_TYPE_UINT64, &client2_id,
		                          DBUS_TYPE_STRING, &client2_name,
		                          DBUS_TYPE_UINT64, NULL,
		                          DBUS_TYPE_STRING, &port2_name,
		                          DBUS_TYPE_INVALID)) {
			lash_error("Failed to parse patch array in graph");
			goto fail;
		}

		//lash_info("Connection %s:%s -> %s:%s", client1_name, port1_name, client2_name, port2_name);

		/* Skip unwanted patches, deduce port direction */
		if (client1_id == client->jackdbus_id) {
			if (client2_id == client->jackdbus_id) {
				/* Patch is an internal connection */
				dbus_message_iter_next(&array_iter);
				continue;
			} else {
				/* Client owns the patch's output port */
				port_is_input = false;
				other_id = client2_id;
			}
		} else if (client2_id != client->jackdbus_id) {
			/* Patch does not involve the client */
			dbus_message_iter_next(&array_iter);
			//lash_info("skipping internal connection");
			continue;
		} else {
			/* Client owns the patch's input port */
			port_is_input = true;
			other_id = client1_id;
		}

		other_client =
		  jack_mgr_client_find_by_jackdbus_id(mgr->clients, other_id);

		if (other_client) {
			other_uuid = &other_client->id;
			other_name = NULL;
		} else {
			other_uuid = NULL;
			other_name = port_is_input ? client1_name
			                           : client2_name;
		}

		if (port_is_input) {
			client1_name = other_name;
			client1_uuid = other_uuid;
			client2_name = client->name;
			client2_uuid = &client->id;
		} else {
			client1_name = client->name;
			client1_uuid = &client->id;
			client2_name = other_name;
			client2_uuid = other_uuid;
		}

		patch = jack_patch_new();

		if (client1_uuid)
			uuid_copy(patch->src_client_id, *client1_uuid);
		else
			patch->src_client = lash_strdup(client1_name);

		if (client2_uuid)
			uuid_copy(patch->dest_client_id, *client2_uuid);
		else
			patch->dest_client = lash_strdup(client2_name);

		patch->src_port = lash_strdup(port1_name);
		patch->dest_port = lash_strdup(port2_name);

		//lash_info("Adding patch %s:%s -> %s:%s", patch->src_client, patch->src_port, patch->dest_client, patch->dest_port);

		patches = lash_list_append(patches, patch);

		dbus_message_iter_next(&array_iter);
	}

	/* Make a fresh backup of the newly acquired patch list */
	jack_mgr_client_free_patch_list(client->backup_patches);
	client->backup_patches = (patches
	                          ? jack_mgr_client_dup_patch_list(patches)
	                          : NULL);

	return patches;

fail:
	/* If we're here there was something rotten in the graph message,
	   we should remove it */
	lashd_jackdbus_mgr_graph_free();
	return NULL;

}

static void
lashd_jackdbus_mgr_graph_return_handler(DBusPendingCall *pending,
                                        void            *data)
{
	lash_debug("Saving graph");

	DBusMessage *msg;
	DBusMessageIter iter;
	dbus_uint64_t graph_version;
	const char *err_str;

	msg = dbus_pending_call_steal_reply(pending);

	/* Check that the message is valid */
	if (!msg) {
		lash_error("Cannot get method return from pending call");
		goto end;
	} else if (!method_return_verify(msg, &err_str)) {
		lash_error("Failed to get graph: %s", err_str);
		goto end_unref_msg;
	} else if (!dbus_message_iter_init(msg, &iter)) {
		lash_error("Method return has no arguments");
		goto end_unref_msg;
	}

	/* Check that the graph version argument is valid */
	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_UINT64) {
		lash_error("Cannot find graph version in graph");
		goto end_unref_msg;
	}

	/* Get the graph version and see if we already have the latest one */
	dbus_message_iter_get_basic(&iter, &graph_version);
	if (graph_version == g_jack_mgr_ptr->graph_version) {
		lash_debug("Current graph is already the latest version");
		goto end_unref_msg;
	}

	/* Free the old graph and save the new one */
	lashd_jackdbus_mgr_graph_free();
	g_jack_mgr_ptr->graph = msg;
	g_jack_mgr_ptr->graph_version = graph_version;
	lash_debug("Graph saved");

end:
	dbus_pending_call_unref(pending);
	return;

end_unref_msg:
	dbus_message_unref(msg);
	dbus_pending_call_unref(pending);
}

void
lashd_jackdbus_mgr_get_graph(lashd_jackdbus_mgr_t *mgr)
{
	if (!mgr) {
		lash_error("JACK manager pointer is NULL");
		return;
	}

	lash_debug("Requesting graph version >= %llu", mgr->graph_version);

	method_call_new_single(g_server->dbus_service,
	                       NULL,
	                       lashd_jackdbus_mgr_graph_return_handler,
	                       true,
	                       "org.jackaudio.service",
	                       "/org/jackaudio/Controller",
	                       "org.jackaudio.JackPatchbay",
	                       "GetGraph",
	                       DBUS_TYPE_UINT64,
	                       &mgr->graph_version);
}

/* EOF */
