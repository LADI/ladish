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

#ifndef __LASH_CONN_MGR_H__
#define __LASH_CONN_MGR_H__

#include <pthread.h>

#include <lash/lash.h>

#include "server_event.h"

typedef struct _conn_mgr conn_mgr_t;

struct _conn_mgr
{
  int             listen_socket;
  fd_set          sockets;
  int             fd_max;
  
  struct _server * server;
        
  lash_list_t *    open_connections;
  pthread_mutex_t connections_lock;
  lash_list_t *    connections;
  
  /* events for the server */
  pthread_t       recv_thread;
  
  /* events from server to clients */
  pthread_t       send_thread;
  pthread_mutex_t client_event_lock;
  pthread_cond_t  client_event_cond;
  lash_list_t *    client_events;
  
  int             quit;
};

extern int no_v6;

conn_mgr_t * conn_mgr_new     (struct _server * server);
void         conn_mgr_destroy (conn_mgr_t * conn_mgr);

int              conn_mgr_start             (conn_mgr_t * conn_mgr);
void             conn_mgr_stop              (conn_mgr_t * conn_mgr);

unsigned long    conn_mgr_get_event_count        (conn_mgr_t * conn_mgr);
server_event_t * conn_mgr_get_event              (conn_mgr_t * conn_mgr);

void             conn_mgr_send_client_event          (conn_mgr_t * conn_mgr, server_event_t * event);
void             conn_mgr_send_client_lash_event      (conn_mgr_t * conn_mgr,
                                                      unsigned long conn_id,
                                                      lash_event_t * lash_event);
void             conn_mgr_send_client_lash_config     (conn_mgr_t * conn_mgr,
                                                      unsigned int conn_id,
                                                      lash_config_t * lash_config);
void             conn_mgr_send_client_lash_comm_event (conn_mgr_t * conn_mgr,
                                                      unsigned int conn_id,
                                                      lash_comm_event_t * event);

#endif /* __LASH_CONN_MGR_H__ */
