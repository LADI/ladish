// -*- Mode: C++ ; indent-tabs-mode: t -*-
/* This file is part of Patchage.
 * Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
 * 
 * Patchage is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Patchage is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/format.hpp>

#include "lash_proxy.hpp"

#define LASH_SERVICE       "org.nongnu.LASH"
#define LASH_OBJECT        "/"
#define LASH_IFACE_CONTROL "org.nongnu.LASH.Control"

using namespace std;

lash_proxy::lash_proxy(Patchage* app)
	: _app(app)
	, _server_responding(false)
{
	list<string> projects;

	dbus_bus_add_match(_app->_dbus_connection, "type='signal',interface='" DBUS_INTERFACE_DBUS "',member=NameOwnerChanged,arg0='" LASH_SERVICE "'", NULL);
	dbus_bus_add_match(_app->_dbus_connection, "type='signal',interface='" LASH_IFACE_CONTROL "',member=ProjectAdded", NULL);
	dbus_bus_add_match(_app->_dbus_connection, "type='signal',interface='" LASH_IFACE_CONTROL "',member=ProjectClosed", NULL);
	dbus_bus_add_match(_app->_dbus_connection, "type='signal',interface='" LASH_IFACE_CONTROL "',member=ProjectNameChanged", NULL);

	dbus_connection_add_filter(_app->_dbus_connection, dbus_message_hook, this, NULL);

	// get initial list of projects
	// calling any method to updates server responding status
	// this also actiavtes lash object if it not activated already
	get_loaded_projects(projects);

	for (list<string>::iterator iter = projects.begin(); iter != projects.end(); iter++)
	{
		app->on_project_added(*iter);
	}

	_app->set_lash_availability(_server_responding);
}

lash_proxy::~lash_proxy()
{
}

void
lash_proxy::error_msg(const std::string& msg) const
{
	_app->error_msg((std::string)"[LASH] " + msg);
}

void
lash_proxy::info_msg(const std::string& msg) const
{
	_app->info_msg((std::string)"[LASH] " + msg);
}

DBusHandlerResult
lash_proxy::dbus_message_hook(
	DBusConnection * connection,
	DBusMessage * message,
	void * proxy)
{
	const char * project_name;
	const char *object_name;
	const char *old_owner;
	const char *new_owner;

	assert(proxy);
	lash_proxy * me = reinterpret_cast<lash_proxy *>(proxy);
	assert(me->_app->_dbus_connection);

	//me->info_msg("dbus_message_hook() called.");

	// Handle signals we have subscribed for in attach()

	if (dbus_message_is_signal(message, DBUS_INTERFACE_DBUS, "NameOwnerChanged"))
	{
		if (!dbus_message_get_args(
			    message, &me->_app->_dbus_error,
			    DBUS_TYPE_STRING, &object_name,
			    DBUS_TYPE_STRING, &old_owner,
			    DBUS_TYPE_STRING, &new_owner,
			    DBUS_TYPE_INVALID))
		{
			me->error_msg(str(boost::format("dbus_message_get_args() failed to extract NameOwnerChanged signal arguments (%s)") % me->_app->_dbus_error.message));
			dbus_error_free(&me->_app->_dbus_error);
			return DBUS_HANDLER_RESULT_HANDLED;
		}

		if ((string)object_name != LASH_SERVICE)
		{
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		}

		if (old_owner[0] == '\0')
		{
			me->info_msg((string)"LASH activated.");
			me->_app->set_lash_availability(true);
		}
		else if (new_owner[0] == '\0')
		{
			me->info_msg((string)"LASH deactivated.");
			me->_app->set_lash_availability(false);
		}

		return DBUS_HANDLER_RESULT_HANDLED;
	}

	if (dbus_message_is_signal(message, LASH_IFACE_CONTROL, "ProjectAdded"))
	{
		if (!dbus_message_get_args( message, &me->_app->_dbus_error,
					DBUS_TYPE_STRING, &project_name,
					DBUS_TYPE_INVALID))
		{
			me->error_msg(str(boost::format("dbus_message_get_args() failed to extract ProjectAdded signal arguments (%s)") % me->_app->_dbus_error.message));
			dbus_error_free(&me->_app->_dbus_error);
			return DBUS_HANDLER_RESULT_HANDLED;
		}

		me->info_msg((string)"Project '" + project_name + "' added.");
		me->_app->on_project_added(project_name);

		return DBUS_HANDLER_RESULT_HANDLED;
	}

	if (dbus_message_is_signal(message, LASH_IFACE_CONTROL, "ProjectClosed"))
	{
		if (!dbus_message_get_args(
			    message, &me->_app->_dbus_error,
			    DBUS_TYPE_STRING, &project_name,
			    DBUS_TYPE_INVALID))
		{
			me->error_msg(str(boost::format("dbus_message_get_args() failed to extract ProjectClosed signal arguments (%s)") % me->_app->_dbus_error.message));
			dbus_error_free(&me->_app->_dbus_error);
			return DBUS_HANDLER_RESULT_HANDLED;
		}

		me->info_msg((string)"Project '" + project_name + "' closed.");
		me->_app->on_project_closed(project_name);

		return DBUS_HANDLER_RESULT_HANDLED;
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

bool
lash_proxy::call(
	bool response_expected,
	const char* iface,
	const char* method,
	DBusMessage ** reply_ptr_ptr,
	int in_type,
	...)
{
	va_list ap;

	va_start(ap, in_type);

	_server_responding = _app->dbus_call(
		response_expected,
		LASH_SERVICE,
		LASH_OBJECT,
		iface,
		method,
		reply_ptr_ptr,
		in_type,
		ap);

	va_end(ap);

	return _server_responding;
}

void
lash_proxy::get_loaded_projects(
	list<string>& projects)
{
	DBusMessage* reply_ptr;
	const char * reply_signature;
	DBusMessageIter iter;
	DBusMessageIter array_iter;
	const char * project_name;

	if (!call(true, LASH_IFACE_CONTROL, "GetLoadedProjects", &reply_ptr, DBUS_TYPE_INVALID)) {
		return;
	}

	reply_signature = dbus_message_get_signature(reply_ptr);

	if (strcmp(reply_signature, "as") != 0)
	{
		error_msg((string)"GetLoadedProjects() reply signature mismatch. " + reply_signature);
		goto unref;
	}

	dbus_message_iter_init(reply_ptr, &iter);

	for (dbus_message_iter_recurse(&iter, &array_iter);
	     dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_INVALID;
	     dbus_message_iter_next(&array_iter))
	{
		dbus_message_iter_get_basic(&array_iter, &project_name);
		projects.push_back(project_name);
	}

unref:
	dbus_message_unref(reply_ptr);
}

void
lash_proxy::get_available_projects(
	list<string>& projects)
{
	DBusMessage * reply_ptr;
	const char * reply_signature;
	DBusMessageIter iter;
	DBusMessageIter array_iter;
	const char * project_name;

	if (!call(true, LASH_IFACE_CONTROL, "GetProjects", &reply_ptr, DBUS_TYPE_INVALID))
	{
		return;
	}

	reply_signature = dbus_message_get_signature(reply_ptr);

	if (strcmp(reply_signature, "as") != 0)
	{
		error_msg((string)"GetLoadedProjects() reply signature mismatch. " + reply_signature);
		goto unref;
	}

	dbus_message_iter_init(reply_ptr, &iter);

	for (dbus_message_iter_recurse(&iter, &array_iter);
	     dbus_message_iter_get_arg_type(&array_iter) != DBUS_TYPE_INVALID;
	     dbus_message_iter_next(&array_iter))
	{
		dbus_message_iter_get_basic(&array_iter, &project_name);
		projects.push_back(project_name);
	}

unref:
	dbus_message_unref(reply_ptr);
}

void
lash_proxy::load_project(
	const string& project_name)
{
	DBusMessage * reply_ptr;
	const char * project_name_cstr;

	project_name_cstr = project_name.c_str();

	if (!call(true, LASH_IFACE_CONTROL, "LoadProject", &reply_ptr, DBUS_TYPE_STRING, &project_name_cstr, DBUS_TYPE_INVALID))
	{
		return;
	}

	dbus_message_unref(reply_ptr);
}

void
lash_proxy::save_all_projects()
{
	DBusMessage * reply_ptr;

	if (!call(true, LASH_IFACE_CONTROL, "Save", &reply_ptr, DBUS_TYPE_INVALID))
	{
		return;
	}

	dbus_message_unref(reply_ptr);
}

void
lash_proxy::save_project(
	const string& project_name)
{
	DBusMessage * reply_ptr;
	const char * project_name_cstr;

	project_name_cstr = project_name.c_str();

	if (!call(true, LASH_IFACE_CONTROL, "ProjectSave", &reply_ptr, DBUS_TYPE_STRING, &project_name_cstr, DBUS_TYPE_INVALID))
	{
		return;
	}

	dbus_message_unref(reply_ptr);
}

void
lash_proxy::close_project(
	const string& project_name)
{
	DBusMessage * reply_ptr;
	const char * project_name_cstr;

	project_name_cstr = project_name.c_str();

	if (!call(true, LASH_IFACE_CONTROL, "ProjectClose", &reply_ptr, DBUS_TYPE_STRING, &project_name_cstr, DBUS_TYPE_INVALID))
	{
		return;
	}

	dbus_message_unref(reply_ptr);
}

void
lash_proxy::close_all_projects()
{
	DBusMessage * reply_ptr;

	if (!call(true, LASH_IFACE_CONTROL, "Clear", &reply_ptr, DBUS_TYPE_INVALID))
	{
		return;
	}

	dbus_message_unref(reply_ptr);
}
