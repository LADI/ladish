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


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
                     
#include <lash/lash.h>
#include <lash/internal_headers.h>

#include "conn.h"

void
conn_init (conn_t * conn)
{
  static unsigned long id = 1;
  conn->id = id;
  id++;
  conn->socket = 0;
  pthread_mutex_init (&conn->lock, NULL);
}

void
conn_free (conn_t * conn)
{
  if (conn->socket)
    close (conn->socket);
  conn->socket = 0;
  pthread_mutex_destroy (&conn->lock);
}

conn_t *
conn_new ()
{
  conn_t * conn;
  conn = lash_malloc (sizeof (conn_t));
  conn_init (conn);
  return conn;
}

void
conn_destroy (conn_t * conn)
{
  conn_free (conn);
  free (conn);
}

void
conn_lock (conn_t * conn)
{
  pthread_mutex_lock (&conn->lock);
}

void
conn_unlock (conn_t * conn)
{
  pthread_mutex_unlock (&conn->lock);
}

const char *
conn_get_str_id (conn_t * conn)
{
  static char * str_id = NULL;
  static size_t str_id_len = 0;

  const char * host;
  const char * port;
  size_t new_str_id_len;

  host = lash_lookup_peer_name (conn->socket);
  port = lash_lookup_peer_port (conn->socket);

  new_str_id_len = strlen (host) + 1 + strlen (port) + 1;

  if (new_str_id_len > str_id_len)
    {
      str_id_len = new_str_id_len;

      if (!str_id)
	str_id = lash_malloc (str_id_len);
      else
	str_id = lash_realloc (str_id, str_id_len);
    }

  sprintf (str_id, "%s:%s", host, port);

  return str_id;
}


/*
 * stuff for pings
 */

time_t
conn_get_recv_stamp   (conn_t * conn)
{
  return conn->recv_stamp;
}

int
conn_get_pinged       (conn_t * conn)
{
  return conn->ping_stamp != (time_t) -1;
}

int
conn_recv_timed_out (conn_t * conn, time_t time)
{
  return time - conn->recv_stamp > CONN_TIMEOUT;
}

int
conn_ping_timed_out (conn_t * conn, time_t time)
{
  if (conn_get_pinged (conn))
    {
      return time - conn->ping_stamp > CONN_TIMEOUT;
    }
  else
    return 0;
}

void
conn_set_recv_stamp   (conn_t * conn)
{
  conn->ping_stamp = (time_t) -1;
  conn->recv_stamp = time (NULL);
  if (conn->recv_stamp == (time_t) -1)
    {
      fprintf (stderr, "%s: could not get time to set recieve stamp for connection '%ld', aborting!: %s\n",
               __FUNCTION__, conn->id, strerror (errno));
      abort ();      
    }
}

void
conn_set_ping_stamp   (conn_t * conn)
{
  conn->ping_stamp = time (NULL);
  if (conn->ping_stamp == (time_t) -1)
    {
      fprintf (stderr, "%s: could not get time to set ping transmission stamp for connection '%ld', aborting!: %s\n",
               __FUNCTION__, conn->id, strerror (errno));
      abort ();      
    }
}



/* EOF */

