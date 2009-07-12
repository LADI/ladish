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

#include <stdlib.h>
#include <stdint.h>
#include <dbus/dbus.h>

#include "../../common/safety.h"
#include "../../common/debug.h"

#include "../../dbus/method.h"

#include "lash/event.h"

#include "event.h"
#include "client.h"

#include "lash/client_interface.h"

#define set_string_property(property, value) \
  do {                               \
    if (property)                    \
      free(property);                \
    if (value)                       \
      property = lash_strdup(value); \
    else                             \
      property = NULL;               \
  } while (0)

lash_event_t *
lash_event_new(void)
{
	return lash_calloc(1, sizeof(lash_event_t));
}

//   LASH_Client_Name = 1,  /* set the client's user-visible name */
//   LASH_Jack_Client_Name, /* tell the server what name the client is connected to jack with */
//   LASH_Save_File,        /* tell clients to save to files */
//   LASH_Restore_File,     /* tell clients to restore from files */
//   LASH_Save_Data_Set,    /* tell clients to send the server a data set */
//   LASH_Restore_Data_Set, /* tell clients a data set will be arriving */
//   LASH_Save,             /* save the project */
//   LASH_Quit,             /* tell the server to close the connection */

//   LASH_Server_Lost,      /* the server disconnected */
//   LASH_Project_Add,             /* new project has been created */
//   LASH_Project_Remove,          /* existing project has been lost */
//   LASH_Project_Dir,             /* change project dir */
//   LASH_Project_Name,            /* change project name */
//   LASH_Client_Add,              /* a new client has been added to a project */
//   LASH_Client_Remove,           /* a client has been lost from a project */
//   LASH_Percentage               /* display a percentage of an action to the user */

struct _handler_ctx
{
	lash_client_t *client;
	int            ev_type;
};

static void
_lash_id_query_handler(DBusPendingCall *pending,
                       void            *data)
{
	DBusMessage *msg = dbus_pending_call_steal_reply(pending);
	DBusError err;
	const char *name;
	dbus_bool_t retval;
	lash_event_t *event;
	lash_client_t *client;
	struct _handler_ctx *ctx;
	const char *err_str;

	ctx = data;
	client = ctx->client;

	if (!msg) {
		lash_error("Cannot get method return from pending call");
		goto end;
	}

	if (!method_return_verify(msg, &err_str)) {
		lash_error("Server failed to return client name: %s", err_str);
		goto end_unref_msg;
	}

	dbus_error_init(&err);

  retval = dbus_message_get_args(msg, &err,
                                 DBUS_TYPE_STRING, &name,
                                 DBUS_TYPE_INVALID);

	if (!retval) {
		lash_error("Cannot get message argument: %s", err.message);
		dbus_error_free(&err);
		goto end_unref_msg;
	}

	if (name && !name[0])
		name = NULL;

	/* Create an event and add it to the incoming queue */

	if (!(event = lash_event_new_with_all(ctx->ev_type,
	                                      name))) {
		lash_error("Failed to allocate event");
		goto end_unref_msg;
	}

	lash_client_add_event(client, event);

end_unref_msg:
	dbus_message_unref(msg);

end:
	dbus_pending_call_unref(pending);
}

static void
_lash_client_name(lash_client_t *client,
                  lash_event_t  *event)
{
	if (event->string) {
		lash_info("Not sending deprecated LASH_Client_Name event");
	} else {
		struct _handler_ctx ctx = { client, LASH_Client_Name };

		method_call_new_void(client->dbus_service, &ctx,
		                     _lash_id_query_handler, false,
		                     "org.nongnu.LASH",
		                     "/",
		                     "org.nongnu.LASH.Server",
		                     "GetName");
	}
}

static void
_lash_jack_client_name(lash_client_t *client,
                       lash_event_t  *event)
{
	if (event->string) {
		lash_jack_client_name(client, (const char *) event->string);
	} else {
		struct _handler_ctx ctx = { client, LASH_Jack_Client_Name };

		method_call_new_void(client->dbus_service, &ctx,
		                     _lash_id_query_handler, false,
		                     "org.nongnu.LASH",
		                     "/",
		                     "org.nongnu.LASH.Server",
		                     "GetJackName");
	}
}

static void
_lash_task_done(lash_client_t *client,
                lash_event_t  *event)
{
	if (client->pending_task) {
		const uint8_t x = 255;
		method_call_new_valist(client->dbus_service, NULL,
		                       method_default_handler, false,
		                       "org.nongnu.LASH",
		                       "/",
		                       "org.nongnu.LASH.Server",
		                       "Progress",
		                       DBUS_TYPE_UINT64, &client->pending_task,
		                       DBUS_TYPE_BYTE, &x,
		                       DBUS_TYPE_INVALID);
		client->pending_task = 0;
	} else {
		lash_error("No pending task to send notification about");
	}
}

static void
_lash_save_data_set(lash_client_t *client,
                    lash_event_t  *event)
{
	if (!client->pending_task) {
		lash_error("Server has not requested a save, not sending configs");
		return;
	}

	/* This is the same code as in lash_new_save_data_set_task(), but
	   I'm too lazy to make it into a function because it's for the
	   compat API. */

	if (!dbus_message_iter_close_container(&client->iter, &client->array_iter)) {
		lash_error("Failed to close array container");
		dbus_message_unref(client->unsent_configs.message);
		goto end;
	}

	if (!method_send(&client->unsent_configs, false)) {
		lash_error("Failed to send CommitDataSet method call");
		goto end;
	}

	lash_debug("Sent data set message");

end:
	client->pending_task = 0;
}

/*
 * Tell the server to save the project the client is attached to
 */
static void
_lash_save(lash_client_t *client,
           lash_event_t  *event)
{
	method_call_new_void(client->dbus_service, NULL,
	                     method_default_handler, false,
	                     "org.nongnu.LASH",
	                     "/",
	                     "org.nongnu.LASH.Server",
	                     "SaveProject");
}

/*
 * Tell the server to close the project the client is attached to
 */
static void
_lash_quit(lash_client_t *client,
           lash_event_t  *event)
{
	method_call_new_void(client->dbus_service, NULL,
	                     method_default_handler, false,
	                     "org.nongnu.LASH",
	                     "/",
	                     "org.nongnu.LASH.Server",
	                     "CloseProject");
}

static const LASHEventConstructor g_lash_event_ctors[] = {
	NULL,
	_lash_client_name,
	_lash_jack_client_name,
	NULL,
	_lash_task_done,
	_lash_task_done,
	_lash_save_data_set,
	_lash_task_done,
	_lash_save,
	_lash_quit,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

lash_event_t *
lash_event_new_with_type(enum LASH_Event_Type type)
{
	if (type < 1 || type > 17) {
		lash_error("Invalid type");
		return NULL;
	}

	lash_event_t *event;

	event = lash_calloc(1, sizeof(lash_event_t));
	event->type = type;
	event->ctor = g_lash_event_ctors[type];

	return event;
}

lash_event_t *
lash_event_new_with_all(enum LASH_Event_Type  type,
                        const char           *string)
{
	if (type < 1 || type > 17) {
		lash_error("Invalid type");
		return NULL;
	}

	lash_event_t *event;

	event = lash_calloc(1, sizeof(lash_event_t));
	event->type = type;
	event->ctor = g_lash_event_ctors[type];
	lash_event_set_string(event, string);

	return event;
}

void
lash_event_destroy(lash_event_t *event)
{
	if (event) {
		lash_free(&event->string);
		lash_free(&event->project);
		free(event);
	}
}

enum LASH_Event_Type
lash_event_get_type(const lash_event_t *event)
{
	if (event)
		return event->type;

	return LASH_Event_Unknown;
}

void
lash_event_set_type(lash_event_t         *event,
                    enum LASH_Event_Type  type)
{
	if (type < 1 || type > 17) {
		lash_error("Invalid type");
		return;
	}

	event->type = type;
	event->ctor = g_lash_event_ctors[type];
}

const char *
lash_event_get_string(const lash_event_t *event)
{
	if (event)
		return event->string;

	return NULL;
}

void
lash_event_set_string(lash_event_t *event,
                       const char  *string)
{
	if (event)
		set_string_property(event->string, string);
}

const char *
lash_event_get_project(const lash_event_t *event)
{
	if (event)
		return event->project;

	return NULL;
}

void
lash_event_set_project(lash_event_t *event,
                       const char   *project)
{
	if (event)
		set_string_property(event->project, project);
}

void
lash_event_get_client_id(const lash_event_t *event,
                         uuid_t              id)
{
	if (event)
		uuid_copy(id, event->client_id);
}

void
lash_event_set_client_id(lash_event_t *event,
                         uuid_t        id)
{
	if (event)
		uuid_copy(event->client_id, id);
}

void
lash_event_set_alsa_client_id(lash_event_t  *event,
                              unsigned char  alsa_id)
{
  return;
}

unsigned char
lash_event_get_alsa_client_id(const lash_event_t *event)
{
	return 0;
}

void
lash_str_set_alsa_client_id(char          *str,
                            unsigned char  alsa_id)
{
  return;
}

unsigned char
lash_str_get_alsa_client_id(const char *str)
{
  return 0;
}

/* EOF */
