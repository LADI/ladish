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

#include <stdlib.h>
#include <stdarg.h>
#include <string.h> /* strerror() */

#include "../common/safety.h"
#include "../common/debug.h"
#include "service.h"

service_t *
service_new(const char *service_name,
            bool       *quit,
            int         num_paths,
                        ...)
{
	if (!quit || num_paths < 1) {
		lash_debug("Invalid arguments");
		return NULL;
	}

	lash_debug("Creating D-Bus service");

	service_t *service;
	DBusError err;
	int r;
	va_list argp;
	object_path_t **path_pptr, *path_ptr;

	service = lash_calloc(1, sizeof(service_t));
	service->object_paths = lash_calloc(num_paths + 1,
	                                    sizeof(object_path_t *));

	dbus_error_init(&err);

	service->connection = dbus_bus_get(DBUS_BUS_SESSION, &err);
	if (dbus_error_is_set(&err)) {
		lash_error("Failed to get bus: %s", err.message);
		goto fail_free_err;
	}

	if (!(service->unique_name = dbus_bus_get_unique_name(service->connection))) {
		lash_error("Failed to read unique bus name");
		goto fail_free_err;
	}

	if (service_name) {
		r = dbus_bus_request_name(service->connection, service_name,
		                          DBUS_NAME_FLAG_DO_NOT_QUEUE,
		                          &err);
		if (r == -1) {
			lash_error("Failed to acquire bus name: %s", err.message);
			goto fail_free_err;
		} else if (r == DBUS_REQUEST_NAME_REPLY_EXISTS) {
			lash_error("Requested bus name already exists");
			goto fail_free_err;
		}

		service->name = lash_strdup(service_name);
	} else {
		service->name = lash_strdup("");
	}

	/* Populate the array and register object paths */
	va_start(argp, num_paths);
	for (path_pptr = service->object_paths;
	     (*path_pptr = va_arg(argp, object_path_t *));
	     ++path_pptr) {
		if (!object_path_register(service->connection,
		                          *path_pptr)) {
			lash_error("Failed to register object path");
			va_end(argp);
			goto fail;
		}
	}
	va_end(argp);

	/* Set the keepalive pointer */
	service->quit = quit;

	return service;

fail_free_err:
	dbus_error_free(&err);

fail:
	lash_free(&service->object_paths);
	service_destroy(service);

	/* Always free the object paths which were passed to us so that
	   way we can guarantee that the caller need not worry. */
	va_start(argp, num_paths);
	while ((path_ptr = va_arg(argp, object_path_t *)))
		free(path_ptr);
	va_end(argp);

	return NULL;
}

void
service_destroy(service_t *service)
{
	lash_debug("Destroying D-Bus service");

	if (service) {
		/* cut the bus connection */
		if (service->connection) {
			dbus_connection_unref(service->connection);
			service->connection = NULL;
		}

		/* reap the object path(s) */
		if (service->object_paths) {
			object_path_t **path_pptr;
			for (path_pptr = service->object_paths;
			     *path_pptr; ++path_pptr) {
				object_path_destroy(*path_pptr);
				*path_pptr = NULL;
			}
			free(service->object_paths);
			service->object_paths = NULL;
		}

		/* other stuff */
		if (service->name) {
			free(service->name);
			service->name = NULL;
		}
		service->unique_name = NULL;
		service->quit = NULL;

		/* finalize */
		free(service);
		service = NULL;
	}
#ifdef LASH_DEBUG
	else
		lash_debug("Nothing to destroy");
#endif
}

/* EOF */
