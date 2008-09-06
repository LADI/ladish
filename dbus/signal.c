/*
 *   LASH
 *
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

#include <stdarg.h>

#include "common/debug.h"
#include "dbus/signal.h"
#include "dbus/service.h"

static void
signal_send(signal_msg_t *signal);

void
signal_new_single(service_t  *service,
                  const char *path,
                  const char *interface,
                  const char *name,
                  int         type,
                  const void *arg)
{
	signal_msg_t signal;
	DBusMessageIter iter;

	lash_debug("Sending signal %s.%s from %s", interface, name, path);

	if ((signal.message = dbus_message_new_signal(path, interface, name))) {
		dbus_message_iter_init_append(signal.message, &iter);
		if (dbus_message_iter_append_basic(&iter, type, arg)) {
			signal.connection = service->connection;
			signal_send(&signal);
		} else {
			lash_error("Ran out of memory trying to append signal argument");
		}

		dbus_message_unref(signal.message);
		signal.message = NULL;

		return;
	}

	lash_error("Ran out of memory trying to create new signal");
}

void
signal_new_valist (service_t  *service,
                   const char *path,
                   const char *interface,
                   const char *name,
                   int         type,
                               ...)
{
	signal_msg_t signal;
	va_list argp;

	lash_debug("Sending signal %s.%s from %s", interface, name, path);

	if ((signal.message = dbus_message_new_signal (path, interface, name))) {
		va_start(argp, type);
		if (dbus_message_append_args_valist(signal.message, type, argp)) {
			signal.connection = service->connection;
			signal_send(&signal);
		} else {
			lash_error("Ran out of memory trying to append signal argument(s)");
		}
		va_end(argp);

		dbus_message_unref(signal.message);
		signal.message = NULL;

		return;
	}

	lash_error("Ran out of memory trying to create new signal");
}

static void
signal_send(signal_msg_t *signal)
{
	if (!dbus_connection_send(signal->connection, signal->message, NULL)) {
		lash_error("Ran out of memory trying to queue signal");
	}

	dbus_connection_flush(signal->connection);
}

/* EOF */
