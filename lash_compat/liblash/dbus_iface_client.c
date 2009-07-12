/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *   Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
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

#include "config.h"

#include "../../common/safety.h"
#include "../../common/debug.h"

#include "../../dbus/error.h"
#include "../../dbus/object_path.h"
#include "../../dbus/interface.h"
#include "../../dbus/method.h"

#include "lash/types.h"

#include "client.h"
#include "lash_config.h"

#ifdef LASH_OLD_API
# include "lash/event.h"
# include "lash/config.h"
#endif

#define client_ptr ((lash_client_t *)(((object_path_t *)call->context)->context))

static void
lash_dbus_try_save_handler(DBusPendingCall *pending,
                           void            *data)
{
	DBusMessage *msg = dbus_pending_call_steal_reply(pending);
	DBusError err;
	dbus_bool_t response;
	const char *err_str;

	if (!msg) {
		lash_error("Cannot get method return from pending call");
		goto end;
	}

	if (!method_return_verify(msg, &err_str)) {
		lash_error("Server failed to : %s", err_str);
		goto end_unref_msg;
	}

	dbus_error_init(&err);

	if (!dbus_message_get_args(msg, &err,
	                           DBUS_TYPE_BOOLEAN,
	                           &response,
	                           DBUS_TYPE_INVALID)) {
		lash_error("Cannot get message argument: %s", err.message);
		dbus_error_free(&err);
		goto end_unref_msg;
	}

	lash_debug("Server says: Saving is %s", response ? "OK" : "not OK");

end_unref_msg:
	dbus_message_unref(msg);

end:
	dbus_pending_call_unref(pending);
}

static void
lash_dbus_try_save(method_call_t *call)
{
	lash_debug("TrySave");

	dbus_bool_t retval;

	if (client_ptr->pending_task) {
		lash_dbus_error(call, LASH_DBUS_ERROR_UNFINISHED_TASK,
		                "Cannot save now; task %llu is unfinished",
		                client_ptr->pending_task);
		return;
	}

	if (!client_ptr->cb.trysave) {
		lash_dbus_error(call, LASH_DBUS_ERROR_GENERIC,
		                "TrySave callback not registered");
		return;
	}

	/* Check whether the client says it's OK to save */
	if (client_ptr->cb.trysave(client_ptr->ctx.trysave)) {
		/* Client says it can save  */
		method_call_new_void(client_ptr->dbus_service,
		                     NULL,
		                     lash_dbus_try_save_handler,
		                     true,
		                     "org.nongnu.LASH",
		                     "/",
		                     "org.nongnu.LASH.Server",
		                     "WaitForSave");
		retval = true;
	} else {
		/* Client says it cannot save, server needs to
		   know it in order to cancel the pending save */
		retval = false;
	}

	method_return_new_single(call, DBUS_TYPE_BOOLEAN, &retval);
}

/* Report task completion or failure to the LASH server */
static void
report_success_or_failure(lash_client_t *client,
                          bool           success)
{
	if (!client->pending_task) {
		lash_error("Client has no pending task");
		return;
	}

	uint8_t x = (uint8_t) (success ? 255 : 0);

	/* Send a success or failure report */
	method_call_new_valist(client->dbus_service, NULL,
	                       method_default_handler, false,
	                       "org.nongnu.LASH",
	                       "/",
	                       "org.nongnu.LASH.Server",
	                       "Progress",
	                       DBUS_TYPE_UINT64, &client->pending_task,
	                       DBUS_TYPE_BYTE, &x,
	                       DBUS_TYPE_INVALID);
}

void
lash_new_save_task(lash_client_t *client,
                   dbus_uint64_t  task_id)
{
	client->pending_task = task_id;
	client->task_progress = 0;

	/* Check if a save callback has been registered */
	if (client->cb.save) {
		/* Call the save callback; its return value dictates whether 
		   to report success or failure back to the server */
		if (client->cb.save(client->ctx.save)) {
			report_success_or_failure(client, true);
		} else {
			lash_error("Client failed to save data");
			report_success_or_failure(client, false);
		}

		client->pending_task = 0;
	} else {
#ifdef LASH_OLD_API
		/* Create a Save event and add it to the incoming queue */
		lash_event_t *event;
		if (!(event = lash_event_new_with_all(LASH_Save_File,
		                                      client->data_path))) {
			lash_error("Failed to allocate lash_event_t");
			client->pending_task = 0;
			return;
		}

		lash_client_add_event(client, event);
#endif /* LASH_OLD_API */
	}
}

static bool
get_task_id(method_call_t   *call,
            dbus_uint64_t   *task_id,
            DBusMessageIter *iter)
{
	DBusMessageIter xiter;

	if (!iter)
		iter = &xiter;

	if (!dbus_message_iter_init(call->message, iter)
	    || dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_UINT64) {
		lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_ARGS,
		                "Invalid arguments to method \"%s\": "
		                "Cannot find task ID in message",
		                call->method_name);
		return false;
	}

	dbus_message_iter_get_basic(iter, task_id);
	dbus_message_iter_next(iter);

	if (client_ptr->pending_task) {
		lash_dbus_error(call, LASH_DBUS_ERROR_UNFINISHED_TASK,
		                "Task %llu is unfinished, bouncing task %llu",
		                client_ptr->pending_task, *task_id);
		return false;
	}

	return true;
}

static void
lash_dbus_save(method_call_t *call)
{
	lash_debug("Save");

	dbus_uint64_t task_id;

	if (!get_task_id(call, &task_id, NULL))
		return;

	/* Send a void return here to avoid possible timeout */
	call->reply = dbus_message_new_method_return(call->message);
	method_return_send(call);

	lash_new_save_task(client_ptr, task_id);
}

static void
lash_dbus_load(method_call_t *call)
{
	lash_debug("Load");

	DBusMessageIter iter;
	dbus_uint64_t task_id;
	const char *data_path;

	if (!get_task_id(call, &task_id, &iter))
		return;


	// TODO: Don't send the data path, the client should know it at all times
	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING) {
		lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_ARGS,
		                "Invalid arguments to method \"%s\": "
		                "Cannot find data path in message",
		                call->method_name);
		return;
	}
	dbus_message_iter_get_basic(&iter, &data_path);

	lash_strset(&client_ptr->data_path, data_path);

	client_ptr->pending_task = task_id;
	client_ptr->task_progress = 0;

	/* Check if a load callback has been registered */
	if (client_ptr->cb.load) {
		/* Send a void return here to avoid possible timeout */
		call->reply = dbus_message_new_method_return(call->message);
		method_return_send(call);

		/* Call the load callback; its return value dictates whether
		   to report success or failure back to the server */
		if (client_ptr->cb.load(client_ptr->ctx.load)) {
			report_success_or_failure(client_ptr, true);
		} else {
			lash_error("Client failed to load data");
			report_success_or_failure(client_ptr, false);
		}

		client_ptr->pending_task = 0;
	} else {
#ifndef LASH_OLD_API
		lash_dbus_error(call, LASH_DBUS_ERROR_GENERIC,
		                "Load callback not registered");
		client_ptr->pending_task = 0;
#else /* LASH_OLD_API */
		/* Create a Restore event and add it to the incoming queue */
		lash_event_t *event;
		if (!(event = lash_event_new_with_all(LASH_Restore_File,
		                                      client_ptr->data_path))) {
			lash_dbus_error(call, LASH_DBUS_ERROR_GENERIC,
			                "Failed to allocate lash_event_t");
			return;
		}

		lash_client_add_event(client_ptr, event);
#endif /* LASH_OLD_API */
	}
}

void
lash_new_save_data_set_task(lash_client_t *client,
                            dbus_uint64_t  task_id)
{
	method_msg_t *new_call;
	DBusMessageIter *iter, *array_iter;

/* An ugly hack for the backwards compat API */
#ifdef LASH_OLD_API
	new_call = &client->unsent_configs;
	iter = &client->iter;
	array_iter = &client->array_iter;
#else /* !LASH_OLD_API */
	method_msg_t _new_call;
	DBusMessageIter _iter, _array_iter;
	new_call = &_new_call;
	iter = &_iter;
	array_iter = &_array_iter;
#endif

	client->pending_task = task_id;
	client->task_progress = 0;

	if (!method_call_init(new_call, client->dbus_service,
	                      NULL,
	                      method_default_handler,
	                      "org.nongnu.LASH",
	                      "/",
	                      "org.nongnu.LASH.Server",
	                      "CommitDataSet")) {
		lash_error("Failed to initialise CommitDataSet method call");
		goto fail;
	}

	dbus_message_iter_init_append(new_call->message, iter);

	if (!dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT64, &task_id)) {
		lash_error("Failed to write task ID");
		goto fail_unref;
	}

	if (!dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "{sv}", array_iter)) {
		lash_error("Failed to open config array container");
		goto fail_unref;
	}

	if (client->cb.save_data_set) {
		struct _lash_config_handle cfg;

		/* Call the save callback */
		cfg.iter = array_iter;
		cfg.is_read = false;
		if (!client->cb.save_data_set(&cfg,
		                              client->ctx.save_data_set)) {
			lash_error("Callback failed to save data set");
			dbus_message_iter_close_container(iter, array_iter);
			goto fail_unref;
		}

		if (!dbus_message_iter_close_container(iter, array_iter)) {
			lash_error("Failed to close array container");
			goto fail_unref;
		}

		/* Succesfully sending the data set implies success */
		if (!method_send(new_call, false)) {
			lash_error("Failed to send CommitDataSet method call");
			goto fail;
		}

		lash_debug("Sent data set message");

		client->pending_task = 0;
	} else {
#ifndef LASH_OLD_API
		lash_error("SaveDataSet callback not registered");
		goto fail;
#else /* LASH_OLD_API */
		/* Create a SaveDataSet event and add it to the incoming queue */
		lash_event_t *event;
		if (!(event = lash_event_new_with_type(LASH_Save_Data_Set))) {
			lash_error("Failed to allocate lash_event_t");
			goto fail;
		}
		lash_client_add_event(client, event);
#endif /* LASH_OLD_API */
	}

	return;

fail_unref:
	dbus_message_unref(new_call->message);
fail:
	report_success_or_failure(client, false);
	client->pending_task = 0;
}

#if 0
static void
lash_dbus_save_data_set(method_call_t *call)
{
	lash_debug("SaveDataSet");

	dbus_uint64_t task_id;

	if (!get_task_id(call, &task_id, NULL))
		return;

	/* Send a void return here to avoid possible timeout */
	call->reply = dbus_message_new_method_return(call->message);
	method_return_send(call);

	lash_new_save_data_set_task(client_ptr, task_id);
}
#endif

static void
lash_dbus_load_data_set(method_call_t *call)
{
	lash_debug("LoadDataSet");

	DBusMessageIter iter, array_iter;
	dbus_uint64_t task_id;
	struct _lash_config_handle cfg;

	if (!get_task_id(call, &task_id, &iter))
		return;

	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
		lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_ARGS,
		                "Invalid arguments to method \"%s\": "
		                "Cannot find data set array in message",
		                call->method_name);
		return;
	}

	dbus_message_iter_recurse(&iter, &array_iter);

	cfg.iter = &array_iter;
	cfg.is_read = true;

	client_ptr->pending_task = task_id;
	client_ptr->task_progress = 0;

	if (client_ptr->cb.load_data_set) {
		/* Send a void return here to avoid possible timeout */
		call->reply = dbus_message_new_method_return(call->message);
		method_return_send(call);

		/* Call the load callback; its return value dictates whether
		   to report success or failure back to the server */
		if (client_ptr->cb.load_data_set(&cfg,
		                                 client_ptr->ctx.load_data_set)) {
			report_success_or_failure(client_ptr, true);
		} else {
			lash_error("Callback failed to load data set");
			report_success_or_failure(client_ptr, false);
		}

		client_ptr->pending_task = 0;
	} else {
#ifndef LASH_OLD_API
		lash_dbus_error(call, LASH_DBUS_ERROR_GENERIC,
		                "LoadDataSet callback not registered");
		client_ptr->pending_task = 0;
#else /* LASH_OLD_API */
		const char *key;
		int ret, type;

		union {
			double      d;
			uint32_t    u;
			const char *s;
			const void *v;
		} value;

		lash_event_t *event;
		lash_config_t *config;

		event = lash_event_new_with_type(LASH_Restore_Data_Set);
		if (!event) {
			lash_dbus_error(call, LASH_DBUS_ERROR_GENERIC,
			                "Failed to allocate lash_event_t");
			client_ptr->pending_task = 0;
			return;
		}
		lash_client_add_event(client_ptr, event);

		while ((ret = lash_config_read(&cfg, &key, &value, &type))) {
			if (ret == -1) {
				lash_dbus_error(call, LASH_DBUS_ERROR_GENERIC,
				                "Failed to read data set");
				break;
			}

			config = lash_config_new_with_key(key);
			if (!config) {
				lash_dbus_error(call, LASH_DBUS_ERROR_GENERIC,
				                "Failed to allocate lash_config_t");
				break;
			}

			if (type == LASH_TYPE_DOUBLE)
				lash_config_set_value_double(config, value.d);
			else if (type == LASH_TYPE_INTEGER)
				lash_config_set_value_int(config, value.u);
			else if (type == LASH_TYPE_STRING)
				lash_config_set_value_string(config, value.s);
			else if (type == LASH_TYPE_RAW)
				lash_config_set_value(config, value.v, (size_t) ret);
			else {
				lash_error("Unknown config type '%c'", type);
				lash_config_destroy(config);
				continue;
			}

			lash_client_add_config(client_ptr, config);
		}
#endif /* LASH_OLD_API */
	}
}

void
lash_new_quit_task(lash_client_t *client)
{
	if (client->pending_task)
		lash_error("Warning: Task %llu is unfinished, quitting anyway", client->pending_task);

	/* Check if a quit callback has been registered */
	if (client->cb.quit) {
		client->cb.quit(client->ctx.quit);
	} else {
#ifndef LASH_OLD_API
		lash_error("Quit callback not registered");
#else /* LASH_OLD_API */
		/* Create a Quit event and add it to the incoming queue */
		lash_event_t *event;
		if (!(event = lash_event_new_with_type(LASH_Quit))) {
			lash_error("Failed to allocate lash_event_t");
			return;
		}

		lash_client_add_event(client, event);
#endif /* LASH_OLD_API */
	}
}

static void
lash_dbus_quit(method_call_t *call)
{
	/* Send a return right away to prevent LASH from getting an error
	   from the bus daemon if the clients decides to exit() in its
	   quit callback. */
	call->reply = dbus_message_new_method_return(call->message);
	method_return_send(call);

	lash_new_quit_task(client_ptr);
}

static void
lash_dbus_path_change_handler(DBusPendingCall *pending,
                              void            *data)
{
	DBusMessage *msg = dbus_pending_call_steal_reply(pending);
	lash_client_t *client = data;
	DBusError err;
	const char *new_path, *err_str;

	if (!msg) {
		lash_error("Cannot get method return from pending call");
		goto end;
	}

	if (!method_return_verify(msg, &err_str)) {
		lash_error("Server failed to commit path change: %s", err_str);
		goto end_unref_msg;
	}

	dbus_error_init(&err);

	if (!dbus_message_get_args(msg, &err,
	                           DBUS_TYPE_STRING,
	                           &new_path,
	                           DBUS_TYPE_INVALID)
	    || !new_path || !new_path[0]) {
		lash_error("Cannot get message argument: %s", err.message);
		dbus_error_free(&err);
		goto end_unref_msg;
	}

	lash_strset(&client->data_path, new_path);

	lash_debug("Server set save path to '%s'", new_path);

end_unref_msg:
	dbus_message_unref(msg);

end:
	dbus_pending_call_unref(pending);
}

static void
lash_dbus_try_path_change(method_call_t *call)
{
	lash_debug("TryPathChange");

	dbus_bool_t retval;

	if (client_ptr->pending_task) {
		lash_dbus_error(call, LASH_DBUS_ERROR_UNFINISHED_TASK,
		                "Cannot change path now; task %llu is unfinished",
		                client_ptr->pending_task);
		return;
	}

	if (!client_ptr->cb.path) {
		lash_dbus_error(call, LASH_DBUS_ERROR_GENERIC,
		                "Path change callback not registered");
		return;
	}

	/* Check whether the client says it's OK to change the path */
	if (client_ptr->cb.path(client_ptr->ctx.path)) {
		/* Client says path can be changed, send a commit message */
		method_call_new_void(client_ptr->dbus_service,
		                     client_ptr,
		                     lash_dbus_path_change_handler,
		                     true,
		                     "org.nongnu.LASH",
		                     "/",
		                     "org.nongnu.LASH.Server",
		                     "CommitPathChange");
		retval = true;
	} else {
		/* Client says path cannot be changed, server needs to
		   know it in order to cancel the pending path change */
		retval = false;
	}

	method_return_new_single(call, DBUS_TYPE_BOOLEAN, &retval);
}

static void
lash_dbus_client_name_changed(method_call_t *call)
{
	const char *new_name;
	DBusError err;

	dbus_error_init(&err);

	if (!dbus_message_get_args(call->message, &err,
	                           DBUS_TYPE_STRING, &new_name,
	                           DBUS_TYPE_INVALID)) {
		lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_ARGS,
		                "Invalid arguments to method \"%s\": %s",
		                call->method_name, err.message);
		dbus_error_free(&err);
		return;
	}

	lash_debug("Setting new client name to '%s'", new_name);

	if (!new_name[0])
		new_name = NULL;

	lash_strset(&client_ptr->name, new_name);

	/* Call ClientNameChanged callback */
	if (client_ptr->cb.name)
		client_ptr->cb.name(client_ptr->ctx.name);
}

#undef client_ptr

/*
 * Interface methods.
 */

METHOD_ARGS_BEGIN(Save)
  METHOD_ARG_DESCRIBE("task_id", "t", DIRECTION_IN)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Load)
  METHOD_ARG_DESCRIBE("task_id", "t", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("data_path", "s", DIRECTION_IN)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(LoadDataSet)
  METHOD_ARG_DESCRIBE("task_id", "t", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("configs", "a{sv}", DIRECTION_IN)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Quit)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(TrySave)
  METHOD_ARG_DESCRIBE("ok_to_save", "b", DIRECTION_OUT)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(TryPathChange)
  METHOD_ARG_DESCRIBE("ok_to_change", "b", DIRECTION_OUT)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(ClientNameChanged)
  METHOD_ARG_DESCRIBE("new_name", "s", DIRECTION_IN)
METHOD_ARGS_END

METHODS_BEGIN
  METHOD_DESCRIBE(Save, lash_dbus_save)
  METHOD_DESCRIBE(Load, lash_dbus_load)
  METHOD_DESCRIBE(LoadDataSet, lash_dbus_load_data_set)
  METHOD_DESCRIBE(Quit, lash_dbus_quit)
  METHOD_DESCRIBE(TrySave, lash_dbus_try_save)
  METHOD_DESCRIBE(TryPathChange, lash_dbus_try_path_change)
  METHOD_DESCRIBE(ClientNameChanged, lash_dbus_client_name_changed)
METHODS_END

/*
 * Interface description.
 */

INTERFACE_BEGIN(g_liblash_interface_client, "org.nongnu.LASH.Client")
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
INTERFACE_END
