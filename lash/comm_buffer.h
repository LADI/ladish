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

#ifndef __LASH_COMM_BUFFER_H__
#define __LASH_COMM_BUFFER_H__

#include <lash/loader.h>
#include <lash/internal_headers.h>

void lash_buffer_from_comm_event_connect           (char ** buf_ptr, size_t * buf_size_ptr, lash_connect_params_t *);
void lash_buffer_from_comm_event_event             (char ** buf_ptr, size_t * buf_size_ptr, lash_event_t * event);
void lash_buffer_from_comm_event_config            (char ** buf_ptr, size_t * buf_size_ptr, lash_config_t * event);
void lash_buffer_from_comm_event_protocol_mismatch (char ** buf_ptr, size_t * buf_size_ptr, lash_protocol_t protocol);
void lash_buffer_from_comm_event_exec              (char ** buf_ptr, size_t * buf_size_ptr, lash_exec_params_t *);
/* for ping, pong, close */
void lash_buffer_from_comm_event                   (char ** buf_ptr, size_t * buf_size_ptr, lash_comm_event_t * event);

/* returns 0 on a valid connect event, non-zero otherwise */
int  lash_comm_event_from_buffer_connect           (char * buf, size_t buf_size, lash_comm_event_t *);
void lash_comm_event_from_buffer_event             (char * buf, size_t buf_size, lash_comm_event_t *);
void lash_comm_event_from_buffer_config            (char * buf, size_t buf_size, lash_comm_event_t *);
void lash_comm_event_from_buffer_protocol_mismatch (char * buf, size_t buf_size, lash_comm_event_t *);
void lash_comm_event_from_buffer_exec              (char * buf, size_t buf_size, lash_comm_event_t *);
void lash_comm_event_from_buffer                   (char * buf, size_t buf_size, lash_comm_event_t *);

#endif /* __LASH_COMM_BUFFER_H__ */
