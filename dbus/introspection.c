/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *   Copyright (C) 2008 Nedko Arnaudov
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>

#include "../common/safety.h"
#include "../common/debug.h"
#include "introspection.h"
#include "signal.h"
#include "method.h"

#define write_buf(args...) buf_ptr += sprintf(buf_ptr, ## args)

DBusMessage *
introspection_new(object_path_t *path)
{
	if (!path) {
		lash_debug("Invalid arguments");
		return NULL;
	}

	lash_debug("Creating introspection message");

	char *xml_data, *buf_ptr;
	const interface_t **iface_pptr;
	const method_t *method_ptr;
	const method_arg_t *method_arg_ptr;
	const signal_t *signal_ptr;
	const signal_arg_t *signal_arg_ptr;
	DBusMessage *msg;
	DBusMessageIter iter;

	/*
	 * Create introspection XML data.
	 */

	xml_data = lash_malloc(1, 16384);
	buf_ptr = xml_data;

	write_buf("<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
	          " \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
	          "<node name=\"%s\">\n", path->name);

	/* Add the object path's interfaces. */
	for (iface_pptr = (const interface_t **) path->interfaces;
	     iface_pptr && *iface_pptr;
	     ++iface_pptr) {
		write_buf("  <interface name=\"%s\">\n",
		          (*iface_pptr)->name);

		/* Add the interface's methods. */
		for (method_ptr = (const method_t *) (*iface_pptr)->methods;
		     method_ptr && method_ptr->name;
		     ++method_ptr) {
			write_buf("    <method name=\"%s\">\n",
			          method_ptr->name);

			/* Add the method's arguments. */
			for (method_arg_ptr = (const method_arg_t *) method_ptr->args;
			method_arg_ptr && method_arg_ptr->name;
			++method_arg_ptr) {
				write_buf("      <arg name=\"%s\" type=\"%s\" direction=\"%s\" />\n",
				          method_arg_ptr->name,
				          method_arg_ptr->type,
				          method_arg_ptr->direction == DIRECTION_IN ? "in" : "out");
			}
			write_buf("    </method>\n");
		}

		/* Add the interface's signals. */
		for (signal_ptr = (const signal_t *) (*iface_pptr)->signals;
		     signal_ptr && signal_ptr->name;
		     ++signal_ptr) {
			write_buf("    <signal name=\"%s\">\n",
			          signal_ptr->name);

			/* Add the signal's arguments. */
			for (signal_arg_ptr = (const signal_arg_t *) signal_ptr->args;
			signal_arg_ptr && signal_arg_ptr->name;
			++signal_arg_ptr) {
				write_buf("      <arg name=\"%s\" type=\"%s\" />\n",
				          signal_arg_ptr->name,
				          signal_arg_ptr->type);
			}
			write_buf("    </signal>\n");
		}
		write_buf("  </interface>\n");
	}
	write_buf("</node>");

	/*
	 * Create a D-Bus message from the XML data.
	 */

	if ((msg = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_RETURN))) {
		dbus_message_iter_init_append(msg, &iter);
		if (dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING,
		                                    (const void *) &xml_data)) {
			dbus_message_set_no_reply(msg, TRUE);
		} else {
			dbus_message_unref(msg);
			msg = NULL;
			lash_error("Failed to append data to introspection message");
		}
	} else {
		lash_error("Failed to create introspection message");
	}

	free(xml_data);
	xml_data = buf_ptr = NULL;

	return msg;
}

#undef write_buf

void
introspection_destroy(object_path_t *path)
{
	lash_debug("Destroying introspection message");

	if (path && path->introspection) {
		dbus_message_unref(path->introspection);
		path->introspection = NULL;
	}
#ifdef LASH_DEBUG
	else
		lash_debug("Nothing to destroy");
#endif
}

static bool
introspection_handler(const interface_t *interface,
                      method_call_t     *call)
{
	if (strcmp(call->method_name, "Introspect") == 0) {
		/* Try to construct the instrospection message */
		if ((call->reply = dbus_message_copy(((object_path_t *) call->context)->introspection))
		    && dbus_message_set_destination(call->reply, dbus_message_get_sender(call->message))
		    && dbus_message_set_reply_serial(call->reply, dbus_message_get_serial(call->message))) {
			return true;
		}

		/* Failed; clear the message data if it exists. */
		if (call->reply) {
			dbus_message_unref(call->reply);
			call->reply = NULL;
		}

		lash_error("Ran out of memory trying to copy introspection message");

		/* Even after an error we need to return true, because the
		   handler is only supposed to return false if a nonexistent
		   method is requested. */
		return true;
	}

	/* The requested method wasn't "Introspect". */
	return false;
}


/*
 * Interface description.
 */

METHOD_ARGS_BEGIN(Introspect)
  METHOD_ARG_DESCRIBE("xml_data", "s", DIRECTION_OUT)
METHOD_ARGS_END

METHODS_BEGIN
  METHOD_DESCRIBE(Introspect, NULL)
METHODS_END

INTERFACE_BEGIN(g_dbus_interface_dtor_introspectable, "org.freedesktop.DBus.Introspectable")
  INTERFACE_HANDLER(introspection_handler)
  INTERFACE_EXPOSE_METHODS
INTERFACE_END

/* EOF */
