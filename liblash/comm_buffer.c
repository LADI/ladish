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

#define _GNU_SOURCE /* strdup */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>

#include <lash/lash.h>
#include <lash/internal_headers.h>

void
lash_buffer_from_comm_event_connect(char **buf_ptr, size_t * buf_size_ptr,
									lash_connect_params_t * params)
{
	size_t buf_size;
	char *buf;
	size_t proj_name_size;
	size_t working_dir_size;
	size_t class_size;
	size_t arg_size;
	int i;
	uint32_t *iptr;
	char *ptr;
	char id_str[37];

	if (!buf_ptr)
		return;

	buf_size = sizeof(uint32_t) * 5;
	buf_size += proj_name_size =
		(params->project ? strlen(params->project) : 0) + 1;
	buf_size += working_dir_size = strlen(params->working_dir) + 1;
	buf_size += class_size = strlen(params->class) + 1;
	buf_size += sizeof(id_str);

	for (i = 0; i < params->argc; i++)
		buf_size += strlen(params->argv[i]) + 1;

	buf = lash_malloc(buf_size);

	iptr = (uint32_t *) buf;
	*iptr = htonl(LASH_Comm_Event_Connect);
	iptr++;

	*iptr = htonl(LASH_COMM_PROTOCOL_VERSION);
	iptr++;

	*iptr = htonl(params->protocol_version);
	iptr++;

	*iptr = htonl(params->flags);
	iptr++;

	ptr = (char *)iptr;
	if (params->project)
		memcpy(ptr, params->project, proj_name_size);
	else
		*ptr = 0;
	ptr += proj_name_size;

	memcpy(ptr, params->working_dir, working_dir_size);
	ptr += working_dir_size;

	memcpy(ptr, params->class, class_size);
	ptr += class_size;

	uuid_unparse(params->id, id_str);
	memcpy(ptr, id_str, sizeof(id_str));
	ptr += sizeof(id_str);

	iptr = (uint32_t *) ptr;
	*iptr = htonl(params->argc);
	iptr++;

	ptr = (char *)iptr;
	for (i = 0; i < params->argc; i++) {
		arg_size = strlen(params->argv[i]) + 1;
		memcpy(ptr, params->argv[i], arg_size);
		ptr += arg_size;
	}

	*buf_ptr = buf;
	*buf_size_ptr = buf_size;
}

int
lash_comm_event_from_buffer_connect(char *buf, size_t buf_size,
									lash_comm_event_t * event)
{
	lash_connect_params_t *params;
	uint32_t *iptr;
	char *ptr;
	int i;

	LASH_DEBUGARGS("buf_size: %d", buf_size);

	iptr = (uint32_t *) buf;
	event->type = ntohl(*iptr);
	iptr++;

	if (ntohl(*iptr) != LASH_COMM_PROTOCOL_VERSION)
		return -1;
	iptr++;

	params = lash_connect_params_new();

	params->protocol_version = ntohl(*iptr);
	iptr++;

	params->flags = ntohl(*iptr);
	iptr++;

	ptr = (char *)iptr;
	if (*ptr)
		lash_connect_params_set_project(params, ptr);
	ptr += strlen(ptr) + 1;

	lash_connect_params_set_working_dir(params, ptr);
	ptr += strlen(ptr) + 1;

	lash_connect_params_set_class(params, ptr);
	ptr += strlen(ptr) + 1;

	uuid_parse(ptr, params->id);
	ptr += sizeof(char[37]);

	iptr = (uint32_t *) ptr;
	params->argc = ntohl(*iptr);
	iptr++;

	params->argv = lash_malloc(params->argc * sizeof(char *));

	/* set the argv array */
	ptr = (char *)iptr;
	for (i = 0; i < params->argc; i++) {
		LASH_DEBUGARGS("recieving argv[%d] == '%s'", i, ptr);
		params->argv[i] = strdup(ptr);
		ptr += strlen(ptr) + 1;
	}

	event->event_data.connect = params;

	return 0;
}

void
lash_buffer_from_comm_event_event(char **buf_ptr, size_t * buf_size_ptr,
								  lash_event_t * event)
{
	size_t buf_size;
	char *buf;
	size_t string_size = 0;
	size_t project_size = 0;
	uint32_t *iptr;
	char *ptr;

	buf_size = sizeof(uint32_t) * 2;
	buf_size += sizeof(char[37]);

	if (event->string)
		buf_size += string_size = strlen(event->string) + 1;
	else
		buf_size += 1;

	if (event->project)
		buf_size += project_size = strlen(event->project) + 1;
	else
		buf_size += 1;

	buf = lash_malloc(buf_size);

	iptr = (uint32_t *) buf;
	*iptr = htonl(LASH_Comm_Event_Event);
	iptr++;

	*iptr = htonl(event->type);
	iptr++;

	ptr = (char *)iptr;
	uuid_unparse(event->client_id, ptr);
	ptr += sizeof(char[37]);

	if (event->string) {
		memcpy(ptr, event->string, string_size);
		ptr += string_size;
	} else {
		*ptr = '\0';
		ptr++;
	}

	if (event->project) {
		memcpy(ptr, event->project, project_size);
		ptr += project_size;
	} else {
		*ptr = '\0';
	}

	*buf_ptr = buf;
	*buf_size_ptr = buf_size;
}

void
lash_comm_event_from_buffer_event(char *buf, size_t buf_size,
								  lash_comm_event_t * comm_event)
{
	uint32_t *iptr;
	char *ptr;
	lash_event_t *event;

	comm_event->type = LASH_Comm_Event_Event;
	iptr = (uint32_t *) buf;
	iptr++;

	event = lash_event_new();

	lash_event_set_type(event, ntohl(*iptr));
	iptr++;

	ptr = (char *)iptr;
	uuid_parse(ptr, event->client_id);
	ptr += sizeof(char[37]);

	if (*ptr == '\0')
		ptr++;
	else {
		lash_event_set_string(event, ptr);
		ptr += strlen(event->string) + 1;
	}

	if (*ptr != '\0')
		lash_event_set_project(event, ptr);

	comm_event->event_data.event = event;
}

void
lash_buffer_from_comm_event_config(char **buf_ptr, size_t * buf_size_ptr,
								   lash_config_t * config)
{
	size_t buf_size = sizeof(uint32_t);
	char *buf;
	size_t key_size;
	uint32_t *iptr;
	char *ptr;

	buf_size += key_size = strlen(config->key) + 1;
	if (config->value) {
		buf_size += sizeof(uint32_t);
		buf_size += config->value_size;
	}

	buf = lash_malloc(buf_size);

	iptr = (uint32_t *) buf;
	*iptr = htonl(LASH_Comm_Event_Config);
	iptr++;

	memcpy(iptr, config->key, key_size);
	ptr = (char *)iptr;
	ptr += key_size;

	if (config->value) {
		iptr = (uint32_t *) ptr;
		*iptr = htonl(config->value_size);
		iptr++;

		memcpy(iptr, config->value, config->value_size);
	}

	*buf_ptr = buf;
	*buf_size_ptr = buf_size;
}

void
lash_comm_event_from_buffer_config(char *buf, size_t buf_size,
								   lash_comm_event_t * event)
{
	uint32_t *iptr;
	char *ptr;
	lash_config_t *config;
	size_t key_size;
	size_t value_size;

	event->type = LASH_Comm_Event_Config;
	iptr = (uint32_t *) buf;
	iptr++;

	config = lash_config_new();

	ptr = (char *)iptr;
	lash_config_set_key(config, ptr);

	key_size = strlen(ptr) + 1;
	if (buf_size > key_size + sizeof(uint32_t)) {
		ptr += key_size;

		iptr = (uint32_t *) ptr;
		value_size = ntohl(*iptr);
		iptr++;

		lash_config_set_value(config, iptr, value_size);
	}

	event->event_data.config = config;
}

void
lash_buffer_from_comm_event_protocol_mismatch(char **buf_ptr,
											  size_t * buf_size_ptr,
											  lash_protocol_t protocol)
{
	size_t buf_size = sizeof(uint32_t) + sizeof(lash_protocol_t);
	char *buf;
	uint32_t *iptr;

	buf = lash_malloc(buf_size);

	iptr = (uint32_t *) buf;
	*iptr = htonl(LASH_Comm_Event_Protocol_Mismatch);
	iptr++;

	*iptr = htonl(protocol);
	/* iptr++ */

	*buf_ptr = buf;
	*buf_size_ptr = buf_size;
}

void
lash_comm_event_from_buffer_protocol_mismatch(char *buf, size_t buf_size,
											  lash_comm_event_t * event)
{
	uint32_t *iptr;

	iptr = (uint32_t *) buf;
	iptr++;

	lash_comm_event_set_protocol_mismatch(event, ntohl(*iptr));
}

void
lash_buffer_from_comm_event_exec(char **buf_ptr, size_t * buf_size_ptr,
								 lash_exec_params_t * params)
{
	uint32_t *iptr;
	char *ptr;
	char *buf;
	size_t working_dir_size;
	size_t server_size;
	size_t project_size;
	size_t argv_size;
	size_t buf_size;
	int i;

	/* see how big our buffer will be */
	buf_size = sizeof(uint32_t) * 3;
	buf_size += sizeof(char[37]);
	buf_size += working_dir_size = strlen(params->working_dir) + 1;
	buf_size += server_size = strlen(params->server) + 1;
	buf_size += project_size = strlen(params->project) + 1;

	for (i = 0; i < params->argc; i++)
		buf_size += strlen(params->argv[i]) + 1;

	/* create it */

	buf = lash_malloc(buf_size);

	iptr = (uint32_t *) buf;
	*iptr = htonl(LASH_Comm_Event_Exec);
	iptr++;

	*iptr = htonl(params->flags);
	iptr++;

	*iptr = htonl(params->argc);
	iptr++;

	ptr = (char *)iptr;
	uuid_unparse(params->id, ptr);
	ptr += 37;

	memcpy(ptr, params->working_dir, working_dir_size);
	ptr += working_dir_size;

	memcpy(ptr, params->server, server_size);
	ptr += server_size;

	memcpy(ptr, params->project, project_size);
	ptr += project_size;

	for (i = 0; i < params->argc; i++) {
		argv_size = strlen(params->argv[i]) + 1;
		memcpy(ptr, params->argv[i], argv_size);
		ptr += argv_size;
	}

	/* done */

	*buf_ptr = buf;
	*buf_size_ptr = buf_size;
}

void
lash_comm_event_from_buffer_exec(char *buf, size_t buf_size,
								 lash_comm_event_t * event)
{
	uint32_t *iptr;
	char *ptr;
	lash_exec_params_t *params;
	int i;

	iptr = (uint32_t *) buf;
	event->type = LASH_Comm_Event_Exec;
	iptr++;

	params = lash_exec_params_new();

	params->flags = ntohl(*iptr);
	iptr++;

	params->argc = ntohl(*iptr);
	iptr++;

	ptr = (char *)iptr;
	uuid_parse(ptr, params->id);
	ptr += 37;

	lash_exec_params_set_working_dir(params, ptr);
	ptr += strlen(ptr) + 1;

	lash_exec_params_set_server(params, ptr);
	ptr += strlen(ptr) + 1;

	lash_exec_params_set_project(params, ptr);
	ptr += strlen(ptr) + 1;

	params->argv = lash_malloc(sizeof(char *) * params->argc);
	for (i = 0; i < params->argc; i++) {
		params->argv[i] = lash_strdup(ptr);
		ptr += strlen(ptr) + 1;
	}

#ifdef LASH_DEBUG
	LASH_DEBUGARGS
		("created exec comm event with: flags=%d, working_dir=%s, server=%s, project=%s, argc=%d, args:",
		 params->flags, params->working_dir, params->server, params->project,
		 params->argc);
	for (i = 0; i < params->argc; i++) {
		LASH_DEBUGARGS("  argv[%d]: '%s'", i, params->argv[i]);
	}
#endif

	lash_comm_event_set_exec(event, params);
}

/* for ping, pong and close */
void
lash_buffer_from_comm_event(char **buf_ptr, size_t * buf_size_ptr,
							lash_comm_event_t * event)
{
	size_t buf_size = sizeof(uint32_t);
	char *buf;
	uint32_t *iptr;

	buf = lash_malloc(buf_size);

	iptr = (uint32_t *) buf;
	*iptr = htonl(event->type);
	iptr++;

	*buf_ptr = buf;
	*buf_size_ptr = buf_size;
}

void
lash_comm_event_from_buffer(char *buf, size_t buf_size,
							lash_comm_event_t * event)
{
	event->type = ntohl(*((uint32_t *) buf));
}

/* EOF */
