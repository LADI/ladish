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

#ifndef __LASHD_JACK_FPORT_H__
#define __LASHD_JACK_FPORT_H__

#include <pthread.h>
#include <jack/jack.h>

typedef struct _jack_fport jack_fport_t;

struct _jack_fport
{
  jack_port_id_t  id;
  char *          name;
};

jack_fport_t * jack_fport_new ();   
jack_fport_t * jack_fport_new_with_id (jack_port_id_t id);
void           jack_fport_destroy (jack_fport_t * port);

void jack_fport_set_id    (jack_fport_t * port, jack_port_id_t id);
void jack_fport_set_name  (jack_fport_t * port, const char * name);
int  jack_fport_find_name (jack_fport_t * port, jack_client_t * jack_client);

jack_port_id_t jack_fport_get_id   (const jack_fport_t * port);
const char *   jack_fport_get_name (const jack_fport_t * port);


#endif /* __LASHD_JACK_FPORT_H__ */
