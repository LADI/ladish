/*
 *   LASH
 *    
 *   Copyright (C) 2002, 2003 Robert Ham <rah@bash.sh>
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

#ifndef __LASH_CLIENT_H__
#define __LASH_CLIENT_H__

#include <pthread.h>

#include <lash/lash.h>
#include <lash/internal_headers.h>



struct _lash_client
{
  lash_args_t      *args;
  char             *class;

  short             server_connected;

  int               socket;

  pthread_mutex_t   events_in_lock;
  lash_list_t       *events_in;
  pthread_mutex_t   configs_in_lock;
  lash_list_t       *configs_in;
  pthread_mutex_t   comm_events_out_lock;
  lash_list_t       *comm_events_out;
  
  pthread_cond_t    send_conditional;

  int               recv_close;
  int               send_close;
  pthread_t         recv_thread;
  pthread_t         send_thread;
};

lash_client_t * lash_client_new ();
void           lash_client_destroy (lash_client_t *);

void lash_client_set_class (lash_client_t *, const char * class);


/* both of these return > 0 if a whole event was done, or
   -1 if there was an error */
int lash_comm_send_event (int socket, lash_comm_event_t * event);
int lash_comm_recv_event (int socket, lash_comm_event_t * event);

/* returns 0 on success, non-0 on error */
int lash_comm_connect_to_server (struct _lash_client *, const char * server, const char * service, struct _lash_connect_params *);

void * lash_comm_recv_run (void * data);
void * lash_comm_send_run (void * data);


#endif /* __LASH_CLIENT_H__ */

