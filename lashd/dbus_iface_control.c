/*
 *   LASH
 *
 *   Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
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

#include "common/safety.h"
#include "common/debug.h"
#include "common/klist.h"

#include "dbus/interface.h"
#include "dbus/error.h"

#include "server.h"
#include "project.h"
#include "client.h"
#include "client_dependency.h"
#include "appdb.h"

#define INTERFACE_NAME "org.nongnu.LASH.Control"

static bool
maybe_add_dict_entry_string(DBusMessageIter *dict_iter_ptr,
                            const char      *key,
                            const char      *value)
{
	DBusMessageIter dict_entry_iter;

	if (!value)
		return true;

	if (!dbus_message_iter_open_container(dict_iter_ptr, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter))
		return false;

	if (!dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, (const void *) &key)) {
		dbus_message_iter_close_container(dict_iter_ptr, &dict_entry_iter);
		return false;
	}

	method_iter_append_variant(&dict_entry_iter, DBUS_TYPE_STRING, &value);

	if (!dbus_message_iter_close_container(dict_iter_ptr, &dict_entry_iter))
		return false;

	return true;
}

static bool
add_dict_entry_uint32(
	DBusMessageIter * dict_iter_ptr,
	const char * key,
	dbus_uint32_t value)
{
	DBusMessageIter dict_entry_iter;

	if (!value)
		return true;

	if (!dbus_message_iter_open_container(dict_iter_ptr, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter))
		return false;

	if (!dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, (const void *) &key)) {
		dbus_message_iter_close_container(dict_iter_ptr, &dict_entry_iter);
		return false;
	}

	method_iter_append_variant(&dict_entry_iter, DBUS_TYPE_UINT32, &value);

	if (!dbus_message_iter_close_container(dict_iter_ptr, &dict_entry_iter))
		return false;

	return true;
}

static bool
add_dict_entry_bool(
	DBusMessageIter * dict_iter_ptr,
	const char * key,
	dbus_bool_t value)
{
	DBusMessageIter dict_entry_iter;

	if (!dbus_message_iter_open_container(dict_iter_ptr, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry_iter))
		return false;

	if (!dbus_message_iter_append_basic(&dict_entry_iter, DBUS_TYPE_STRING, (const void *) &key)) {
		dbus_message_iter_close_container(dict_iter_ptr, &dict_entry_iter);
		return false;
	}

	method_iter_append_variant(&dict_entry_iter, DBUS_TYPE_BOOLEAN, &value);

	if (!dbus_message_iter_close_container(dict_iter_ptr, &dict_entry_iter))
		return false;

	return true;
}

static void
lashd_dbus_projects_get_available(method_call_t *call)
{
	DBusMessageIter iter, array_iter, struct_iter, dict_iter;
	struct list_head * node_ptr;
	project_t * project_ptr;

	call->reply = dbus_message_new_method_return(call->message);
	if (!call->reply)
		goto fail;

	dbus_message_iter_init_append(call->reply, &iter);

	if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(sa{sv})", &array_iter))
		goto fail_unref;

	list_for_each (node_ptr, &g_server->all_projects) {
		project_ptr = list_entry(node_ptr, project_t, siblings_all);

		if (!dbus_message_iter_open_container(&array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter))
			goto fail_unref;

		if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &project_ptr->name)) {
			dbus_message_iter_close_container(&iter, &array_iter);
			goto fail_unref;
		}

		if (!dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter))
			goto fail_unref;

		if (!maybe_add_dict_entry_string(&dict_iter, "Description", project_ptr->description))
			goto fail_unref;

		if (!add_dict_entry_uint32(&dict_iter, "Modification Time", project_ptr->last_modify_time))
			goto fail_unref;

		if (!dbus_message_iter_close_container(&struct_iter, &dict_iter))
			goto fail_unref;

		if (!dbus_message_iter_close_container(&array_iter, &struct_iter))
			goto fail_unref;
	}

	if (!dbus_message_iter_close_container(&iter, &array_iter))
		goto fail_unref;

	return;

fail_unref:
	dbus_message_unref(call->reply);
	call->reply = NULL;

fail:
	lash_error("Ran out of memory trying to construct method return");
}

static void
lashd_dbus_project_open(method_call_t *call)
{
	DBusError err;
	const char *project_name;

	dbus_error_init(&err);

	if (!dbus_message_get_args(call->message, &err,
	                           DBUS_TYPE_STRING, &project_name,
	                           DBUS_TYPE_INVALID)) {
		lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_ARGS,
		                "Invalid arguments to method \"%s\": %s",
		                call->method_name, err.message);
		dbus_error_free(&err);
		return;
	}

	lash_info("Opening project '%s'", project_name);

	// TODO: Evaluate return, send error return if necessary
	server_project_restore_by_name(project_name);
}

static void
lashd_dbus_projects_get(method_call_t *call)
{
	struct list_head * node_ptr;
	project_t * project_ptr;
	DBusMessageIter iter, sub_iter;

	call->reply = dbus_message_new_method_return(call->message);
	if (!call->reply)
		goto fail;

	dbus_message_iter_init_append(call->reply, &iter);

	if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "s", &sub_iter))
		goto fail_unref;

	list_for_each(node_ptr, &g_server->loaded_projects)
	{
		project_ptr = list_entry(node_ptr, project_t, siblings_loaded);
		if (!dbus_message_iter_append_basic(&sub_iter, DBUS_TYPE_STRING, &project_ptr->name)) {
			dbus_message_iter_close_container(&iter, &sub_iter);
			goto fail_unref;
		}
	}

	if (!dbus_message_iter_close_container(&iter, &sub_iter))
		goto fail_unref;

	return;

fail_unref:
	dbus_message_unref(call->reply);
	call->reply = NULL;

fail:
	lash_error("Ran out of memory trying to construct method return");
}

static
void
lashd_dbus_project_get_properties(
	method_call_t * call)
{
	DBusMessageIter iter, dict_iter;
	project_t * project_ptr;

	DBusError error;
	const char *project_name;

	dbus_error_init(&error);

	if (!dbus_message_get_args(
		    call->message,
		    &error,
		    DBUS_TYPE_STRING, &project_name,
		    DBUS_TYPE_INVALID))
	{
		lash_dbus_error(
			call,
			LASH_DBUS_ERROR_INVALID_ARGS,
			"Invalid arguments to method \"%s\"",
			call->method_name);
		dbus_error_free(&error);
		return;
	}

	project_ptr = server_find_project_by_name(project_name);
	if (project_ptr == NULL)
	{
		lash_dbus_error(
			call,
			LASH_DBUS_ERROR_UNKNOWN_PROJECT,
			"Cannot find project \"%s\"",
			project_name);
		return;
	}

	call->reply = dbus_message_new_method_return(call->message);
	if (!call->reply)
		goto fail;

	dbus_message_iter_init_append(call->reply, &iter);

	if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter))
		goto fail_unref;

	if (!maybe_add_dict_entry_string(&dict_iter, "Description", project_ptr->description))
		goto fail_unref;

	if (!maybe_add_dict_entry_string(&dict_iter, "Notes", project_ptr->notes))
		goto fail_unref;

	if (!add_dict_entry_bool(&dict_iter, "Modified Status", project_ptr->modified_status))
		goto fail_unref;

	if (!dbus_message_iter_close_container(&iter, &dict_iter))
		goto fail_unref;

	return;

fail_unref:
	dbus_message_unref(call->reply);
	call->reply = NULL;

fail:
	lash_error("Ran out of memory trying to construct method return");
}

static
void
lashd_dbus_project_set_description(
	method_call_t * call)
{
	project_t * project_ptr;
	DBusError error;
	const char * project_name;
	const char * description;

	dbus_error_init(&error);

	if (!dbus_message_get_args(
		    call->message,
		    &error,
		    DBUS_TYPE_STRING, &project_name,
		    DBUS_TYPE_STRING, &description,
		    DBUS_TYPE_INVALID))
	{
		lash_dbus_error(
			call,
			LASH_DBUS_ERROR_INVALID_ARGS,
			"Invalid arguments to method \"%s\"",
			call->method_name);
		dbus_error_free(&error);
		return;
	}

	project_ptr = server_find_project_by_name(project_name);
	if (project_ptr == NULL)
	{
		lash_dbus_error(
			call,
			LASH_DBUS_ERROR_UNKNOWN_PROJECT,
			"Cannot find project \"%s\"",
			project_name);
		return;
	}

	project_set_description(project_ptr, description);
}

static
void
lashd_dbus_project_set_notes(
	method_call_t * call)
{
	project_t * project_ptr;
	DBusError error;
	const char * project_name;
	const char * notes;

	dbus_error_init(&error);

	if (!dbus_message_get_args(
		    call->message,
		    &error,
		    DBUS_TYPE_STRING, &project_name,
		    DBUS_TYPE_STRING, &notes,
		    DBUS_TYPE_INVALID))
	{
		lash_dbus_error(
			call,
			LASH_DBUS_ERROR_INVALID_ARGS,
			"Invalid arguments to method \"%s\"",
			call->method_name);
		dbus_error_free(&error);
		return;
	}

	project_ptr = server_find_project_by_name(project_name);
	if (project_ptr == NULL)
	{
		lash_dbus_error(
			call,
			LASH_DBUS_ERROR_UNKNOWN_PROJECT,
			"Cannot find project \"%s\"",
			project_name);
		return;
	}

	project_set_notes(project_ptr, notes);
}

static
void
lashd_dbus_project_get_clients(
	method_call_t * call)
{
	DBusMessageIter iter, array_iter, struct_iter;
	DBusError error;
	const char *project_name;
	project_t * project_ptr;
	client_t * client_ptr;
	struct list_head * node_ptr;
	const char * value_string;

	dbus_error_init(&error);

	if (!dbus_message_get_args(
		    call->message,
		    &error,
		    DBUS_TYPE_STRING, &project_name,
		    DBUS_TYPE_INVALID))
	{
		lash_dbus_error(
			call,
			LASH_DBUS_ERROR_INVALID_ARGS,
			"Invalid arguments to method \"%s\"",
			call->method_name);
		dbus_error_free(&error);
		return;
	}

	project_ptr = server_find_project_by_name(project_name);
	if (project_ptr == NULL)
	{
		lash_dbus_error(
			call,
			LASH_DBUS_ERROR_UNKNOWN_PROJECT,
			"Cannot find project \"%s\"",
			project_name);
		return;
	}

	call->reply = dbus_message_new_method_return(call->message);
	if (!call->reply)
		goto fail;

	dbus_message_iter_init_append(call->reply, &iter);

	if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(ss)", &array_iter))
		goto fail_unref;

	list_for_each(node_ptr, &project_ptr->clients)
	{
		client_ptr = list_entry(node_ptr, client_t, siblings);

		if (!dbus_message_iter_open_container(&array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter))
			goto fail_unref;

		value_string = client_ptr->id_str;

		if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &value_string))
			goto fail_close_array_iter;

		value_string = client_ptr->name;

		if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, &value_string))
			goto fail_close_array_iter;

		if (!dbus_message_iter_close_container(&array_iter, &struct_iter))
			goto fail_unref;
	}

	if (!dbus_message_iter_close_container(&iter, &array_iter))
		goto fail_unref;

	return;

fail_close_array_iter:
	dbus_message_iter_close_container(&iter, &array_iter);

fail_unref:
	dbus_message_unref(call->reply);
	call->reply = NULL;

fail:
	lash_error("Ran out of memory trying to construct method return");
}

static void
lashd_dbus_applications_get(method_call_t *call)
{
	DBusMessageIter iter, array_iter, struct_iter, dict_iter;
	struct list_head *node_ptr;
	struct lash_appdb_entry *entry_ptr;

	lash_info("Getting applications list");

	call->reply = dbus_message_new_method_return(call->message);
	if (!call->reply)
		goto fail;

	dbus_message_iter_init_append(call->reply, &iter);

	if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(sa{sv})", &array_iter))
		goto fail_unref;

	list_for_each(node_ptr, &g_server->appdb) {
		entry_ptr = list_entry(node_ptr, struct lash_appdb_entry, siblings);

		if (!dbus_message_iter_open_container(&array_iter, DBUS_TYPE_STRUCT, NULL, &struct_iter))
			goto fail_unref;

		if (!dbus_message_iter_append_basic(&struct_iter, DBUS_TYPE_STRING, (const void *) &entry_ptr->name)) {
			dbus_message_iter_close_container(&iter, &array_iter);
			goto fail_unref;
		}

		if (!dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter))
			goto fail_unref;

		if (!maybe_add_dict_entry_string(&dict_iter, "GenericName", entry_ptr->generic_name))
			goto fail_unref;

		if (!maybe_add_dict_entry_string(&dict_iter, "Comment", entry_ptr->comment))
			goto fail_unref;

		if (!maybe_add_dict_entry_string(&dict_iter, "Icon", entry_ptr->icon))
			goto fail_unref;

		if (!dbus_message_iter_close_container(&struct_iter, &dict_iter))
			goto fail_unref;

		if (!dbus_message_iter_close_container(&array_iter, &struct_iter))
			goto fail_unref;
	}

	if (!dbus_message_iter_close_container(&iter, &array_iter))
		goto fail_unref;

	return;

fail_unref:
	dbus_message_unref(call->reply);
	call->reply = NULL;
fail:
	return;
}

static void
lashd_dbus_client_get_dependencies(method_call_t *call)
{
	DBusError error;
	DBusMessageIter iter, array_iter, struct_iter, array_iter2;
	const char *project_name;
	project_t *project;
	client_t *client;
	client_dependency_t *dep;
	struct list_head *cnode, *dnode;
	char *id_str;

	dbus_error_init(&error);

	if (!dbus_message_get_args(call->message, &error,
	                           DBUS_TYPE_STRING,
	                           &project_name,
	                           DBUS_TYPE_INVALID)) {
		lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_ARGS,
		                "Invalid arguments to method \"%s\"",
		                call->method_name);
		dbus_error_free(&error);
		return;
	}

	project = server_find_project_by_name(project_name);
	if (!project) {
		lash_dbus_error(call, LASH_DBUS_ERROR_UNKNOWN_PROJECT,
		                "Cannot find project \"%s\"",
		                project_name);
		return;
	}

	id_str = lash_malloc(1, 37);

	call->reply = dbus_message_new_method_return(call->message);
	if (!call->reply)
		goto fail;

	dbus_message_iter_init_append(call->reply, &iter);

	if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "(sas)",
	                                      &array_iter)) {
		goto fail_unref;
	}

	list_for_each (cnode, &project->clients) {
		client = list_entry(cnode, client_t, siblings);

		if (list_empty(&client->dependencies))
			continue;

		if (!dbus_message_iter_open_container(&array_iter, DBUS_TYPE_STRUCT,
		                                      NULL, &struct_iter)) {
			goto fail_close1;
		}

		if (!dbus_message_iter_append_basic(&struct_iter,
		                                    DBUS_TYPE_STRING,
		                                    (const void *) &client->name)
		    || !dbus_message_iter_open_container(&struct_iter, DBUS_TYPE_ARRAY,
		                                         "s", &array_iter2)) {
			goto fail_close2;
		}

		list_for_each (dnode, &client->dependencies) {
			dep = list_entry(dnode, client_dependency_t, siblings);

			uuid_unparse(dep->client_id, id_str);

			if (!dbus_message_iter_append_basic(&array_iter2,
			                                    DBUS_TYPE_STRING,
			                                    (const void *) &id_str)) {
				goto fail_close3;
			}
		}

		if (!dbus_message_iter_close_container(&struct_iter, &array_iter2))
			goto fail_close2;

		if (!dbus_message_iter_close_container(&array_iter, &struct_iter))
			goto fail_close1;
	}

	if (!dbus_message_iter_close_container(&iter, &array_iter))
		goto fail_unref;

	free(id_str);
	return;

fail_close3:
	dbus_message_iter_close_container(&struct_iter, &array_iter2);
fail_close2:
	dbus_message_iter_close_container(&array_iter, &struct_iter);
fail_close1:
	dbus_message_iter_close_container(&iter, &array_iter);

fail_unref:
	dbus_message_unref(call->reply);
	call->reply = NULL;

fail:
	free(id_str);
	lash_error("Ran out of memory trying to construct method return");
}

static bool
lashd_dbus_get_client_dependency_modify_data(method_call_t  *call,
                                             project_t     **project,
                                             client_t      **client,
                                             uuid_t          dep_id)
{
	DBusError error;
	const char *project_name, *client_id_str, *dep_id_str;
	uuid_t client_id;
	project_t *proj;

	dbus_error_init(&error);

	if (!dbus_message_get_args(call->message, &error,
	                           DBUS_TYPE_STRING,
	                           &project_name,
	                           DBUS_TYPE_STRING,
	                           &client_id_str,
	                           DBUS_TYPE_STRING,
	                           &dep_id_str,
	                           DBUS_TYPE_INVALID)) {
		lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_ARGS,
		                "Invalid arguments to method \"%s\"",
		                call->method_name);
		dbus_error_free(&error);
		return false;
	}

	dbus_error_free(&error);

	if (uuid_parse((char *) client_id_str, client_id) != 0) {
		lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_CLIENT_ID,
		                "Cannot parse client ID string \"%s\"",
		                client_id_str);
		return false;
	}

	if (uuid_parse((char *) dep_id_str, dep_id) != 0) {
		lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_CLIENT_ID,
		                "Cannot parse dependency ID string \"%s\"",
		                dep_id_str);
		return false;
	}

	proj = server_find_project_by_name(project_name);
	if (!proj) {
		lash_dbus_error(call, LASH_DBUS_ERROR_UNKNOWN_PROJECT,
		                "Cannot find project \"%s\"",
		                project_name);
		return false;
	} else if (project)
		*project = proj;

	*client = project_get_client_by_id(&proj->clients, client_id);
	if (!*client) {
		lash_dbus_error(call, LASH_DBUS_ERROR_UNKNOWN_CLIENT,
		                "Cannot find client '%s'",
		                client_id_str);
		return false;
	}

	return true;
}

static void
lashd_dbus_client_add_dependency(method_call_t *call)
{
	project_t *project;
	client_t *client;
	uuid_t dep_id;

	if (lashd_dbus_get_client_dependency_modify_data(call, &project,
	                                                 &client, dep_id))
		client_dependency_add(&project->clients, client, dep_id);
}

static void
lashd_dbus_client_remove_dependency(method_call_t *call)
{
	client_t *client;
	uuid_t dep_id;

	if (lashd_dbus_get_client_dependency_modify_data(call, NULL,
	                                                 &client, dep_id))
		client_dependency_remove(&client->dependencies, dep_id);
}

static void
lashd_dbus_load_project_path(method_call_t *call)
{
	DBusError err;
	const char *project_path;

	dbus_error_init(&err);

	if (!dbus_message_get_args(call->message, &err,
	                           DBUS_TYPE_STRING, &project_path,
	                           DBUS_TYPE_INVALID)) {
		lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_ARGS,
		                "Invalid arguments to method \"%s\": %s",
		                call->method_name, err.message);
		dbus_error_free(&err);
		return;
	}

	lash_info("Loading project from %s", project_path);

	// TODO: Evaluate return, send error return if necessary
	server_project_restore_by_dir(project_path);
}

static void
lashd_dbus_project_move(method_call_t *call)
{
	DBusError err;
	const char *project_name, *new_path;
	project_t *project;

	dbus_error_init(&err);

	if (!dbus_message_get_args(call->message, &err,
	                           DBUS_TYPE_STRING, &project_name,
	                           DBUS_TYPE_STRING, &new_path,
	                           DBUS_TYPE_INVALID)) {
		lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_ARGS,
		                "Invalid arguments to method \"%s\"",
		                call->method_name);
		dbus_error_free(&err);
		return;
	}

	project = server_find_project_by_name(project_name);

	// TODO: Make project_move return bool so that we can return D-Bus error
	if (project)
		project_move(project, new_path);
}

static
void
lashd_dbus_project_rename(
	method_call_t * call)
{
	project_t * project_ptr;
	DBusError error;
	const char * old_project_name;
	const char * new_project_name;

	dbus_error_init(&error);

	if (!dbus_message_get_args(
		    call->message,
		    &error,
		    DBUS_TYPE_STRING, &old_project_name,
		    DBUS_TYPE_STRING, &new_project_name,
		    DBUS_TYPE_INVALID))
	{
		lash_dbus_error(
			call,
			LASH_DBUS_ERROR_INVALID_ARGS,
			"Invalid arguments to method \"%s\"",
			call->method_name);
		dbus_error_free(&error);
		return;
	}

	project_ptr = server_find_project_by_name(old_project_name);
	if (project_ptr == NULL)
	{
		lash_dbus_error(
			call,
			LASH_DBUS_ERROR_UNKNOWN_PROJECT,
			"Cannot find project \"%s\"",
			old_project_name);
		return;
	}

	project_rename(project_ptr, new_project_name);
}

static void
lashd_dbus_project_save(method_call_t *call)
{
	DBusError err;
	const char *project_name;

	dbus_error_init(&err);

	if (!dbus_message_get_args(call->message, &err,
	                           DBUS_TYPE_STRING, &project_name,
	                           DBUS_TYPE_INVALID)) {
		lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_ARGS,
		                "Invalid arguments to method \"%s\"",
		                call->method_name);
		dbus_error_free(&err);
		return;
	}

	if (!server_project_save_by_name(project_name)) {
		lash_dbus_error(call, LASH_DBUS_ERROR_UNKNOWN_PROJECT,
		                "Cannot find project \"%s\"", project_name);
	}
}

static void
lashd_dbus_project_close(method_call_t *call)
{
	DBusError err;
	const char *project_name;

	dbus_error_init(&err);

	if (!dbus_message_get_args(call->message, &err,
	                           DBUS_TYPE_STRING, &project_name,
	                           DBUS_TYPE_INVALID)) {
		lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_ARGS,
		                "Invalid arguments to method \"%s\"",
		                call->method_name);
		dbus_error_free(&err);
		return;
	}

	if (!server_project_close_by_name(project_name)) {
		lash_dbus_error(call, LASH_DBUS_ERROR_UNKNOWN_PROJECT,
		                "Cannot find project \"%s\"", project_name);
	}
}

static void
lashd_dbus_projects_save_all(method_call_t *call)
{
	lash_debug("Saving all projects");

	server_save_all_projects();
}

static void
lashd_dbus_projects_close_all(method_call_t *call)
{
	lash_debug("Closing all projects");

	server_close_all_projects();
}

static void
lashd_dbus_exit(method_call_t *call)
{
	g_server->quit = true;
}

void
lashd_dbus_signal_emit_project_appeared(const char *project_name,
                                        const char *project_path)
{
	signal_new_valist(g_server->dbus_service,
	                  "/", INTERFACE_NAME, "ProjectAppeared",
	                  DBUS_TYPE_STRING, &project_name,
	                  DBUS_TYPE_STRING, &project_path,
	                  DBUS_TYPE_INVALID);
}

void
lashd_dbus_signal_emit_project_disappeared(const char *project_name)
{
	signal_new_single(g_server->dbus_service,
	                  "/", INTERFACE_NAME, "ProjectDisappeared",
	                  DBUS_TYPE_STRING, &project_name);
}

void
lashd_dbus_signal_emit_project_name_changed(const char *old_name,
                                            const char *new_name)
{
	signal_new_valist(g_server->dbus_service,
	                  "/", INTERFACE_NAME, "ProjectNameChanged",
	                  DBUS_TYPE_STRING, &old_name,
	                  DBUS_TYPE_STRING, &new_name,
	                  DBUS_TYPE_INVALID);
}

void
lashd_dbus_signal_emit_project_path_changed(const char *project_name,
                                            const char *new_path)
{
	signal_new_valist(g_server->dbus_service,
	                  "/", INTERFACE_NAME, "ProjectPathChanged",
	                  DBUS_TYPE_STRING, &project_name,
	                  DBUS_TYPE_STRING, &new_path,
	                  DBUS_TYPE_INVALID);
}

void
lashd_dbus_signal_emit_project_description_changed(const char *project_name,
                                                   const char *description)
{
	signal_new_valist(g_server->dbus_service,
	                  "/", INTERFACE_NAME, "ProjectDescriptionChanged",
	                  DBUS_TYPE_STRING, &project_name,
	                  DBUS_TYPE_STRING, &description,
	                  DBUS_TYPE_INVALID);
}

void
lashd_dbus_signal_emit_project_notes_changed(const char *project_name,
                                             const char *notes)
{
	signal_new_valist(g_server->dbus_service,
	                  "/", INTERFACE_NAME, "ProjectNotesChanged",
	                  DBUS_TYPE_STRING, &project_name,
	                  DBUS_TYPE_STRING, &notes,
	                  DBUS_TYPE_INVALID);
}

void
lashd_dbus_signal_emit_project_saved(const char *project_name)
{
	signal_new_single(g_server->dbus_service,
	                  "/", INTERFACE_NAME, "ProjectSaved",
	                  DBUS_TYPE_STRING, &project_name);
}

void
lashd_dbus_signal_emit_project_loaded(const char *project_name)
{
	signal_new_single(g_server->dbus_service,
	                  "/", INTERFACE_NAME, "ProjectLoaded",
	                  DBUS_TYPE_STRING, &project_name);
}

void
lashd_dbus_signal_emit_client_appeared(const char *client_id,
                                       const char *project_name,
                                       const char *client_name)
{
	signal_new_valist(g_server->dbus_service,
	                  "/", INTERFACE_NAME, "ClientAppeared",
	                  DBUS_TYPE_STRING, &client_id,
	                  DBUS_TYPE_STRING, &project_name,
	                  DBUS_TYPE_STRING, &client_name,
	                  DBUS_TYPE_INVALID);
}

void
lashd_dbus_signal_emit_client_disappeared(const char *client_id,
                                          const char *project_name)
{
	signal_new_valist(g_server->dbus_service,
	                  "/", INTERFACE_NAME, "ClientDisappeared",
	                  DBUS_TYPE_STRING, &client_id,
	                  DBUS_TYPE_STRING, &project_name,
	                  DBUS_TYPE_INVALID);
}

void
lashd_dbus_signal_emit_client_name_changed(const char *client_id,
                                           const char *new_client_name)
{
	signal_new_valist(g_server->dbus_service,
	                  "/", INTERFACE_NAME, "ClientNameChanged",
	                  DBUS_TYPE_STRING, &client_id,
	                  DBUS_TYPE_STRING, &new_client_name,
	                  DBUS_TYPE_INVALID);
}

void
lashd_dbus_signal_emit_progress(uint8_t percentage)
{
	signal_new_single(g_server->dbus_service,
	                  "/", INTERFACE_NAME, "Progress",
	                  DBUS_TYPE_BYTE, &percentage);
}

METHOD_ARGS_BEGIN(ProjectsGetAvailable)
  METHOD_ARG_DESCRIBE("projects_list", "a(sa{sv})", DIRECTION_OUT)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(ProjectOpen)
  METHOD_ARG_DESCRIBE("project_name", "s", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("options", "a{sv}", DIRECTION_IN)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(ProjectsGet)
  METHOD_ARG_DESCRIBE("projects_list", "as", DIRECTION_OUT)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(ProjectGetProperties)
  METHOD_ARG_DESCRIBE("properties", "a{sv}", DIRECTION_OUT)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(ProjectSetDescription)
  METHOD_ARG_DESCRIBE("project_name", "s", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("description", "s", DIRECTION_IN)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(ProjectSetNotes)
  METHOD_ARG_DESCRIBE("project_name", "s", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("notes", "s", DIRECTION_IN)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(ProjectGetClients)
  METHOD_ARG_DESCRIBE("project_name", "s", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("client_list", "a(ss)", DIRECTION_OUT)
METHOD_ARGS_END

// TODO: GetProjectPath

METHOD_ARGS_BEGIN(ApplicationsGet)
  METHOD_ARG_DESCRIBE("applications", "a(sa{sv})", DIRECTION_OUT)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(ClientGetDependencies)
  METHOD_ARG_DESCRIBE("project_name", "s", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("dependencies", "a(sas)", DIRECTION_OUT)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(ClientAddDependency)
  METHOD_ARG_DESCRIBE("project_name", "s", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("client_id", "s", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("dependency_id", "s", DIRECTION_IN)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(ClientRemoveDependency)
  METHOD_ARG_DESCRIBE("project_name", "s", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("client_id", "s", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("dependency_id", "s", DIRECTION_IN)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(LoadProjectPath)
  METHOD_ARG_DESCRIBE("project_path", "s", DIRECTION_IN)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(ProjectMove)
  METHOD_ARG_DESCRIBE("project_name", "s", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("new_path", "s", DIRECTION_IN)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(ProjectRename)
  METHOD_ARG_DESCRIBE("old_project_name", "s", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("new_project_name", "s", DIRECTION_IN)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(ProjectSave)
  METHOD_ARG_DESCRIBE("project_name", "s", DIRECTION_IN)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(ProjectClose)
  METHOD_ARG_DESCRIBE("project_name", "s", DIRECTION_IN)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(ProjectsSaveAll)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(ProjectsCloseAll)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Exit)
METHOD_ARGS_END

METHODS_BEGIN
  METHOD_DESCRIBE(ProjectsGetAvailable, lashd_dbus_projects_get_available)
  METHOD_DESCRIBE(ProjectOpen, lashd_dbus_project_open)
  METHOD_DESCRIBE(ProjectsGet, lashd_dbus_projects_get)
  METHOD_DESCRIBE(ProjectGetProperties, lashd_dbus_project_get_properties)
  METHOD_DESCRIBE(ProjectSetDescription, lashd_dbus_project_set_description)
  METHOD_DESCRIBE(ProjectSetNotes, lashd_dbus_project_set_notes)
  METHOD_DESCRIBE(ProjectGetClients, lashd_dbus_project_get_clients)
  METHOD_DESCRIBE(ApplicationsGet, lashd_dbus_applications_get)
  METHOD_DESCRIBE(ClientGetDependencies, lashd_dbus_client_get_dependencies)
  METHOD_DESCRIBE(ClientAddDependency, lashd_dbus_client_add_dependency)
  METHOD_DESCRIBE(ClientRemoveDependency, lashd_dbus_client_remove_dependency)
  METHOD_DESCRIBE(LoadProjectPath, lashd_dbus_load_project_path)
  METHOD_DESCRIBE(ProjectMove, lashd_dbus_project_move)
  METHOD_DESCRIBE(ProjectRename, lashd_dbus_project_rename)
  METHOD_DESCRIBE(ProjectSave, lashd_dbus_project_save)
  METHOD_DESCRIBE(ProjectClose, lashd_dbus_project_close)
  METHOD_DESCRIBE(ProjectsSaveAll, lashd_dbus_projects_save_all)
  METHOD_DESCRIBE(ProjectsCloseAll, lashd_dbus_projects_close_all)
  METHOD_DESCRIBE(Exit, lashd_dbus_exit)
METHODS_END

/*
 * Interface signals.
 */

SIGNAL_ARGS_BEGIN(ProjectAppeared)
  SIGNAL_ARG_DESCRIBE("project_name", "s")
  SIGNAL_ARG_DESCRIBE("project_path", "s")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(ProjectDisappeared)
  SIGNAL_ARG_DESCRIBE("project_name", "s")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(ProjectNameChanged)
  SIGNAL_ARG_DESCRIBE("project_name_old", "s")
  SIGNAL_ARG_DESCRIBE("project_name_new", "s")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(ProjectPathChanged)
  SIGNAL_ARG_DESCRIBE("project_name", "s")
  SIGNAL_ARG_DESCRIBE("new_path", "s")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(ProjectModifiedStatusChanged)
  SIGNAL_ARG_DESCRIBE("project_name", "s")
  SIGNAL_ARG_DESCRIBE("modified", "b")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(ProjectDescriptionChanged)
  SIGNAL_ARG_DESCRIBE("project_name", "s")
  SIGNAL_ARG_DESCRIBE("description", "s")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(ProjectNotesChanged)
  SIGNAL_ARG_DESCRIBE("project_name", "s")
  SIGNAL_ARG_DESCRIBE("notes", "s")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(ProjectSaved)
  SIGNAL_ARG_DESCRIBE("project_name", "s")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(ProjectLoaded)
  SIGNAL_ARG_DESCRIBE("project_name", "s")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(ClientAppeared)
  SIGNAL_ARG_DESCRIBE("client_id", "s")
  SIGNAL_ARG_DESCRIBE("project_name", "s")
  SIGNAL_ARG_DESCRIBE("client_name", "s")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(ClientDisappeared)
  SIGNAL_ARG_DESCRIBE("client_id", "s")
  SIGNAL_ARG_DESCRIBE("project_name", "s")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(ClientNameChanged)
  SIGNAL_ARG_DESCRIBE("client_id", "s")
  SIGNAL_ARG_DESCRIBE("new_name", "s")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(ClientJackNameChanged)
  SIGNAL_ARG_DESCRIBE("client_id", "s")
  SIGNAL_ARG_DESCRIBE("jack_name", "s")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(ClientAlsaIdChanged)
  SIGNAL_ARG_DESCRIBE("client_id", "s")
  SIGNAL_ARG_DESCRIBE("alsa_id", "y")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(Progress)
  SIGNAL_ARG_DESCRIBE("percentage", "y")
SIGNAL_ARGS_END

SIGNALS_BEGIN
  SIGNAL_DESCRIBE(ProjectAppeared)
  SIGNAL_DESCRIBE(ProjectDisappeared)
  SIGNAL_DESCRIBE(ProjectModifiedStatusChanged)
  SIGNAL_DESCRIBE(ProjectNameChanged)
  SIGNAL_DESCRIBE(ProjectDescriptionChanged)
  SIGNAL_DESCRIBE(ProjectNotesChanged)
  SIGNAL_DESCRIBE(ProjectPathChanged)
  SIGNAL_DESCRIBE(ProjectSaved)
  SIGNAL_DESCRIBE(ProjectLoaded)
  SIGNAL_DESCRIBE(ClientAppeared)
  SIGNAL_DESCRIBE(ClientDisappeared)
  SIGNAL_DESCRIBE(ClientNameChanged)
  SIGNAL_DESCRIBE(ClientJackNameChanged)
  SIGNAL_DESCRIBE(ClientAlsaIdChanged)
  SIGNAL_DESCRIBE(Progress)
SIGNALS_END

/*
 * Interface description.
 */

INTERFACE_BEGIN(g_lashd_interface_control, INTERFACE_NAME)
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
  INTERFACE_EXPOSE_SIGNALS
INTERFACE_END
