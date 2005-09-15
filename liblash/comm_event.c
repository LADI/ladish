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

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <lash/comm_event.h>

void
lash_comm_event_init(lash_comm_event_t * event)
{
	event->type = 0;
	event->event_data.number = 0;
}

void
lash_comm_event_free(lash_comm_event_t * event)
{
	switch (event->type) {
	case LASH_Comm_Event_Connect:
		if (event->event_data.connect)
			lash_connect_params_destroy(event->event_data.connect);
		break;
	case LASH_Comm_Event_Event:
		if (event->event_data.event)
			lash_event_destroy(event->event_data.event);
		break;
	case LASH_Comm_Event_Config:
		if (event->event_data.config)
			lash_config_destroy(event->event_data.config);
		break;
	case LASH_Comm_Event_Exec:
		if (event->event_data.exec)
			lash_exec_params_destroy(event->event_data.exec);
		break;
	default:
		break;
	}
}

lash_comm_event_t *
lash_comm_event_new()
{
	lash_comm_event_t *event;

	event = lash_malloc(sizeof(lash_comm_event_t));
	lash_comm_event_init(event);
	return event;
}

void
lash_comm_event_destroy(lash_comm_event_t * event)
{
	lash_comm_event_free(event);
	free(event);
}

void
lash_comm_event_set_type(lash_comm_event_t * event,
						 enum LASH_Comm_Event_Type type)
{
	event->type = type;
}

void
lash_comm_event_set_connect(lash_comm_event_t * event,
							lash_connect_params_t * params)
{
	lash_comm_event_set_type(event, LASH_Comm_Event_Connect);
	event->event_data.connect = params;
}

void
lash_comm_event_set_event(lash_comm_event_t * event, lash_event_t * cevent)
{
	lash_comm_event_set_type(event, LASH_Comm_Event_Event);
	event->event_data.event = cevent;
}

void
lash_comm_event_set_config(lash_comm_event_t * event, lash_config_t * config)
{
	lash_comm_event_set_type(event, LASH_Comm_Event_Config);
	event->event_data.config = config;
}

void
lash_comm_event_set_protocol_mismatch(lash_comm_event_t * event,
									  lash_protocol_t protocol)
{
	lash_comm_event_set_type(event, LASH_Comm_Event_Protocol_Mismatch);
	event->event_data.number = protocol;
}

void
lash_comm_event_set_exec(lash_comm_event_t * event,
						 lash_exec_params_t * params)
{
	lash_comm_event_set_type(event, LASH_Comm_Event_Exec);
	event->event_data.exec = params;
}

enum LASH_Comm_Event_Type
lash_comm_event_get_type(lash_comm_event_t * event)
{
	return event->type;
}

lash_connect_params_t *
lash_comm_event_take_connect(lash_comm_event_t * event)
{
	lash_connect_params_t *params;

	params = event->event_data.connect;
	event->event_data.connect = NULL;
	return params;
}

lash_config_t *
lash_comm_event_take_config(lash_comm_event_t * event)
{
	lash_config_t *config;

	config = event->event_data.config;
	event->event_data.config = NULL;
	return config;
}

lash_event_t *
lash_comm_event_take_event(lash_comm_event_t * event)
{
	lash_event_t *lash_event;

	lash_event = event->event_data.event;
	event->event_data.event = NULL;
	return lash_event;
}

lash_exec_params_t *
lash_comm_event_take_exec(lash_comm_event_t * event)
{
	lash_exec_params_t *params;

	params = event->event_data.exec;
	event->event_data.exec = NULL;
	return params;
}

long
lash_comm_event_get_number(const lash_comm_event_t * event)
{
	return event->event_data.number;
}

/* EOF */
