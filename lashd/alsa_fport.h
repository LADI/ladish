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

#ifndef __LASHD_ALSA_FPORT_H__
#define __LASHD_ALSA_FPORT_H__

#ifdef WITH_ALSA

#include <pthread.h>

typedef struct _alsa_fport alsa_fport_t;

struct _alsa_fport
{
  unsigned char   client;
  unsigned char   port;
};

alsa_fport_t * alsa_fport_new ();
alsa_fport_t * alsa_fport_new_with_all (unsigned char client, unsigned char port);
void           alsa_fport_destroy (alsa_fport_t * port);

void alsa_fport_set_client (alsa_fport_t * fport, unsigned char client);
void alsa_fport_set_port   (alsa_fport_t * fport, unsigned char port);

unsigned char alsa_fport_get_client (const alsa_fport_t * fport);
unsigned char alsa_fport_get_port   (const alsa_fport_t * fport);

#endif /* WITH_ALSA */

#endif /* __LASHD_ALSA_FPORT_H__ */
