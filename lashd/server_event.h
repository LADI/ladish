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

#ifndef __LASHD_SERVER_EVENT_H__
#define __LASHD_SERVER_EVENT_H__

#include <uuid/uuid.h>

#include <lash/lash.h>
#include <lash/internal_headers.h>

enum Server_Event_Type
{
  Client_Connect = 1,
  Client_Disconnect,
  Client_Event,
  Client_Config,
  Client_Comm_Event
};

typedef struct _server_event server_event_t;

struct _server_event
{
  enum Server_Event_Type    type;
  unsigned long             conn_id;
  union
  {
    lash_connect_params_t *  lash_connect_params;
    lash_event_t *           lash_event;
    lash_config_t *          lash_config;
    lash_comm_event_t *      lash_comm_event;
  } data;
};

server_event_t * server_event_new ();
void             server_event_destroy ();

void server_event_set_type           (server_event_t * event, enum Server_Event_Type type);
void server_event_set_conn_id        (server_event_t * event, unsigned long id);
void server_event_set_lash_event      (server_event_t * event, lash_event_t * lash_event);
void server_event_set_lash_config     (server_event_t * event, lash_config_t * lash_config);
void server_event_set_lash_comm_event (server_event_t * event, lash_comm_event_t * lash_comm_event);
void server_event_set_lash_connect_params (server_event_t *, lash_connect_params_t * params);

lash_event_t *            server_event_take_lash_event          (server_event_t * event);
lash_config_t *           server_event_take_lash_config         (server_event_t * event);
lash_comm_event_t *       server_event_take_lash_comm_event     (server_event_t * event);
lash_connect_params_t *   server_event_take_lash_connect_params (server_event_t * event);

#endif /* __LASHD_SERVER_EVENT_H__ */
