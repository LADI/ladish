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
#define LASH_IFACE_CONTROL "org.nongnu.LASH.Control"

using namespace std;

lash_proxy::lash_proxy(Patchage* app)
	: _app(app)
	, _server_responding(false)
{
	dbus_bus_add_match(_app->_dbus_connection, "type='signal',interface='" DBUS_INTERFACE_DBUS "',member=NameOwnerChanged,arg0='" LASH_SERVICE "'", NULL);
	dbus_bus_add_match(_app->_dbus_connection, "type='signal',interface='" LASH_IFACE_CONTROL "',member=ProjectAdded", NULL);
	dbus_bus_add_match(_app->_dbus_connection, "type='signal',interface='" LASH_IFACE_CONTROL "',member=ProjectClosed", NULL);
	dbus_bus_add_match(_app->_dbus_connection, "type='signal',interface='" LASH_IFACE_CONTROL "',member=ProjectNameChanged", NULL);

	dbus_connection_add_filter(_app->_dbus_connection, dbus_message_hook, this, NULL);
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
		}
		else if (new_owner[0] == '\0')
		{
			me->info_msg((string)"LASH deactivated.");
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
