/*
 *   LASH
 *
 *   Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
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

#include <string.h>
#include <dbus/dbus.h>

#include "common/safety.h"
#include "common/debug.h"

#include "jackdbus_mgr.h"
#include "jack_patch.h"
#include "jack_mgr_client.h"
#include "server.h"
#include "client.h"
#include "project.h"

#define JACKDBUS_SERVICE         "org.jackaudio.service"
#define JACKDBUS_OBJECT          "/org/jackaudio/Controller"
#define JACKDBUS_IFACE_CONTROL   "org.jackaudio.JackControl"
#define JACKDBUS_IFACE_PATCHBAY  "org.jackaudio.JackPatchbay"

static lashd_jackdbus_mgr_t *g_jack_mgr_ptr = NULL;

static DBusHandlerResult
lashd_jackdbus_handler(DBusConnection *connection,
                       DBusMessage    *message,
                       void           *data);

static void
lashd_jackdbus_mgr_get_unknown_clients(lashd_jackdbus_mgr_t *mgr);

static bool
lashd_jackdbus_mgr_get_client_data(jack_mgr_client_t *client);

lashd_jackdbus_mgr_t *
lashd_jackdbus_mgr_new(void)
{
	if (!g_server || !g_server->dbus_service
	    || !g_server->dbus_service->connection) {
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

	dbus_bus_add_match(g_server->dbus_service->connection,
			   "type='signal'"
			   ",sender='" JACKDBUS_SERVICE "'"
			   ",path='" JACKDBUS_OBJECT "'"
			   ",interface='" JACKDBUS_IFACE_PATCHBAY "'"
			   ",member='ClientAppeared'",
			   &err);
	if (dbus_error_is_set(&err)) {
		lash_error("Failed to add D-Bus match rule: %s", err.message);
		dbus_error_free(&err);
		goto fail;
	}

	dbus_bus_add_match(g_server->dbus_service->connection,
	                   "type='signal'"
	                   ",sender='" JACKDBUS_SERVICE "'"
	                   ",path='" JACKDBUS_OBJECT "'"
	                   ",interface='" JACKDBUS_IFACE_PATCHBAY "'"
	                   ",member='ClientDisappeared'",
	                   &err);
	if (dbus_error_is_set(&err)) {
		lash_error("Failed to add D-Bus match rule: %s", err.message);
		dbus_error_free(&err);
		goto fail;
	}

	dbus_bus_add_match(g_server->dbus_service->connection,
	                   "type='signal'"
	                   ",sender='" JACKDBUS_SERVICE "'"
	                   ",path='" JACKDBUS_OBJECT "'"
	                   ",interface='" JACKDBUS_IFACE_PATCHBAY "'"
	                   ",member='PortAppeared'",
	                   &err);
	if (dbus_error_is_set(&err)) {
		lash_error("Failed to add D-Bus match rule: %s", err.message);
		dbus_error_free(&err);
		goto fail;
	}

	dbus_bus_add_match(g_server->dbus_service->connection,
			   "type='signal'"
			   ",sender='" JACKDBUS_SERVICE "'"
			   ",path='" JACKDBUS_OBJECT "'"
			   ",interface='" JACKDBUS_IFACE_PATCHBAY "'"
			   ",member='PortsConnected'",
			   &err);
	if (dbus_error_is_set(&err)) {
		lash_error("Failed to add D-Bus match rule: %s", err.message);
		dbus_error_free(&err);
		goto fail;
	}

	dbus_bus_add_match(g_server->dbus_service->connection,
			   "type='signal'"
			   ",sender='" JACKDBUS_SERVICE "'"
			   ",path='" JACKDBUS_OBJECT "'"
			   ",interface='" JACKDBUS_IFACE_PATCHBAY "'"
			   ",member='PortsDisconnected'",
			   &err);
	if (dbus_error_is_set(&err)) {
		lash_error("Failed to add D-Bus match rule: %s", err.message);
		dbus_error_free(&err);
		goto fail;
	}

	dbus_bus_add_match(
		g_server->dbus_service->connection,
		"type='signal',interface='" DBUS_INTERFACE_DBUS "',member=NameOwnerChanged,arg0='" JACKDBUS_SERVICE "'",
		&err);
	if (dbus_error_is_set(&err)) {
		lash_error("Failed to add D-Bus match rule: %s", err.message);
		dbus_error_free(&err);
		goto fail;
	}

	dbus_bus_add_match(g_server->dbus_service->connection,
			   "type='signal'"
			   ",sender='" JACKDBUS_SERVICE "'"
			   ",path='" JACKDBUS_OBJECT "'"
			   ",interface='" JACKDBUS_IFACE_CONTROL "'"
			   ",member='ServerStarted'",
			   &err);
	if (dbus_error_is_set(&err)) {
		lash_error("Failed to add D-Bus match rule: %s", err.message);
		dbus_error_free(&err);
		goto fail;
	}

	dbus_bus_add_match(g_server->dbus_service->connection,
			   "type='signal'"
			   ",sender='" JACKDBUS_SERVICE "'"
			   ",path='" JACKDBUS_OBJECT "'"
			   ",interface='" JACKDBUS_IFACE_CONTROL "'"
			   ",member='ServerStopped'",
			   &err);
	if (dbus_error_is_set(&err)) {
		lash_error("Failed to add D-Bus match rule: %s", err.message);
		dbus_error_free(&err);
		goto fail;
	}

	if (!dbus_connection_add_filter(g_server->dbus_service->connection,
	                                lashd_jackdbus_handler,
	                                NULL, NULL)) {
		lash_error("Failed to add D-Bus filter");
		goto fail;
	}

	INIT_LIST_HEAD(&mgr->clients);
	INIT_LIST_HEAD(&mgr->unknown_clients);
	g_jack_mgr_ptr = mgr;

	/* Get list of unknown JACK clients */
	lashd_jackdbus_mgr_get_graph(mgr);
	lashd_jackdbus_mgr_get_unknown_clients(mgr);

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

/** Return handler for GetClientPid method call in @ref lashd_jackdbus_get_client_pid.
 * @param pending Pointer to D-Bus pending call.
 * @param data Pointer to memory location in which to store the PID.
 */
static void
lashd_jackdbus_get_client_pid_return_handler(DBusPendingCall *pending,
                                             void            *data)
{
	DBusMessage *msg = dbus_pending_call_steal_reply(pending);

	if (msg) {
		const char *err_str;

		if (!method_return_verify(msg, &err_str))
			lash_error("Failed to get client pid: %s", err_str);
		else {
			DBusError err;
			dbus_error_init(&err);

			if (!dbus_message_get_args(msg, &err,
			                           DBUS_TYPE_INT64, data,
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

/** Get the PID of the JACK client whose JACK ID matches @a client_id.
 * @param client_id ID of JACK client whose PID to get.
 * @return PID of JACK client whose ID matches @a client_id, or 0 if no such
 *         client exists.
 */
static dbus_int64_t
lashd_jackdbus_get_client_pid(dbus_uint64_t client_id)
{
	dbus_int64_t pid;

	if (!method_call_new_valist(g_server->dbus_service, &pid,
	                            lashd_jackdbus_get_client_pid_return_handler,
	                            true,
	                            JACKDBUS_SERVICE,
	                            JACKDBUS_OBJECT,
	                            JACKDBUS_IFACE_PATCHBAY,
	                            "GetClientPID",
	                            DBUS_TYPE_UINT64, &client_id,
	                            DBUS_TYPE_INVALID))
		pid = 0;

	return pid;
}

/** Fill @a mgr 's unknown_clients list with one @a jack_mgr_client_t object per
 * each currently running JACK client. This is done in @a lashd_jackdbus_mgr_new
 * for @a mgr to be able to later on match JACK client PIDs against LASH clients.
 * @a mgr must be properly initialized, contain a valid graph, and its
 * unknown_clients list must be empty.
 * @param mgr Pointer to JACK D-Bus manager.
 */
static void
lashd_jackdbus_mgr_get_unknown_clients(lashd_jackdbus_mgr_t *mgr)
{
	lash_debug("Getting unknown JACK clients from graph");

	DBusMessageIter iter, array_iter, struct_iter;
	dbus_uint64_t client_id;
	const char *client_name;
	jack_mgr_client_t *jack_client;

	if (!mgr->graph) {
		lash_error("Cannot find graph");
		return;
	}

	/* The graph message has already been checked, this should be safe */
	dbus_message_iter_init(mgr->graph, &iter);

	/* Skip over the graph version argument */
	dbus_message_iter_next(&iter);

	/* Check that we're getting the client array as expected */
	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
		lash_error("Cannot find client array in graph");
		goto fail;
	}

	/* Iterate the client array and store clients as unknown */
	dbus_message_iter_recurse(&iter, &array_iter);
	while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_STRUCT) {
		dbus_message_iter_recurse(&array_iter, &struct_iter);

		/* Get client's data from array */
		if (!method_iter_get_args(&struct_iter,
		                          DBUS_TYPE_UINT64, &client_id,
		                          DBUS_TYPE_STRING, &client_name,
		                          DBUS_TYPE_INVALID)) {
			lash_error("Failed to parse client array in graph");
			goto fail;
		}

		/* Create JACK client and store in mgr->unknown_clients */
		jack_client = jack_mgr_client_new();
		jack_client->name = lash_strdup(client_name);
		jack_client->jackdbus_id = client_id;
		jack_client->pid = (pid_t) lashd_jackdbus_get_client_pid(client_id);
		lash_debug("Storing unknown JACK client '%s'", client_name);
		list_add_tail(&jack_client->siblings, &mgr->unknown_clients);

		dbus_message_iter_next(&array_iter);
	}

	return;

fail:
	/* If we're here there was something rotten in the graph message,
	   we should remove it */
	lashd_jackdbus_mgr_graph_free();
}

void
lashd_jackdbus_mgr_destroy(lashd_jackdbus_mgr_t *mgr)
{
	if (mgr) {
		// TODO: destroy mgr->clients
		struct list_head *node, *next;
		list_for_each_safe (node, next, &mgr->unknown_clients)
			jack_mgr_client_destroy(list_entry(node, jack_mgr_client_t, siblings));
		lashd_jackdbus_mgr_graph_free();
		free(mgr);
		g_jack_mgr_ptr = NULL;
	}
}

void
lashd_jackdbus_mgr_bind_client(jack_mgr_client_t *client,
                               client_t          *lash_client)
{
	lash_debug("Associating previously unknown JACK client '%s' with '%s'",
	           client->name, lash_client->name);

	/* Unlink JACK client from its current list, if any */
	list_del(&client->siblings);

	/* Copy UUID and move JACK patches from LASH client to JACK client */
	uuid_copy(client->id, lash_client->id);
	jack_mgr_client_free_patch_list(&client->old_patches);
	INIT_LIST_HEAD(&client->old_patches);
	list_splice_init(&lash_client->jack_patches, &client->old_patches);
	jack_mgr_client_dup_patch_list(&client->old_patches, &client->backup_patches);

	/* Add JACK client to the active client list */
	list_add_tail(&client->siblings, &g_jack_mgr_ptr->clients);

	/* Extract JACK client's data from latest graph */
	lashd_jackdbus_mgr_get_graph(g_jack_mgr_ptr);
	if (!lashd_jackdbus_mgr_get_client_data(client))
		lash_error("Problem extracting client data from graph");

	/* Copy JACK client name to LASH client, make sure it has a class string */
	lash_strset(&lash_client->jack_client_name, client->name);
	client_maybe_fill_class(lash_client);
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
	                       JACKDBUS_SERVICE,
	                       JACKDBUS_OBJECT,
	                       JACKDBUS_IFACE_PATCHBAY,
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
lashd_jackdbus_on_client_appeared(
	const char * client_name,
	dbus_uint64_t client_id)
{
	dbus_int64_t pid;
	client_t * client_ptr;
	jack_mgr_client_t * jack_client_ptr;

	pid = lashd_jackdbus_get_client_pid(client_id);

	lash_debug("New JACK client '%s' with id %llu, pid %lld",
	           client_name, (unsigned long long)client_id, (long long)pid);

	jack_client_ptr = jack_mgr_client_new();
	jack_client_ptr->name = lash_strdup(client_name);
	jack_client_ptr->jackdbus_id = client_id;
	jack_client_ptr->pid = (pid_t) pid;

	if (!(client_ptr = server_find_client_by_pid(pid))
	    && !(client_ptr = server_find_lost_client_by_pid(pid))) {
		/* None of the known LASH clients have the same PID */
		lash_debug("Storing unknown JACK client '%s'", client_name);
		list_add_tail(&jack_client_ptr->siblings, &g_jack_mgr_ptr->unknown_clients);
		/* TODO: we need to create liblash-less client object here */
		return;
	}

	/* The new JACK client's PID matches a known LASH client, associate them */

	INIT_LIST_HEAD(&jack_client_ptr->siblings);
	lashd_jackdbus_mgr_bind_client(jack_client_ptr, client_ptr);

	lash_debug("Client added");
}

/** Remove the unknown client whose id parameter matches \a client_id.
 * @param client_id The disappeared client's JACK D-Bus ID.
 */
static void
lashd_jackdbus_on_client_disappeared(dbus_uint64_t client_id)
{
	jack_mgr_client_t *client;

	if (!(client = jack_mgr_client_find_by_jackdbus_id(&g_jack_mgr_ptr->unknown_clients, client_id)))
		return;

	lash_debug("Removing unknown JACK client '%s'", client->name);
	list_del(&client->siblings);
	jack_mgr_client_destroy(client);
}

static void
lashd_jackdbus_mgr_new_foreign_port(const char *client_name,
                                    const char *port_name)
{
	lash_debug("new foreign port '%s:%s'", client_name, port_name);

	struct list_head *node, *node2;
	jack_mgr_client_t *client;
	jack_patch_t *patch;
	const char *foreign_client, *foreign_port;
	bool port_is_input;
	const char *client1_name, *port1_name, *client2_name, *port2_name;

	list_for_each (node, &g_jack_mgr_ptr->clients) {
		client = list_entry(node, jack_mgr_client_t, siblings);

		list_for_each (node2, &client->old_patches) {
			patch = list_entry(node2, jack_patch_t, siblings);

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
	lash_debug("New client port '%s:%s'", client_name, port_name);

	jack_mgr_client_t *other_client;
	struct list_head *node;
	jack_patch_t *patch;
	bool port_is_input;
	uuid_t *other_id;
	const char *client1_name, *port1_name, *client2_name, *port2_name;

	/* Iterate the client's old patches and try to connect those which
	   contain the new port */
	list_for_each (node, &client->old_patches) {
		patch = list_entry(node, jack_patch_t, siblings);

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
			other_client = jack_mgr_client_find_by_id(&g_jack_mgr_ptr->clients,
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
lashd_jackdbus_mgr_del_old_patch(jack_mgr_client_t *client,
                                 dbus_uint64_t      src_id,
                                 const char        *src_name,
                                 const char        *src_port,
                                 dbus_uint64_t      dest_id,
                                 const char        *dest_name,
                                 const char        *dest_port);

static
void
lashd_jackdbus_mgr_check_connection(
	jack_mgr_client_t * jack_client_ptr,
	dbus_uint64_t client1_id,
	dbus_uint64_t client2_id)
{
	client_t * client_ptr;

	if (client1_id == jack_client_ptr->jackdbus_id)
	{
		if (client2_id == jack_client_ptr->jackdbus_id)
		{
			/* Patch is an internal connection */
			return;
		}

		/* An output port patch */
	}
	else if (client2_id == jack_client_ptr->jackdbus_id)
	{
		/* An input port patch */
	}
	else
	{
		/* Patch does not involve the client */
		return;
	}

	client_ptr = server_find_client_by_id(jack_client_ptr->id);
	if (client_ptr != NULL && client_ptr->project != NULL)
	{
		project_set_modified_status(client_ptr->project, true);
	}
}

static void
lashd_jackdbus_mgr_ports_connected(dbus_uint64_t  client1_id,
                                   const char    *client1_name,
                                   const char    *port1_name,
                                   dbus_uint64_t  client2_id,
                                   const char    *client2_name,
                                   const char    *port2_name)
{
	jack_mgr_client_t *client;

	client = jack_mgr_client_find_by_jackdbus_id(&g_jack_mgr_ptr->clients,
	                                             client1_id);
	if (client)
	{
		lashd_jackdbus_mgr_del_old_patch(client, client1_id,
		                                 client1_name, port1_name,
		                                 client2_id, client2_name,
		                                 port2_name);

		lashd_jackdbus_mgr_check_connection(client, client1_id, client2_id);
	}

	client = jack_mgr_client_find_by_jackdbus_id(&g_jack_mgr_ptr->clients,
	                                             client2_id);
	if (client)
	{
		lashd_jackdbus_mgr_del_old_patch(client, client1_id,
		                                 client1_name, port1_name,
		                                 client2_id, client2_name,
		                                 port2_name);

		lashd_jackdbus_mgr_check_connection(client, client1_id, client2_id);
	}
}

static void
lashd_jackdbus_mgr_ports_disconnected(
	dbus_uint64_t client1_id,
	const char * client1_name,
	const char * port1_name,
	dbus_uint64_t client2_id,
	const char * client2_name,
	const char * port2_name)
{
	jack_mgr_client_t *client;

	client = jack_mgr_client_find_by_jackdbus_id(&g_jack_mgr_ptr->clients, client1_id);
	if (client)
	{
		lashd_jackdbus_mgr_check_connection(client, client1_id, client2_id);
	}

	client = jack_mgr_client_find_by_jackdbus_id(&g_jack_mgr_ptr->clients, client2_id);
	if (client)
	{
		lashd_jackdbus_mgr_check_connection(client, client1_id, client2_id);
	}
}

static
void
lashd_jackdbus_handle_patchbay_signal(
	DBusMessage * message_ptr)
{
	const char * signal_name;
	DBusError err;
	const char *client1_name, *port1_name;
	dbus_uint64_t dummy, client1_id;
	jack_mgr_client_t *client;
	const char *client2_name, *port2_name;
	dbus_uint64_t client2_id;

	signal_name = dbus_message_get_member(message_ptr);
	if (signal_name == NULL)
	{
		lash_error("Received JACK signal with NULL member");
		return;
	}

	dbus_error_init(&err);

	if (strcmp(signal_name, "ClientAppeared") == 0)
	{
		lash_debug("Received ClientAppeared signal");

		if (!dbus_message_get_args(
			    message_ptr,
			    &err,
			    DBUS_TYPE_UINT64, &dummy,
			    DBUS_TYPE_UINT64, &client1_id,
			    DBUS_TYPE_STRING, &client1_name,
			    DBUS_TYPE_INVALID))
		{
			goto fail;
		}

		lashd_jackdbus_on_client_appeared(client1_name, client1_id);
		return;
	}

	if (strcmp(signal_name, "ClientDisappeared") == 0) {
		lash_debug("Received ClientDisappeared signal");

		if (!dbus_message_get_args(message_ptr, &err,
		                           DBUS_TYPE_UINT64, &dummy,
		                           DBUS_TYPE_UINT64, &client1_id,
		                           DBUS_TYPE_STRING, &client1_name,
		                           DBUS_TYPE_INVALID))
			goto fail;

		lashd_jackdbus_on_client_disappeared(client1_id);
		return;
	}

	if (strcmp(signal_name, "PortAppeared") == 0)
	{
		lash_debug("Received PortAppeared signal");

		if (!dbus_message_get_args(
			    message_ptr,
			    &err,
			    DBUS_TYPE_UINT64, &dummy,
			    DBUS_TYPE_UINT64, &client1_id,
			    DBUS_TYPE_STRING, &client1_name,
			    DBUS_TYPE_UINT64, &dummy,
			    DBUS_TYPE_STRING, &port1_name,
			    DBUS_TYPE_INVALID))
		{
			goto fail;
		}

		/* Check if the new port belongs to a known client */
		client = jack_mgr_client_find_by_jackdbus_id(&g_jack_mgr_ptr->clients, client1_id);
		if (client && !list_empty(&client->old_patches))
		{
			lashd_jackdbus_mgr_new_client_port(client, client1_name, port1_name);
		}
		else
		{
			lashd_jackdbus_mgr_new_foreign_port(client1_name, port1_name);
		}

		return;
	}

	if (strcmp(signal_name, "PortsConnected") == 0)
	{
		lash_debug("Received PortsConnected signal");

		if (!dbus_message_get_args(
			    message_ptr,
			    &err,
			    DBUS_TYPE_UINT64, &dummy,
			    DBUS_TYPE_UINT64, &client1_id,
			    DBUS_TYPE_STRING, &client1_name,
			    DBUS_TYPE_UINT64, &dummy,
			    DBUS_TYPE_STRING, &port1_name,
			    DBUS_TYPE_UINT64, &client2_id,
			    DBUS_TYPE_STRING, &client2_name,
			    DBUS_TYPE_UINT64, &dummy,
			    DBUS_TYPE_STRING, &port2_name,
			    DBUS_TYPE_INVALID))
		{
			goto fail;
		}

		lashd_jackdbus_mgr_ports_connected(
			client1_id,
			client1_name,
			port1_name,
			client2_id,
			client2_name,
			port2_name);

		return;
	}

	if (strcmp(signal_name, "PortsDisconnected") == 0)
	{
		lash_debug("Received PortsDisconnected signal");

		if (!dbus_message_get_args(
			    message_ptr,
			    &err,
			    DBUS_TYPE_UINT64, &dummy,
			    DBUS_TYPE_UINT64, &client1_id,
			    DBUS_TYPE_STRING, &client1_name,
			    DBUS_TYPE_UINT64, &dummy,
			    DBUS_TYPE_STRING, &port1_name,
			    DBUS_TYPE_UINT64, &client2_id,
			    DBUS_TYPE_STRING, &client2_name,
			    DBUS_TYPE_UINT64, &dummy,
			    DBUS_TYPE_STRING, &port2_name,
			    DBUS_TYPE_INVALID))
		{
			goto fail;
		}

		lashd_jackdbus_mgr_ports_disconnected(
			client1_id,
			client1_name,
			port1_name,
			client2_id,
			client2_name,
			port2_name);
		return;
	}

	return;

fail:
	lash_error("Cannot get message arguments: %s", err.message);
	dbus_error_free(&err);
}

static
void
lashd_jackdbus_handle_control_signal(
	DBusMessage * message_ptr)
{
	const char * signal_name;

	signal_name = dbus_message_get_member(message_ptr);
	if (signal_name == NULL)
	{
		lash_error("Received JACK signal with NULL member");
		return;
	}

	if (strcmp(signal_name, "ServerStarted") == 0)
	{
		lash_info("JACK server start detected.");
		g_jack_mgr_ptr->graph_version = 0;
		return;
	}

	if (strcmp(signal_name, "ServerStopped") == 0)
	{
		lash_info("JACK server stop detected.");
		g_jack_mgr_ptr->graph_version = 0;
		return;
	}
}

static
DBusHandlerResult
lashd_jackdbus_handle_bus_signal(
	DBusMessage * message_ptr)
{
	const char * signal_name;
	DBusError err;
	const char * object_name;
	const char * old_owner;
	const char * new_owner;

	signal_name = dbus_message_get_member(message_ptr);
	if (signal_name == NULL)
	{
		lash_error("Received bus signal with NULL member");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	//lash_info("bus signal '%s' received", signal_name);

	dbus_error_init(&err);

	if (strcmp(signal_name, "NameOwnerChanged") == 0)
	{
		//lash_info("NameOwnerChanged signal received");

		if (!dbus_message_get_args(
			    message_ptr,
			    &err,
			    DBUS_TYPE_STRING, &object_name,
			    DBUS_TYPE_STRING, &old_owner,
			    DBUS_TYPE_STRING, &new_owner,
			    DBUS_TYPE_INVALID)) {
			lash_error("Cannot get message arguments: %s", err.message);
			dbus_error_free(&err);
			return DBUS_HANDLER_RESULT_HANDLED;
		}

		if (strcmp(object_name, JACKDBUS_SERVICE) != 0)
		{
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		}

		if (old_owner[0] == '\0')
		{
			lash_info("JACK serivce appeared");
			g_jack_mgr_ptr->graph_version = 0;
		}
		else if (new_owner[0] == '\0')
		{
			lash_info("JACK serivce disappeared");
			g_jack_mgr_ptr->graph_version = 0;
		}

		return DBUS_HANDLER_RESULT_HANDLED;
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/** This is jackdbus_mgr's D-Bus message handler. It intercepts signals from
 * jackdbus and the session bus and calls the appropriate handlers. Messages
 * not relevant to jackdbus pass through untouched.
 * @param connection The D-Bus connection.
 * @param message The intercepted message.
 * @param data User data.
 * @return Whether the message was handled or not.
 * 
 */
static DBusHandlerResult
lashd_jackdbus_handler(DBusConnection *connection,
                       DBusMessage    *message,
                       void           *data)
{
	const char *path, *interface;

	/* Let non-signal messages and signals with no interface pass through */
	if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL
	    || !(interface = dbus_message_get_interface(message)))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	/* Handle JACK patchbay and control interface signals */
	if ((path = dbus_message_get_path(message))
	    && strcmp(path, JACKDBUS_OBJECT) == 0) {
		if (strcmp(interface, JACKDBUS_IFACE_PATCHBAY) == 0) {
			lashd_jackdbus_handle_patchbay_signal(message);
			return DBUS_HANDLER_RESULT_HANDLED;
		} else if (strcmp(interface, JACKDBUS_IFACE_CONTROL) == 0) {
			lashd_jackdbus_handle_control_signal(message);
			return DBUS_HANDLER_RESULT_HANDLED;
		}
	}

	/* Handle session bus signals to track JACK service alive state */
	if (strcmp(interface, DBUS_INTERFACE_DBUS) == 0)
		return lashd_jackdbus_handle_bus_signal(message);

	/* Let everything else pass through */
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/** Delete the patch described by @a src_id, @a src_name, @a src_port,
 * @a dest_id, @a dest_name, and @a dest_port from @a client 's old_patches
 * list.
 * @param client Pointer to client.
 * @param src_id Patch source client ID.
 * @param src_name Patch source client name.
 * @param src_port Patch source port name.
 * @param dest_id Patch destination client ID.
 * @param dest_name Patch destination client name.
 * @param dest_port Patch destination port name.
 */
static void
lashd_jackdbus_mgr_del_old_patch(jack_mgr_client_t *client,
                                 dbus_uint64_t      src_id,
                                 const char        *src_name,
                                 const char        *src_port,
                                 dbus_uint64_t      dest_id,
                                 const char        *dest_name,
                                 const char        *dest_port)
{
	uuid_t *src_uuid, *dest_uuid;
	jack_mgr_client_t *c;
	jack_patch_t *patch;

	/* Check if the described patch is associated with the client */
	if (src_id != client->jackdbus_id && dest_id != client->jackdbus_id)
		return;

	/* Grab pointer to UUID of source client if it is known */
	src_uuid = ((src_id == client->jackdbus_id)
	            ? &client->id
	            : ((c = jack_mgr_client_find_by_jackdbus_id(&g_jack_mgr_ptr->clients,
	                                                        src_id))
	               ? &c->id
	               : NULL));

	/* Grab pointer to UUID of destination client if it is known */
	dest_uuid = ((dest_id == client->jackdbus_id)
	             ? &client->id
	             : ((c = jack_mgr_client_find_by_jackdbus_id(&g_jack_mgr_ptr->clients,
	                                                         dest_id))
	                ? &c->id
	                : NULL));

	if (!(patch = jack_patch_find_by_description(&client->old_patches,
	                                             src_uuid, src_name, src_port,
	                                             dest_uuid, dest_name, dest_port)))
		return;

	/* Patch was found and can be deleted */

	lash_debug("Connected patch '%s:%s' -> '%s:%s' is an old patch "
	           "of client '%s'; removing from list", src_name,
	           src_port, dest_name, dest_port, client->name);

	list_del(&patch->siblings);
	jack_patch_destroy(patch);

#ifdef LASH_DEBUG
	if (list_empty(&client->old_patches))
		lash_debug("All old patches of client '%s' have "
		           "now been connected", client->name);
#endif
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

	if (list_empty(&client->old_patches)) {
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

		lashd_jackdbus_mgr_new_client_port(client, client1_name, port1_name);

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

		lashd_jackdbus_mgr_del_old_patch(client, client1_id,
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

bool
lashd_jackdbus_mgr_remove_client(lashd_jackdbus_mgr_t *mgr,
                                 uuid_t                id,
                                 struct list_head     *backup_patches)
{
	jack_mgr_client_t *client;

	client = jack_mgr_client_find_by_id(&mgr->clients, id);
	if (!client) {
		lash_error("Unknown client");
		return false;
	}

	lash_debug("Removing client '%s'", client->name);

	list_del(&client->siblings);

	if (backup_patches)
		list_splice_init(&client->backup_patches, backup_patches);

	jack_mgr_client_destroy(client);

	return true;
}

bool
lashd_jackdbus_mgr_get_client_patches(lashd_jackdbus_mgr_t *mgr,
                                      uuid_t                id,
                                      struct list_head     *dest)
{
	if (!mgr) {
		lash_error("JACK manager pointer is NULL");
		return false;
	}

	jack_mgr_client_t *client, *other_client;
	jack_patch_t *patch;
	DBusMessageIter iter, array_iter, struct_iter;
	dbus_uint64_t client1_id, client2_id, other_id;
	const char *client1_name, *port1_name, *client2_name, *port2_name, *other_name;
	bool port_is_input;
	uuid_t *client1_uuid, *client2_uuid, *other_uuid;

	if (!mgr->graph) {
		lash_error("Cannot find graph");
		return false;
	}

	client = jack_mgr_client_find_by_id(&mgr->clients, id);
	if (!client) {
		char id_str[37];
		uuid_unparse(id, id_str);
		lash_error("Cannot find client %s", id_str);
		return false;
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
		  jack_mgr_client_find_by_jackdbus_id(&mgr->clients, other_id);

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

		list_add_tail(&patch->siblings, dest);

		dbus_message_iter_next(&array_iter);
	}

	/* Make a fresh backup of the newly acquired patch list */
	jack_mgr_client_free_patch_list(&client->backup_patches);
	INIT_LIST_HEAD(&client->backup_patches);
	if (!list_empty(dest))
		jack_mgr_client_dup_patch_list(dest, &client->backup_patches);

	return true;

fail:
	/* If we're here there was something rotten in the graph message,
	   we should remove it */
	lashd_jackdbus_mgr_graph_free();
	return false;

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
	                       JACKDBUS_SERVICE,
	                       JACKDBUS_OBJECT,
	                       JACKDBUS_IFACE_PATCHBAY,
	                       "GetGraph",
	                       DBUS_TYPE_UINT64,
	                       &mgr->graph_version);
}

/* EOF */
