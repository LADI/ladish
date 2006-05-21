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

#ifndef __LASHD_SERVER_H__
#define __LASHD_SERVER_H__

#include <lash/lash.h>
#include <lash/loader.h>

#include "conn_mgr.h"
#include "client.h"
#include "project.h"
#include "jack_mgr.h"
#include "alsa_mgr.h"

typedef struct _server server_t;

struct _server
{
  conn_mgr_t *    conn_mgr;
  jack_mgr_t *    jack_mgr;
#ifdef WITH_ALSA
  alsa_mgr_t *    alsa_mgr;
#else
  void * alsa_mgr;
#endif

  loader_t *      loader;
  int             loader_quit;

  char *          default_dir;
  lash_list_t *    projects;
  lash_list_t *    interfaces;

  lash_list_t *    server_events;
  pthread_mutex_t server_events_lock;
  pthread_cond_t  server_event_cond;
  
  int             quit;
};

server_t * server_new (const char * default_dir);
void       server_destroy (server_t * server);
void       server_create_loader  (server_t * server);

const char * server_get_ui_project (server_t * server);
conn_mgr_t * server_get_conn_mgr (server_t * server);
int          server_get_loader_quit (server_t * server);

void server_set_loader_quit (server_t * server, int quit);

void server_main (server_t * server);

project_t * server_find_project_by_name (server_t * server, const char * project_name);
const char * server_create_new_project_name (server_t * server);
void server_notify_interfaces (project_t * project, client_t * client, enum LASH_Event_Type type, const char * str);


void server_send_event (server_t * server, server_event_t * server_event);

#endif /* __LASHD_SERVER_H__ */
