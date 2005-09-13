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

#include "server_event.h"

void
server_event_init (server_event_t * event)
{
  event->type      = 0;
  event->conn_id   = 0;
  event->data.lash_event = NULL;
}

static void
server_event_free_data (server_event_t * event)
{
  if (event->data.lash_event)
    {
      switch (event->type)
        {
        case Client_Connect:
          lash_connect_params_destroy (event->data.lash_connect_params);
          break;
        case Client_Event:
          lash_event_destroy (event->data.lash_event);
          break;
        case Client_Config:
          lash_config_destroy (event->data.lash_config);
          break;
        case Client_Comm_Event:
          lash_comm_event_destroy (event->data.lash_comm_event);
		default:
		  break;
      }
      event->data.lash_event = NULL;
    }
}

void
server_event_free (server_event_t * event)
{
  server_event_free_data (event);
  event->type      = 0;
  event->conn_id   = 0;
}

server_event_t *
server_event_new ()
{
  server_event_t * event;
  event = lash_malloc (sizeof (server_event_t));
  server_event_init (event);
  return event;
}

void
server_event_destroy (server_event_t * event)
{
  server_event_free (event);
  free (event);
}

void
server_event_set_type (server_event_t * event, enum Server_Event_Type type)
{
  event->type = type;
}

void
server_event_set_conn_id   (server_event_t * event, unsigned long id)
{
  event->conn_id = id;
}

void
server_event_set_lash_event (server_event_t * event, lash_event_t * lash_event)
{
  server_event_free_data (event);
  
  event->type = Client_Event;
  event->data.lash_event = lash_event;
}

void
server_event_set_lash_config (server_event_t * event, lash_config_t * lash_config)
{
  server_event_free_data (event);
  
  event->type = Client_Config;
  event->data.lash_config = lash_config;
}


void
server_event_set_lash_comm_event (server_event_t * event, lash_comm_event_t * lash_comm_event)
{
  server_event_free_data (event);
  
  event->type = Client_Comm_Event;
  event->data.lash_comm_event = lash_comm_event;
}

void
server_event_set_lash_connect_params (server_event_t *event, lash_connect_params_t * params)
{
  server_event_free_data (event);

  event->type = Client_Connect;
  event->data.lash_connect_params = params;
}

lash_event_t *
server_event_take_lash_event (server_event_t * event)
{
  lash_event_t * lash_event;
  lash_event = event->data.lash_event;
  event->data.lash_event = NULL;
  return lash_event;
}

lash_config_t *
server_event_take_lash_config (server_event_t * event)
{
  lash_config_t * lash_config;
  lash_config = event->data.lash_config;
  event->data.lash_config = NULL;
  return lash_config;
}

lash_comm_event_t *
server_event_take_lash_comm_event (server_event_t * event)
{
  lash_comm_event_t * lash_comm_event;
  lash_comm_event = event->data.lash_comm_event;
  event->data.lash_comm_event = NULL;
  return lash_comm_event;
}

lash_connect_params_t *
server_event_take_lash_connect_params (server_event_t * event)
{
  lash_connect_params_t * params;
  params = event->data.lash_connect_params;
  event->data.lash_connect_params = NULL;
  return params;
}

/* EOF */

