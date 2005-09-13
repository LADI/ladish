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

#ifndef __LASH_COMM_EVENT_H__
#define __LASH_COMM_EVENT_H__

#include <uuid/uuid.h>

#include <lash/loader.h>
#include <lash/internal_headers.h>

/*
 * Comm protocol versioning
 *
 * If the low-level protocol versions of client and
 * server don't match, the socket is immediately closed.
 *
 * Protocol versions:
 *
 * 0.4.0:   2
 * < 0.4.0: 1 (implied by LASH_Comm_Event_Connect,
 *             which the protocol field replaces)
 */

#define LASH_COMM_PROTOCOL_VERSION 2


enum LASH_Comm_Event_Type
{
  LASH_Comm_Event_Connect = 1,
  LASH_Comm_Event_Iface_Connect,
  LASH_Comm_Event_Event,
  LASH_Comm_Event_Config,
  LASH_Comm_Event_Exec,
  LASH_Comm_Event_Ping,
  LASH_Comm_Event_Pong,
  LASH_Comm_Event_Close,
  LASH_Comm_Event_Protocol_Mismatch
};

struct _lash_comm_event   
{
  enum LASH_Comm_Event_Type type;
  union
  {
    lash_connect_params_t *connect;
    lash_event_t          *event;  
    lash_config_t         *config;
    lash_exec_params_t    *exec;
    char                 *string;
    long                  number;
  } event_data;
};


void lash_comm_event_init (lash_comm_event_t * event);
void lash_comm_event_free (lash_comm_event_t * event);

lash_comm_event_t * lash_comm_event_new ();
void               lash_comm_event_destroy (lash_comm_event_t * event);

void lash_comm_event_set_type              (lash_comm_event_t *, enum LASH_Comm_Event_Type);
void lash_comm_event_set_connect           (lash_comm_event_t *, lash_connect_params_t *);
void lash_comm_event_set_event             (lash_comm_event_t *, lash_event_t *);
void lash_comm_event_set_config            (lash_comm_event_t *, lash_config_t *);
void lash_comm_event_set_protocol_mismatch (lash_comm_event_t *, lash_protocol_t);
void lash_comm_event_set_exec              (lash_comm_event_t *, lash_exec_params_t *);

enum LASH_Comm_Event_Type  lash_comm_event_get_type (lash_comm_event_t *);

lash_connect_params_t * lash_comm_event_take_connect (lash_comm_event_t * event);
lash_event_t *          lash_comm_event_take_event   (lash_comm_event_t * event);
lash_config_t *         lash_comm_event_take_config  (lash_comm_event_t * event);
lash_exec_params_t *    lash_comm_event_take_exec    (lash_comm_event_t * event);

#endif /* __LASH_COMM_EVENT_H__ */
