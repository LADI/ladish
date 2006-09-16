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

#include <stdlib.h>

#include <lash/types.h>
#include <lash/internal.h>
#include <lash/list.h>
#include <lash/event.h>

lash_event_t *
lash_event_new()
{
	lash_event_t *event;

	event = lash_malloc0(sizeof(lash_event_t));
	uuid_clear(event->client_id);
	return event;
}

lash_event_t *
lash_event_new_with_type(enum LASH_Event_Type type)
{
	lash_event_t *event;

	event = lash_event_new();
	lash_event_set_type(event, type);
	return event;
}

lash_event_t *
lash_event_new_with_all(enum LASH_Event_Type type, const char *string)
{
	lash_event_t *event;

	event = lash_event_new();
	lash_event_set_type(event, type);
	lash_event_set_string(event, string);
	return event;
}

void
lash_event_free(lash_event_t * event)
{
	event->type = 0;
	lash_event_set_string(event, NULL);
	lash_event_set_project(event, NULL);
}

void
lash_event_destroy(lash_event_t * event)
{
	lash_event_free(event);
	free(event);
}

enum LASH_Event_Type
lash_event_get_type(const lash_event_t * event)
{
	return event->type;
}

void
lash_event_set_type(lash_event_t * event, enum LASH_Event_Type type)
{
	event->type = type;
}

const char *
lash_event_get_string(const lash_event_t * event)
{
	return event->string;
}

void
lash_event_set_string(lash_event_t * event, const char *string)
{
	set_string_property(event->string, string);
}

const char *
lash_event_get_project(const lash_event_t * event)
{
	return event->project;
}

void
lash_event_set_project(lash_event_t * event, const char *project)
{
	set_string_property(event->project, project);
}

void
lash_event_get_client_id(const lash_event_t * event, uuid_t id)
{
	uuid_copy(id, event->client_id);
}

void
lash_event_set_client_id(lash_event_t * event, uuid_t id)
{
	uuid_copy(event->client_id, id);
}

void
lash_event_set_alsa_client_id(lash_event_t * event, unsigned char alsa_id)
{
	char id[2];

	lash_str_set_alsa_client_id(id, alsa_id);

	lash_event_set_type(event, LASH_Alsa_Client_ID);
	lash_event_set_string(event, id);
}

unsigned char
lash_event_get_alsa_client_id(const lash_event_t * event)
{
	return lash_str_get_alsa_client_id(lash_event_get_string(event));
}

void
lash_str_set_alsa_client_id(char *str, unsigned char alsa_id)
{
	str[0] = (char)alsa_id;
	str[1] = '\0';
}

unsigned
	char
lash_str_get_alsa_client_id(const char *str)
{
	return (unsigned char)*str;
}

/* EOF */
