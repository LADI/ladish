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

#include <lash/types.h>
#include <lash/client.h>
#include <lash/xmalloc.h>
#include <lash/internal.h>

lash_client_t *
lash_client_new ()
{
  lash_client_t * client;
  client = lash_malloc0 (sizeof (lash_client_t));
  pthread_mutex_init (&client->configs_in_lock, NULL);
  pthread_mutex_init (&client->events_in_lock, NULL);
  pthread_mutex_init (&client->comm_events_out_lock, NULL);
  pthread_cond_init (&client->send_conditional, NULL);
  return client;
}

void
lash_client_destroy (lash_client_t * client)
{
  pthread_mutex_destroy (&client->configs_in_lock);
  pthread_mutex_destroy (&client->events_in_lock);
  pthread_mutex_destroy (&client->comm_events_out_lock);
  pthread_cond_destroy  (&client->send_conditional);

  lash_client_set_class        (client, NULL);

  lash_args_destroy (client->args);

  free (client);
}

const char *
lash_client_get_class (const lash_client_t * client)
{
  return client->class;
}

void
lash_client_set_class (lash_client_t * client, const char * class)
{
  set_string_property (client->class, class);
}

/* EOF */

