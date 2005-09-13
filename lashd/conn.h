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

#ifndef __LASHD_CONN_H__
#define __LASHD_CONN_H__

#include <time.h>

#include <uuid/uuid.h>

#include <lash/lash.h>

#define CONN_TIMEOUT ((time_t) 10)

typedef struct _conn conn_t;

struct _conn
{
  unsigned long           id;
  int                     socket;
  
  pthread_mutex_t         lock;
  
  time_t                  recv_stamp;
  time_t                  ping_stamp;
};

void conn_init (conn_t * conn);
void conn_free (conn_t * conn);

conn_t * conn_new     ();
void     conn_destroy (conn_t *);

void conn_lock (conn_t * conn);
void conn_unlock (conn_t * conn);

const char * conn_get_str_id       (conn_t *);


/* ping stuf */

time_t conn_get_recv_stamp   (conn_t *);
int    conn_get_pinged       (conn_t *);
int    conn_ping_timed_out     (conn_t *, time_t);
int    conn_recv_timed_out     (conn_t *, time_t);

void conn_set_recv_stamp   (conn_t *);
void conn_set_ping_stamp   (conn_t *);


#endif /* __LASHD_CONN_H__ */
