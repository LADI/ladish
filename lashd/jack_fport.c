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
#include <string.h>
#include <stdio.h>

#include "common/safety.h"
#include "common/debug.h"

#include "jack_fport.h"

jack_fport_t *
jack_fport_new(jack_port_id_t  id,
               const char     *name)
{
	jack_fport_t *port;
	char *ptr;
	size_t len;

	port = lash_calloc(1, sizeof(jack_fport_t));

	port->id = id;
	port->name = lash_strdup(name);

	/* Save the client and port name if we can
	   parse them from the port's full name */
	if ((ptr = strchr(name, ':'))) {
		port->port_name = lash_strdup(++ptr);
		len = strlen(name) - strlen(ptr);
		port->client_name = lash_malloc(1, len);
		strncpy(port->client_name, name, --len);
		port->client_name[len] = '\0';
	} else {
		/* Setting both client and port name to the port's
		   full name when we can't parse the full name is
		   a bad hack, but it's better than NULL */
		port->client_name = lash_strdup(name);
		port->port_name = lash_strdup(name);
	}

	return port;
}

void
jack_fport_destroy(jack_fport_t *port)
{
	if (port) {
		lash_free(&port->name);
		lash_free(&port->client_name);
		lash_free(&port->port_name);
		free(port);
	}
#ifdef LASH_DEBUG
	else
		lash_debug("Cannot destroy NULL foreign port");
#endif
}

/* EOF */
