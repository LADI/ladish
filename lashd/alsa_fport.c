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

#include "alsa_fport.h"


alsa_fport_t *
alsa_fport_new()
{
	alsa_fport_t *fport;

	fport = lash_malloc(sizeof(alsa_fport_t));

	fport->client = 0;
	fport->port = 0;

	return fport;
}

alsa_fport_t *
alsa_fport_new_with_all(unsigned char client, unsigned char port)
{
	alsa_fport_t *fport;

	fport = alsa_fport_new();

	alsa_fport_set_client(fport, client);
	alsa_fport_set_port(fport, port);

	return fport;
}

void
alsa_fport_destroy(alsa_fport_t * fport)
{
	free(fport);
}

void
alsa_fport_set_client(alsa_fport_t * fport, unsigned char client)
{
	fport->client = client;
}

void
alsa_fport_set_port(alsa_fport_t * fport, unsigned char port)
{
	fport->port = port;
}

unsigned char
alsa_fport_get_client(const alsa_fport_t * fport)
{
	return fport->client;
}

unsigned char
alsa_fport_get_port(const alsa_fport_t * fport)
{
	return fport->port;
}

/*void
alsa_fport_lock (alsa_fport_t * fport)
{
  pthread_mutex_lock (&fport->lock);
}

void
alsa_fport_unlock (alsa_fport_t * fport)
{
  pthread_mutex_unlock (&fport->lock);
}*/

/* EOF */
