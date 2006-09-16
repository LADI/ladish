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

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
extern int h_errno;

#include <lash/lash.h>
#include <lash/internal_headers.h>

int
lash_comm_send_event(int socket, lash_comm_event_t * event)
{
	int err;
	size_t buf_size;
	char *buf;

	switch (event->type) {
	case LASH_Comm_Event_Connect:
		lash_buffer_from_comm_event_connect(&buf, &buf_size,
											event->event_data.connect);
		break;
	case LASH_Comm_Event_Event:
		lash_buffer_from_comm_event_event(&buf, &buf_size,
										  event->event_data.event);
		break;
	case LASH_Comm_Event_Config:
		lash_buffer_from_comm_event_config(&buf, &buf_size,
										   event->event_data.config);
		break;
	case LASH_Comm_Event_Protocol_Mismatch:
		lash_buffer_from_comm_event_protocol_mismatch(&buf, &buf_size,
													  event->event_data.
													  number);
		break;
	case LASH_Comm_Event_Exec:
		lash_buffer_from_comm_event_exec(&buf, &buf_size,
										 event->event_data.exec);
		break;
	case LASH_Comm_Event_Close:
	case LASH_Comm_Event_Ping:
	case LASH_Comm_Event_Pong:
		lash_buffer_from_comm_event(&buf, &buf_size, event);
		break;
	default:
		break;
	}

	/* send the buffer */
	err = lash_sendall(socket, buf, buf_size, 0);
	if (err == -1) {
		fprintf(stderr, "%s: error sending client event\n", __FUNCTION__);
	}

	free(buf);

	return err;
}

int
lash_comm_recv_event(int socket, lash_comm_event_t * event)
{
	char *buf;
	size_t buf_size;
	int err;
	uint32_t *iptr;

	err = lash_recvall(socket, (void **)&buf, &buf_size, 0);
	if (err < 0)
		return err;

	iptr = (uint32_t *) buf;
	event->type = ntohl(*iptr);

	switch (event->type) {
	case LASH_Comm_Event_Connect:
		err = lash_comm_event_from_buffer_connect(buf, buf_size, event);
		if (err)
			return -3;
		break;
	case LASH_Comm_Event_Event:
		lash_comm_event_from_buffer_event(buf, buf_size, event);
		break;
	case LASH_Comm_Event_Config:
		lash_comm_event_from_buffer_config(buf, buf_size, event);
		break;
	case LASH_Comm_Event_Protocol_Mismatch:
		lash_comm_event_from_buffer_protocol_mismatch(buf, buf_size, event);
		break;
	case LASH_Comm_Event_Exec:
		lash_comm_event_from_buffer_exec(buf, buf_size, event);
		break;
	case LASH_Comm_Event_Close:
	case LASH_Comm_Event_Ping:
	case LASH_Comm_Event_Pong:
		lash_comm_event_from_buffer(buf, buf_size, event);
		break;
	default:
		break;
	}

	free(buf);

	return buf_size;
}

int
lash_comm_connect_to_server(lash_client_t * client, const char *server,
							const char *service,
							lash_connect_params_t * connect)
{
	lash_comm_event_t connect_event;
	int err;

	err = lash_open_socket(&client->socket, server, service);
	if (err) {
		fprintf(stderr, "%s: could not create server connection\n",
				__FUNCTION__);
		return 1;
	}

	connect_event.type = LASH_Comm_Event_Connect;
	connect_event.event_data.connect = connect;

	LASH_PRINT_DEBUG("sending Connect event");
	err = lash_comm_send_event(client->socket, &connect_event);
	if (err == -1) {
		fprintf(stderr, "%s: error sending connect event to the server\n",
				__FUNCTION__);
		close(client->socket);
		return 1;
	}
	LASH_PRINT_DEBUG("Connect event sent");

	return 0;
}

static void
lash_comm_recv_finish(lash_client_t * client)
{
	lash_list_t *node;

	client->recv_close = 1;
	client->send_close = 1;
	pthread_cond_signal(&client->send_conditional);
	pthread_join(client->send_thread, NULL);

	close(client->socket);
	/*lash_args_destroy(client->args);*/
	client->args = NULL;
	free(client->class);
	client->class = NULL;

	pthread_mutex_destroy(&client->comm_events_out_lock);
	pthread_cond_destroy(&client->send_conditional);

	for (node = client->comm_events_out; node; node = lash_list_next(node))
		lash_comm_event_destroy((lash_comm_event_t *) node->data);
	lash_list_free(client->comm_events_out);
}

static void
lash_comm_recv_lost_server(lash_client_t * client)
{
	lash_event_t *ev;

	ev = lash_event_new_with_type(LASH_Server_Lost);
	pthread_mutex_lock(&client->events_in_lock);
	client->events_in = lash_list_append(client->events_in, ev);
	pthread_mutex_unlock(&client->events_in_lock);

	client->server_connected = 0;
	lash_comm_recv_finish(client);
}

void *
lash_comm_recv_run(void *data)
{
	lash_client_t *client;
	lash_comm_event_t comm_event;
	lash_event_t *event;
	lash_config_t *config;
	int err;

	client = (lash_client_t *) data;

	while (!client->recv_close) {
		err = lash_comm_recv_event(client->socket, &comm_event);

		if (err == -1) {
			fprintf(stderr, "%s: error recieving event\n", __FUNCTION__);
			continue;
		}

		if (err == -2) {
			LASH_PRINT_DEBUG("server disconnected");
			lash_comm_recv_lost_server(client);
		}

		switch (comm_event.type) {
		case LASH_Comm_Event_Event:
			event = comm_event.event_data.event;

			/* add the event to the event buffer */
			pthread_mutex_lock(&client->events_in_lock);
			client->events_in = lash_list_append(client->events_in, event);
			pthread_mutex_unlock(&client->events_in_lock);

			break;

		case LASH_Comm_Event_Config:
			config = comm_event.event_data.config;

			LASH_DEBUGARGS("recieved config with key '%s'",
						   lash_config_get_key(config));

			/* add to the configs */
			pthread_mutex_lock(&client->configs_in_lock);
			client->configs_in = lash_list_append(client->configs_in, config);
			pthread_mutex_unlock(&client->configs_in_lock);

			break;

		case LASH_Comm_Event_Ping:{
			lash_comm_event_t *ev;

			ev = lash_comm_event_new();
			lash_comm_event_set_type(ev, LASH_Comm_Event_Pong);

			pthread_mutex_lock(&client->comm_events_out_lock);
			client->comm_events_out =
				lash_list_append(client->comm_events_out, ev);
			pthread_mutex_unlock(&client->comm_events_out_lock);
			pthread_cond_signal(&client->send_conditional);
			break;
		}

		case LASH_Comm_Event_Protocol_Mismatch:
			fprintf(stderr,
					"%s: protocol version mismatch!; server is using protocol version %s\n",
					__FUNCTION__,
					lash_protocol_string(comm_event.event_data.number));
			lash_comm_recv_lost_server(client);
			break;

		case LASH_Comm_Event_Close:
			lash_comm_recv_finish(client);
			break;

		default:
			fprintf(stderr, "%s: recieved unknown event of type %d\n",
					__FUNCTION__, comm_event.type);
			break;
		}
	}

	return NULL;
}

void *
lash_comm_send_run(void *data)
{
	lash_client_t *client;
	lash_list_t *list;
	lash_comm_event_t *comm_event;
	int err;

	client = (lash_client_t *) data;

	while (!client->send_close) {
		pthread_mutex_lock(&client->comm_events_out_lock);
		list = client->comm_events_out;
		if (list) {
			client->comm_events_out = NULL;
		} else {
			pthread_cond_wait(&client->send_conditional,
							  &client->comm_events_out_lock);
			list = client->comm_events_out;
			client->comm_events_out = NULL;
		}
		pthread_mutex_unlock(&client->comm_events_out_lock);

		if (client->send_close)
			break;

		while (list) {
			comm_event = (lash_comm_event_t *) list->data;

			err = lash_comm_send_event(client->socket, comm_event);
			if (err == -1) {
				fprintf(stderr, "%s: error sending client comm event\n",
						__FUNCTION__);
			}

			list = lash_list_remove(list, comm_event);

			lash_comm_event_free(comm_event);
			free(comm_event);
		}

	}

	return NULL;
}

/* EOF */
