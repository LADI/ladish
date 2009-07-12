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

#ifndef __LASH_EVENT_H__
#define __LASH_EVENT_H__

#include <uuid/uuid.h>

#include "lash/types.h"

#ifdef __cplusplus
extern "C" {
#endif

lash_event_t * lash_event_new           (void);
lash_event_t * lash_event_new_with_type (enum LASH_Event_Type type);
lash_event_t * lash_event_new_with_all  (enum LASH_Event_Type type, const char * string);
void          lash_event_destroy       (lash_event_t * event);

enum LASH_Event_Type  lash_event_get_type      (const lash_event_t * event);
const char *         lash_event_get_string    (const lash_event_t * event);
const char *         lash_event_get_project   (const lash_event_t * event);
void                 lash_event_get_client_id (const lash_event_t * event, uuid_t id);
 
void lash_event_set_type      (lash_event_t * event, enum LASH_Event_Type type);
void lash_event_set_string    (lash_event_t * event, const char * string);
void lash_event_set_project   (lash_event_t * event, const char * project);
void lash_event_set_client_id (lash_event_t * event, uuid_t id);

void lash_event_set_alsa_client_id (lash_event_t * event, unsigned char alsa_id);
unsigned char lash_event_get_alsa_client_id (const lash_event_t * event);

void lash_str_set_alsa_client_id (char * str, unsigned char alsa_id);
unsigned char lash_str_get_alsa_client_id (const char * str);

#ifdef __cplusplus
}
#endif

#endif /* __LASH_EVENT_H__ */
