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

#include <dbus/dbus.h>

#include "common.hpp"
#include "a2j_proxy.hpp"
#include "Patchage.hpp"
#include "globals.hpp"
#include "dbus_helpers.h"

#define A2J_SERVICE       "org.gna.home.a2jmidid"
#define A2J_OBJECT        "/"
#define A2J_IFACE_CONTROL "org.gna.home.a2jmidid.control"

static
void
error_msg(
	const string& msg)
{
	g_app->error_msg((string)"[A2J] " + msg);
}

static
void
info_msg(
	const string& msg)
{
	g_app->info_msg((string)"[A2J] " + msg);
}

struct a2j_proxy_impl
{
	void
	init();

	static
	DBusHandlerResult
	dbus_message_hook(
		DBusConnection * connection,
		DBusMessage * message,
		void * proxy);

	bool
	call(
		bool response_expected,
		const char* iface,
		const char* method,
		DBusMessage ** reply_ptr_ptr,
		int in_type,
		...);

	bool
	get_jack_client_name(
		string& jack_client_name_ref);

	bool
	is_started();

	bool _server_responding;
	string _jack_client_name;
};

a2j_proxy::a2j_proxy()
{
	_impl_ptr = new a2j_proxy_impl;
	_impl_ptr->init();
}

a2j_proxy::~a2j_proxy()
{
	delete _impl_ptr;
}

const char *
a2j_proxy::get_jack_client_name()
{
	return _impl_ptr->_jack_client_name.c_str();
}

void
a2j_proxy_impl::init()
{
	unsigned int status;

	_server_responding = false;

	patchage_dbus_add_match("type='signal',interface='" DBUS_INTERFACE_DBUS "',member=NameOwnerChanged,arg0='" A2J_SERVICE "'");

	patchage_dbus_add_filter(dbus_message_hook, this);

	// get jack client name
	// calling any method to updates server responding status
	// this also actiavtes a2j object if it not activated already
	get_jack_client_name(_jack_client_name);

	if (is_started())
	{
		status = A2J_STATUS_BRIDGE_STARTED;
	}
	else
	{
		if (!_server_responding)
		{
			status = A2J_STATUS_NO_RESPONSE;
		}
		else
		{
			status = A2J_STATUS_BRIDGE_STOPPED;
		}
	}

	g_app->set_a2j_status(status);
}

DBusHandlerResult
a2j_proxy_impl::dbus_message_hook(
	DBusConnection * connection,
	DBusMessage * message,
	void * proxy)
{
	const char * object_name;
	const char * old_owner;
	const char * new_owner;

	assert(proxy);
	//a2j_proxy_impl * me = reinterpret_cast<a2j_proxy_impl *>(proxy);

	//info_msg("dbus_message_hook() called.");

	// Handle signals we have subscribed for in attach()

	if (dbus_message_is_signal(message, DBUS_INTERFACE_DBUS, "NameOwnerChanged"))
	{
		if (!dbus_message_get_args(
			    message, &g_dbus_error,
			    DBUS_TYPE_STRING, &object_name,
			    DBUS_TYPE_STRING, &old_owner,
			    DBUS_TYPE_STRING, &new_owner,
			    DBUS_TYPE_INVALID))
		{
			error_msg(str(boost::format("dbus_message_get_args() failed to extract NameOwnerChanged signal arguments (%s)") % g_dbus_error.message));
			dbus_error_free(&g_dbus_error);
			return DBUS_HANDLER_RESULT_HANDLED;
		}

		if ((string)object_name != A2J_SERVICE)
		{
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		}

		if (old_owner[0] == '\0')
		{
			info_msg((string)"A2J activated.");
			g_app->set_a2j_status(A2J_STATUS_BRIDGE_STOPPED);
		}
		else if (new_owner[0] == '\0')
		{
			info_msg((string)"A2J deactivated.");
			g_app->set_a2j_status(A2J_STATUS_NO_RESPONSE);
		}

		return DBUS_HANDLER_RESULT_HANDLED;
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

bool
a2j_proxy_impl::call(
	bool response_expected,
	const char* iface,
	const char* method,
	DBusMessage ** reply_ptr_ptr,
	int in_type,
	...)
{
	va_list ap;

	va_start(ap, in_type);

	_server_responding = patchage_dbus_call_valist(
		response_expected,
		A2J_SERVICE,
		A2J_OBJECT,
		iface,
		method,
		reply_ptr_ptr,
		in_type,
		ap);

	va_end(ap);

	return _server_responding;
}

bool
a2j_proxy_impl::get_jack_client_name(
	string& jack_client_name_ref)
{
	DBusMessage * reply_ptr;
	const char * jack_client_name;

	if (!call(true, A2J_IFACE_CONTROL, "get_jack_client_name", &reply_ptr, DBUS_TYPE_INVALID))
	{
		return false;
	}

	if (!dbus_message_get_args(reply_ptr, &g_dbus_error, DBUS_TYPE_STRING, &jack_client_name, DBUS_TYPE_INVALID))
	{
		dbus_message_unref(reply_ptr);
		dbus_error_free(&g_dbus_error);
		error_msg("decoding reply of get_jack_client_name failed.");
		return false;
	}

	jack_client_name_ref = jack_client_name;

	dbus_message_unref(reply_ptr);

	return true;
}

bool
a2j_proxy::map_jack_port(
	const char * jack_port_name,
	string& alsa_client_name_ref,
	string& alsa_port_name_ref)
{
	DBusMessage * reply_ptr;
	dbus_uint32_t alsa_client_id;
	dbus_uint32_t alsa_port_id;
	const char * alsa_client_name;
	const char * alsa_port_name;

	if (!_impl_ptr->call(
				true,
				A2J_IFACE_CONTROL,
				"map_jack_port_to_alsa",
				&reply_ptr,
				DBUS_TYPE_STRING,
				&jack_port_name,
				DBUS_TYPE_INVALID))
	{
		return false;
	}

	if (!dbus_message_get_args(
				reply_ptr,
				&g_dbus_error,
				DBUS_TYPE_UINT32,
				&alsa_client_id,
				DBUS_TYPE_UINT32,
				&alsa_port_id,
				DBUS_TYPE_STRING,
				&alsa_client_name,
				DBUS_TYPE_STRING,
				&alsa_port_name,
				DBUS_TYPE_INVALID))
	{
		dbus_message_unref(reply_ptr);
		dbus_error_free(&g_dbus_error);
		error_msg("decoding reply of map_jack_port_to_alsa failed.");
		return false;
	}

	alsa_client_name_ref = alsa_client_name;
	alsa_port_name_ref = alsa_port_name;

	dbus_message_unref(reply_ptr);

	return true;
}

bool
a2j_proxy_impl::is_started()
{
	DBusMessage * reply_ptr;
	dbus_bool_t started;

	if (!call(true, A2J_IFACE_CONTROL, "is_started", &reply_ptr, DBUS_TYPE_INVALID))
	{
		return false;
	}

	if (!dbus_message_get_args(reply_ptr, &g_dbus_error, DBUS_TYPE_BOOLEAN, &started, DBUS_TYPE_INVALID))
	{
		dbus_message_unref(reply_ptr);
		dbus_error_free(&g_dbus_error);
		error_msg("decoding reply of is_started failed.");
		return false;
	}

	dbus_message_unref(reply_ptr);

	return started;
}
