/*
 *   LASH
 *    
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

#include <lash/lash.h>
#include <lash/internal_headers.h>

#include "jack_fport.h"

jack_fport_t *
jack_fport_new()
{
	jack_fport_t *port;

	port = lash_malloc(sizeof(jack_fport_t));

	port->id = 0;
	port->name = NULL;

	return port;
}

jack_fport_t *
jack_fport_new_with_id(jack_port_id_t id)
{
	jack_fport_t *port;

	port = jack_fport_new();
	jack_fport_set_id(port, id);

	return port;
}

void
jack_fport_destroy(jack_fport_t * port)
{
	jack_fport_set_name(port, NULL);
	free(port);
}

void
jack_fport_set_id(jack_fport_t * port, jack_port_id_t id)
{
	port->id = id;
}

void
jack_fport_set_name(jack_fport_t * port, const char *name)
{
	set_string_property(port->name, name);
}

int
jack_fport_find_name(jack_fport_t * port, jack_client_t * jack_client)
{
	jack_port_t *jack_port;

	jack_port = jack_port_by_id(jack_client, port->id);

	if (!jack_port)
		return 1;

	jack_fport_set_name(port, jack_port_name(jack_port));
	return 0;
}

jack_port_id_t
jack_fport_get_id(const jack_fport_t * port)
{
	return port->id;
}

const char *
jack_fport_get_name(const jack_fport_t * port)
{
	return port->name;
}

/* EOF */
