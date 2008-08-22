/* This file is part of Patchage.
 * Copyright (C) 2008 Dave Robillard <dave@drobilla.net>
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

#include "DBus.hpp"
#include <stdexcept>
#include <boost/format.hpp>
#include "Patchage.hpp"

#define DBUS_CALL_DEFAULT_TIMEOUT 1000 // in milliseconds


DBus::DBus(Patchage* app)
	: _app(app)
	, _connection(NULL)
{
	dbus_error_init(&_error);

	// Connect to the bus
	_connection = dbus_bus_get(DBUS_BUS_SESSION, &_error);
	if (dbus_error_is_set(&_error)) {
		app->error_msg("dbus_bus_get() failed");
		app->error_msg(_error.message);
		dbus_error_free(&_error);
	}

	dbus_connection_setup_with_g_main(_connection, NULL);
}


bool
DBus::call(
		bool response_expected,
		const char * service,
		const char * object,
		const char * iface,
		const char * method,
		DBusMessage ** reply_ptr_ptr,
		int in_type,
		va_list ap)
{
	DBusMessage* request_ptr;
	DBusMessage* reply_ptr;

	request_ptr = dbus_message_new_method_call(
		service,
		object,
		iface,
		method);
	if (!request_ptr) {
		throw std::runtime_error("dbus_message_new_method_call() returned 0");
	}

	dbus_message_append_args_valist(request_ptr, in_type, ap);

	// send message and get a handle for a reply
	reply_ptr = dbus_connection_send_with_reply_and_block(
		_connection,
		request_ptr,
		DBUS_CALL_DEFAULT_TIMEOUT,
		&_error);

	dbus_message_unref(request_ptr);

	if (!reply_ptr) {
		if (response_expected) {
			_app->error_msg(str(boost::format("no reply from server when calling method '%s'"
					", error is '%s'") % method % _error.message));
		}
		dbus_error_free(&_error);
	} else {
		*reply_ptr_ptr = reply_ptr;
	}

	return reply_ptr;
}

bool
DBus::call(
		bool response_expected,
		const char * serivce,
		const char * object,
		const char * iface,
		const char * method,
		DBusMessage ** reply_ptr_ptr,
		int in_type,
		...)
{
	bool ret;
	va_list ap;

	va_start(ap, in_type);

	ret = _app->dbus()->call(
		response_expected,
		serivce,
		object,
		iface,
		method,
		reply_ptr_ptr,
		in_type,
		ap);

	va_end(ap);

	return (ap != NULL);
}
