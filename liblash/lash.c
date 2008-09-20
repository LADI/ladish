/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *   Copyright (C) 2002, 2003 Robert Ham <rah@bash.sh>
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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>
#include <dbus/dbus.h>

#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/param.h>
#include <rpc/xdr.h>

#include "common/safety.h"
#include "common/debug.h"

#include "lash/lash.h"

#include "dbus/service.h"

#include "dbus_service.h"
#include "client.h"
#include "lash_config.h"

static void
ping_handler(DBusPendingCall *pending,
             void            *data)
{
	DBusMessage *msg = dbus_pending_call_steal_reply(pending);
	if (msg)
		dbus_message_unref(msg);
	dbus_pending_call_unref(pending);
	fprintf(stderr, "Server replied: Pong!\n");
}

static lash_client_t *
lash_client_new_with_service(void)
{
	lash_client_t *client = lash_client_new();
	if (!client) {
		lash_error("Failed to allocate memory for client");
		return NULL;
	}

	/* Connect to the D-Bus daemon */
	client->dbus_service = lash_dbus_service_new(client);
	if (!client->dbus_service) {
		lash_error("Failed to start client D-Bus service");
		lash_client_destroy(client);
		client = NULL;
	}

	return client;
}

/* The client's signal handler function for signals
   originating from org.nongnu.LASH.Server */
static void
lash_server_signal_handler(lash_client_t *client,
                           const char    *member,
                           DBusMessage   *message)
{
	const char *project_name;
	dbus_uint64_t task_id;
	DBusError err;

	dbus_error_init(&err);

	if (strcmp(member, "Save") == 0) {
		if (!dbus_message_get_args(message, &err,
		                           DBUS_TYPE_STRING, &project_name,
		                           DBUS_TYPE_UINT64, &task_id,
		                           DBUS_TYPE_INVALID)) {
			lash_error("Cannot get signal arguments: %s", err.message);
			dbus_error_free(&err);
			return;
		}

		//lash_info("Save signal for project '%s' received.", project_name);

		/* Silently return if this signal doesn't concern our project */
		if (!client->project_name
		    || strcmp(client->project_name, project_name) != 0)
			return;

		if (!client->pending_task) {
			if ((client->flags & LASH_Config_Data_Set))
				lash_new_save_data_set_task(client, task_id);
			else if ((client->flags & LASH_Config_File))
				lash_new_save_task(client, task_id);
		} else {
			lash_dbus_error("Task %llu is unfinished",
			                client->pending_task);
		}

		//lash_info("Save signal for project '%s' processed.", project_name); fflush(stdout);

	} else if (strcmp(member, "Quit") == 0) {
		if (!dbus_message_get_args(message, &err,
		                           DBUS_TYPE_STRING, &project_name,
		                           DBUS_TYPE_INVALID)) {
			lash_error("Cannot get signal arguments: %s", err.message);
			dbus_error_free(&err);
			return;
		}

		/* Silently return if this signal doesn't concern our project */
		if (client->project_name
		    && strcmp(client->project_name, project_name) != 0)
			return;

		lash_new_quit_task(client);

	} else {
		lash_error("Received unknown signal '%s'", member);
	}
}

static void
lash_project_name_changed_handler(lash_client_t *client,
                                  DBusMessage   *message)
{
	const char *old_name, *new_name;
	DBusError err;

	dbus_error_init(&err);

	if (!dbus_message_get_args(message, &err,
	                           DBUS_TYPE_STRING, &old_name,
	                           DBUS_TYPE_STRING, &new_name,
	                           DBUS_TYPE_INVALID)) {
		lash_error("Cannot get signal arguments: %s", err.message);
		dbus_error_free(&err);
		return;
	}

	if (client->project_name) {
		if (strcmp(client->project_name, old_name) != 0)
			return;
	} else if (old_name[0])
		return;

	if (!new_name[0])
		new_name = NULL;

	lash_strset(&client->project_name, new_name);

	/* Call client's ProjectChanged callback */
	if (client->cb.proj)
		client->cb.proj(client->ctx.proj);
}

static void
lash_control_signal_handler(lash_client_t *client,
                            const char    *member,
                            DBusMessage   *message)
{
	if (!client->cb.control) {
		lash_error("Controller callback function is not set");
		return;
	}

	uint8_t sig_id;
	enum LASH_Event_Type type;
	dbus_bool_t ret;
	DBusError err;
	const char *str1, *str2;
	unsigned char byte_var[2];
	uuid_t id;

	if (strcmp(member, "ProjectDisappeared") == 0) {
		sig_id = 1;
		type = LASH_Project_Remove;
	} else if (strcmp(member, "ProjectAppeared") == 0) {
		sig_id = 2;
		type = LASH_Project_Add;
	} else if (strcmp(member, "ProjectNameChanged") == 0) {
		sig_id = 3;
		type = LASH_Project_Name;
	} else if (strcmp(member, "ProjectPathChanged") == 0) {
		sig_id = 4;
		type = LASH_Project_Dir;
	} else if (strcmp(member, "ClientAppeared") == 0) {
		sig_id = 5;
		type = LASH_Client_Add;
	} else if (strcmp(member, "ClientDisappeared") == 0) {
		sig_id = 6;
		type = LASH_Client_Remove;
	} else if (strcmp(member, "ClientNameChanged") == 0) {
		sig_id = 7;
		type = LASH_Client_Name;
	} else if (strcmp(member, "ClientJackNameChanged") == 0) {
		sig_id = 8;
		type = LASH_Jack_Client_Name;
	} else if (strcmp(member, "ClientAlsaIdChanged") == 0) {
		sig_id = 9;
		type = LASH_Alsa_Client_ID;
	} else if (strcmp(member, "Progress") == 0) {
		sig_id = 10;
		type = LASH_Percentage;
	} else {
		lash_error("Received unknown signal '%s'", member);
		return;
	}

	dbus_error_init(&err);

	if (sig_id == 1) {
		ret = dbus_message_get_args(message, &err,
		                            DBUS_TYPE_STRING, &str1,
		                            DBUS_TYPE_INVALID);
		str2 = "";
	} else if (sig_id < 9) {
		ret = dbus_message_get_args(message, &err,
		                            DBUS_TYPE_STRING, &str1,
		                            DBUS_TYPE_STRING, &str2,
		                            DBUS_TYPE_INVALID);
	} else if (sig_id == 9) {
		ret = dbus_message_get_args(message, &err,
		                            DBUS_TYPE_STRING, &str1,
		                            DBUS_TYPE_BYTE, &byte_var[0],
		                            DBUS_TYPE_INVALID);
		byte_var[1] = '\0';
		str2 = (const char *) byte_var;
	} else {
		ret = dbus_message_get_args(message, &err,
		                            DBUS_TYPE_BYTE, &byte_var[0],
		                            DBUS_TYPE_INVALID);
		byte_var[1] = '\0';
		str1 = (const char *) byte_var;
		str2 = "";
	}

	if (!ret) {
		lash_error("Cannot get signal arguments: %s", err.message);
		dbus_error_free(&err);
		return;
	}

	if (sig_id < 5 || sig_id > 9)
		uuid_clear(id);
	else if (uuid_parse(str1, id) == 0) {
		str1 = str2;
	} else {
		lash_error("Cannot parse client id");
		return;
	}

	/* Call the control callback */
	client->cb.control(type, str1, str2, id, client->ctx.control);
}

static DBusHandlerResult
lash_dbus_signal_handler(DBusConnection *connection,
                         DBusMessage    *message,
                         void           *data)
{
	/* Let non-signal messages fall through */
	if (dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL)
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	lash_client_t *client = data;
	const char *member, *interface;

	member = dbus_message_get_member(message);

	if (!member) {
		lash_error("Received signal with NULL member");
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	interface = dbus_message_get_interface(message);

	if (!interface) {
		lash_error("Received signal with NULL interface");
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	if (strcmp(interface, "org.nongnu.LASH.Server") == 0) {
		lash_debug("Received Server signal '%s'", member);
		lash_server_signal_handler(client, member, message);

	} else if (strcmp(interface, "org.nongnu.LASH.Control") == 0) {
		lash_debug("Received Control signal '%s'", member);

		/* This arrangement is OK only so far as a normal client
		   doesn't intercept more than one signal, ProjectNameChanged */
		if (client->is_controller != 1) {
			lash_project_name_changed_handler(client, message);
			if (client->is_controller) {
				lash_control_signal_handler(client, member, message);
			}
		} else {
			lash_control_signal_handler(client, member, message);
		}

	} else if (strcmp(interface, "org.freedesktop.DBus") != 0) {
		lash_error("Received signal from unknown interface '%s'",
		           interface);
	}

	return DBUS_HANDLER_RESULT_HANDLED;
}

static bool
lash_client_register_as_controller(lash_client_t *client)
{
	DBusError err;

	dbus_error_init(&err);

	lash_debug("Registering client as control application");

	/* Listen to the LASH server's frontend-facing beacon */
	dbus_bus_add_match(client->dbus_service->connection,
	                   "type='signal'"
	                   ",sender='org.nongnu.LASH'"
	                   ",path='/'"
	                   ",interface='org.nongnu.LASH.Control'",
	                   &err);

	if (dbus_error_is_set(&err)) {
		lash_error("Failed to add D-Bus match rule: "
		           "%s", err.message);
		dbus_error_free(&err);
		return false;
	}

	return true;
}

static void
lash_client_add_filter(lash_client_t **client)
{
	if (!dbus_connection_add_filter((*client)->dbus_service->connection,
	                                lash_dbus_signal_handler,
	                                *client, NULL)) {
		lash_error("Failed to add D-Bus filter");
		lash_client_destroy(*client);
		*client = NULL;
	}
}

lash_client_t *
lash_client_open(const char  *class,
                 int          flags,
                 int          argc,
                 char       **argv)
{
	lash_client_t *client = NULL;

	if (!class || !class[0] || !argc || !argv || !argv[0] || !argv[0][0]) {
		lash_error("Invalid arguments");
		goto end;
	}

	char *str, wd[MAXPATHLEN];
	DBusError err;

	client = lash_client_new_with_service();
	if (!client) {
		lash_error("Failed to create new client");
		goto end;
	}

	/* Set the client parameters */
	if (!(str = getcwd(wd, MAXPATHLEN))) {
		lash_error("Cannot get working directory: %s",
		           strerror(errno));
		wd[0] = '\0';
		if ((str = getenv("PWD")) || (str = getenv("HOME"))) {
			if (strlen(str) < MAXPATHLEN)
				strcpy(wd, str);
		}
	}
	client->argc = argc;
	client->argv = argv;
	client->working_dir = lash_strdup(wd);
	client->flags = flags;
	lash_strset(&client->class, class);

	dbus_error_init(&err);

	/* Check whether the server is active */
	if (!dbus_bus_name_has_owner(client->dbus_service->connection,
	                             // TODO: Move service name into public header
	                             "org.nongnu.LASH", &err)) {
		if (dbus_error_is_set(&err)) {
			lash_error("Failed to query LASH service "
			           "availability: %s", err.message);
			dbus_error_free(&err);
			dbus_error_init(&err);
		}

		if (!getenv("LASH_NO_START_SERVER"))
			lash_info("Attempting to auto-start LASH server");
		else {
			lash_info("Not attempting to auto-start LASH server");
			goto end;
		}
	}

	lash_dbus_service_connect(client);

	if (client->flags & LASH_Server_Interface) {
		lash_client_register_as_controller(client);
		client->flags ^= LASH_Server_Interface;
		client->is_controller = 2;
	}

	/* Listen to the LASH server's client-facing beacon */
	dbus_bus_add_match(client->dbus_service->connection,
	                   "type='signal'"
	                   ",sender='org.nongnu.LASH'"
	                   ",path='/'"
	                   ",interface='org.nongnu.LASH.Server'",
	                   &err);

	if (!dbus_error_is_set(&err)) {
		dbus_bus_add_match(client->dbus_service->connection,
		                   "type='signal'"
		                   ",sender='org.nongnu.LASH'"
		                   ",path='/'"
		                   ",interface='org.nongnu.LASH.Control'"
		                   ",member='ProjectNameChanged'",
		                   &err);
	}

	if (dbus_error_is_set(&err)) {
		lash_error("Failed to add D-Bus match rule: %s"
		           "%s", err.message);
		dbus_error_free(&err);
		lash_client_destroy(client);
		client = NULL;
		goto end;
	}

	lash_client_add_filter(&client);

end:
	/* Make sure that no forked client process inherits the id */
	unsetenv("LASH_CLIENT_AUTOLAUNCH_ID");

	return client;
}

lash_client_t *
lash_client_open_controller(void)
{
	lash_client_t *client;

	client = lash_client_new_with_service();
	if (!client) {
		lash_error("Failed to create new client");
		return NULL;
	}

	if (!lash_client_register_as_controller(client)) {
		lash_client_destroy(client);
		return NULL;
	}

	client->is_controller = 1;
	lash_client_add_filter(&client);

	return client;
}

// TODO: Move lash_set_*_callback() args checking to a common function

bool
lash_set_control_callback(lash_client_t       *client,
                          LashControlCallback  callback,
                          void                *user_data)
{
	if (!client) {
		lash_error("Client pointer is NULL");
		return false;
	} else if (!callback) {
		lash_error("Callback function is NULL");
		return false;
	}

	client->cb.control = callback;
	client->ctx.control = user_data;

	return true;
}

bool
lash_set_save_callback(lash_client_t     *client,
                       LashEventCallback  callback,
                       void              *user_data)
{
	if (!client || !callback) {
		lash_error("Invalid arguments");
		return false;
	} else if (!client->server_connected) {
		lash_error("Not connected to server");
		return false;
	}

	client->cb.save = callback;
	client->ctx.save = user_data;

	return true;
}

bool
lash_set_load_callback(lash_client_t     *client,
                       LashEventCallback  callback,
                       void              *user_data)
{
	if (!client || !callback) {
		lash_error("Invalid arguments");
		return false;
	} else if (!client->server_connected) {
		lash_error("Not connected to server");
		return false;
	}

	client->cb.load = callback;
	client->ctx.load = user_data;

	return true;
}

bool
lash_set_save_data_set_callback(lash_client_t      *client,
                                LashConfigCallback  callback,
                                void               *user_data)
{
	if (!client || !callback) {
		lash_error("Invalid arguments");
		return false;
	} else if (!client->server_connected) {
		lash_error("Not connected to server");
		return false;
	}

	client->cb.save_data_set = callback;
	client->ctx.save_data_set = user_data;

	return true;
}

bool
lash_set_load_data_set_callback(lash_client_t      *client,
                                LashConfigCallback  callback,
                                void               *user_data)
{
	if (!client || !callback) {
		lash_error("Invalid arguments");
		return false;
	} else if (!client->server_connected) {
		lash_error("Not connected to server");
		return false;
	}

	client->cb.load_data_set = callback;
	client->ctx.load_data_set = user_data;

	return true;
}

bool
lash_set_quit_callback(lash_client_t     *client,
                       LashEventCallback  callback,
                       void              *user_data)
{
	if (!client || !callback) {
		lash_error("Invalid arguments");
		return false;
	} else if (!client->server_connected) {
		lash_error("Not connected to server");
		return false;
	}

	client->cb.quit = callback;
	client->ctx.quit = user_data;

	return true;
}

bool
lash_set_name_change_callback(lash_client_t     *client,
                              LashEventCallback  callback,
                              void              *user_data)
{
	if (!client || !callback) {
		lash_error("Invalid arguments");
		return false;
	} else if (!client->server_connected) {
		lash_error("Not connected to server");
		return false;
	}

	client->cb.name = callback;
	client->ctx.name = user_data;

	return true;
}

bool
lash_set_project_change_callback(lash_client_t     *client,
                                 LashEventCallback  callback,
                                 void              *user_data)
{
	if (!client || !callback) {
		lash_error("Invalid arguments");
		return false;
	} else if (!client->server_connected) {
		lash_error("Not connected to server");
		return false;
	}

	client->cb.proj = callback;
	client->ctx.proj = user_data;

	return true;
}

bool
lash_set_path_change_callback(lash_client_t     *client,
                              LashEventCallback  callback,
                              void              *user_data)
{
	if (!client || !callback) {
		lash_error("Invalid arguments");
		return false;
	} else if (!client->server_connected) {
		lash_error("Not connected to server");
		return false;
	}

	client->cb.path = callback;
	client->ctx.path = user_data;

	return true;
}

void
lash_wait(lash_client_t *client)
{
	if (client && client->dbus_service)
		(void) dbus_connection_read_write(client->dbus_service->connection, -1);
}

void
lash_dispatch(lash_client_t *client)
{
	if (!client || !client->dbus_service)
		return;

	while (dbus_connection_get_dispatch_status(client->dbus_service->connection)
	       == DBUS_DISPATCH_DATA_REMAINS) {
		dbus_connection_read_write_dispatch(client->dbus_service->connection, 0);
	}
}

bool
lash_dispatch_once(lash_client_t *client)
{
	return
	  (client && client->dbus_service
	   && dbus_connection_read_write_dispatch(client->dbus_service->connection, 0)
	   && dbus_connection_get_dispatch_status(client->dbus_service->connection)
	      == DBUS_DISPATCH_DATA_REMAINS)
	  ? true : false;
}

void
lash_notify_progress(lash_client_t *client,
                     uint8_t        percentage)
{
	if (!client || !client->dbus_service
	    || !client->pending_task || !percentage)
		return;

	if (percentage > 99)
		percentage = 99;

	method_call_new_valist(client->dbus_service, NULL,
	                       method_default_handler, false,
	                       "org.nongnu.LASH",
	                       "/",
	                       "org.nongnu.LASH.Server",
	                       "Progress",
	                       DBUS_TYPE_UINT64, &client->pending_task,
	                       DBUS_TYPE_BYTE, &percentage,
	                       DBUS_TYPE_INVALID);
}

const char *
lash_get_client_name(lash_client_t *client)
{
	return (const char *) ((client && client->dbus_service)
	                       ? client->name : NULL);
}

const char *
lash_get_project_name(lash_client_t *client)
{
	return (const char *) ((client && client->dbus_service)
	                       ? client->project_name : NULL);
}

void
lash_jack_client_name(lash_client_t *client,
                      const char    *name)
{
	if (!client || !name || !name[0]) {
		lash_error("Invalid arguments");
		return;
	}

	// TODO: Find some generic place for this
	if (!client->dbus_service) {
		lash_error("D-Bus service not running");
		return;
	}

	method_call_new_single(client->dbus_service, NULL,
	                       method_default_handler, false,
	                       "org.nongnu.LASH",
	                       "/",
	                       "org.nongnu.LASH.Server",
	                       "JackName",
	                       DBUS_TYPE_STRING, &name);

	lash_debug("Sent JACK name");
}

void
lash_alsa_client_id(lash_client_t *client,
                    unsigned char  id)
{
	if (!client) {
		lash_error("NULL client pointer");
		return;
	}

	// TODO: Find some generic place for this
	if (!client->dbus_service) {
		lash_error("D-Bus service not running");
		return;
	}

	method_call_new_single(client->dbus_service, NULL,
	                       method_default_handler, false,
	                       "org.nongnu.LASH",
	                       "/",
	                       "org.nongnu.LASH.Server",
	                       "AlsaId",
	                       DBUS_TYPE_BYTE, &id);

	lash_debug("Sent ALSA id");
}

// TODO: Convert to ProjectOpen
void
lash_control_load_project_path(lash_client_t *client,
                               const char    *project_path)
{
	if (!client || !project_path) {
		lash_error("Invalid arguments");
		return;
	}

	// TODO: Find some generic place for this
	if (!client->dbus_service) {
		lash_error("D-Bus service not running");
		return;
	}

	method_call_new_single(client->dbus_service, NULL,
	                       method_default_handler, false,
	                       "org.nongnu.LASH",
	                       "/",
	                       "org.nongnu.LASH.Control",
	                       "LoadProjectPath",
	                       DBUS_TYPE_STRING, &project_path);

	lash_debug("Sent LoadProjectPath request");
}

void
lash_control_name_project(lash_client_t *client,
                          const char    *project_name,
                          const char    *new_name)
{
	if (!client || !project_name || !new_name) {
		lash_error("Invalid arguments");
		return;
	}

	// TODO: Find some generic place for this
	if (!client->dbus_service) {
		lash_error("D-Bus service not running");
		return;
	}

	method_call_new_valist(client->dbus_service, NULL,
	                       method_default_handler, false,
	                       "org.nongnu.LASH",
	                       "/",
	                       "org.nongnu.LASH.Control",
	                       "ProjectRename",
	                       DBUS_TYPE_STRING, &project_name,
	                       DBUS_TYPE_STRING, &new_name,
	                       DBUS_TYPE_INVALID);

	lash_debug("Sent ProjectRename request");
}

void
lash_control_move_project(lash_client_t *client,
                          const char    *project_name,
                          const char    *new_path)
{
	if (!client || !project_name || !new_path) {
		lash_error("Invalid arguments");
		return;
	}

	// TODO: Find some generic place for this
	if (!client->dbus_service) {
		lash_error("D-Bus service not running");
		return;
	}

	method_call_new_valist(client->dbus_service, NULL,
	                       method_default_handler, false,
	                       "org.nongnu.LASH",
	                       "/",
	                       "org.nongnu.LASH.Control",
	                       "ProjectMove",
	                       DBUS_TYPE_STRING, &project_name,
	                       DBUS_TYPE_STRING, &new_path,
	                       DBUS_TYPE_INVALID);

	lash_debug("Sent ProjectMove request");
}

void
lash_control_save_project(lash_client_t *client,
                          const char    *project_name)
{
	if (!client || !project_name) {
		lash_error("Invalid arguments");
		return;
	}

	// TODO: Find some generic place for this
	if (!client->dbus_service) {
		lash_error("D-Bus service not running");
		return;
	}

	method_call_new_single(client->dbus_service, NULL,
	                       method_default_handler, false,
	                       "org.nongnu.LASH",
	                       "/",
	                       "org.nongnu.LASH.Control",
	                       "ProjectSave",
	                       DBUS_TYPE_STRING, &project_name);

	lash_debug("Sent ProjectSave request");
}

void
lash_control_close_project(lash_client_t *client,
                           const char    *project_name)
{
	if (!client || !project_name) {
		lash_error("Invalid arguments");
		return;
	}

	// TODO: Find some generic place for this
	if (!client->dbus_service) {
		lash_error("D-Bus service not running");
		return;
	}

	method_call_new_single(client->dbus_service, NULL,
	                       method_default_handler, false,
	                       "org.nongnu.LASH",
	                       "/",
	                       "org.nongnu.LASH.Control",
	                       "ProjectClose",
	                       DBUS_TYPE_STRING, &project_name);

	lash_debug("Sent ProjectClose request");
}

#ifdef LASH_OLD_API

# include "args.h"
# include "event.h"

lash_args_t *
lash_extract_args(int *argc, char ***argv)
{
	int i, j, valid_count;
	lash_args_t *args;

	args = lash_args_new();

	for (i = 1; i < *argc; ++i) {
		if (strncasecmp("--lash-server=", (*argv)[i], 14) == 0) {
			lash_error("Dropping deprecated --lash-server flag");
			(*argv)[i] = NULL;
			continue;
		}

		if (strncasecmp("--lash-project=", (*argv)[i], 15) == 0) {
			lash_error("Dropping deprecated --lash-project flag");
			(*argv)[i] = NULL;
			continue;
		}

		if (strncmp("--lash-id=", (*argv)[i], 10) == 0) {
			/* the id is now extracted from an environment var */
			lash_error("Dropping deprecated --lash-id flag");
			(*argv)[i] = NULL;
			continue;
		}

		if (strncmp("--lash-no-autoresume", (*argv)[i], 20) == 0) {
			lash_debug("Setting LASH_No_Autoresume flag from "
			           "command line");

			lash_args_set_flag(args, LASH_No_Autoresume);

			(*argv)[i] = NULL;

			continue;
		}

		if (strncmp("--lash-no-start-server", (*argv)[i], 22) == 0) {
			lash_debug("Setting LASH_No_Start_Server flag from "
			           "command line");

			lash_args_set_flag(args, LASH_No_Start_Server);

			(*argv)[i] = NULL;

			continue;
		}
	}

	lash_debug("Args checked");

	/* sort out the argv pointers */
	valid_count = *argc;
	for (i = 1; i < valid_count; ++i) {
		lash_debug("Checking argv[%d]", i);
		if ((*argv)[i] == NULL) {

			for (j = i; j < *argc - 1; ++j)
				(*argv)[j] = (*argv)[j + 1];

			valid_count--;
			i--;
		}
	}

	lash_debug("Done");

	*argc = valid_count;

	lash_args_set_args(args, *argc, (const char**)*argv);

	return args;
}

lash_client_t *
lash_init(const lash_args_t *args,
          const char        *class,
          int                client_flags,
          lash_protocol_t    protocol)
{
	lash_debug("Initializing LASH client");
	// TODO: Destroy args
	return lash_client_open(class, client_flags, args->argc, args->argv);
}

unsigned int
lash_get_pending_event_count(lash_client_t *client)
{
	if (!lash_enabled(client))
		return 0;

	lash_dispatch(client);

	return (unsigned int) client->num_events_in;
}

unsigned int
lash_get_pending_config_count(lash_client_t *client)
{
	if (!lash_enabled(client))
		return 0;

	lash_dispatch(client);

	return (unsigned int) client->num_configs_in;
}

lash_event_t *
lash_get_event(lash_client_t *client)
{
	if (!lash_enabled(client))
		return NULL;

	lash_event_t *event = NULL;

	lash_dispatch_once(client);

	if (client->events_in) {
		event = (lash_event_t *) client->events_in->data;
		client->events_in = lash_list_remove(client->events_in, event);
		--client->num_events_in;
	}

	return event;
}

lash_config_t *
lash_get_config(lash_client_t *client)
{
	lash_config_t *config = NULL;

	if (!client)
		return NULL;

	lash_dispatch_once(client);

	if (client->configs_in) {
		config = (lash_config_t *) client->configs_in->data;
		client->configs_in = lash_list_remove(client->configs_in, config);
		--client->num_configs_in;
	}

	return config;
}

void
lash_send_event(lash_client_t *client,
                lash_event_t  *event)
{
	if (!client || !event)
		lash_error("Invalid arguments");
	else if (lash_enabled(client) && event->ctor)
		event->ctor(client, event);

	lash_event_destroy(event);
}

void
lash_send_config(lash_client_t *client,
                 lash_config_t *config)
{
	if (!client || !config) {
		lash_error("Invalid arguments");
	} else {
		struct _lash_config_handle cfg;

		cfg.iter = &client->array_iter;
		cfg.is_read = false;

		lash_config_write_raw(&cfg, config->key,
		                      config->value, config->value_size);
	}

end:
	lash_config_destroy(config);
}

const char *
lash_get_server_name(lash_client_t *client)
{
	return (lash_enabled(client)) ? "localhost" : NULL;
}

#endif /* LASH_OLD_API */

int
lash_server_connected(lash_client_t *client)
{
	return (client) ? client->server_connected : 0;
}

/* EOF */
