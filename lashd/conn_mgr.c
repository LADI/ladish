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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <netdb.h>

#include <lash/lash.h>
#include <lash/internal_headers.h>

#include "conn_mgr.h"
#include "conn.h"
#include "server.h"

void *conn_mgr_recv_run(void *data);
void *conn_mgr_send_run(void *data);

void
conn_mgr_free(conn_mgr_t * conn_mgr)
{
	conn_t *conn;
	lash_list_t *list;

	close(conn_mgr->listen_socket);

	for (list = conn_mgr->open_connections; list; list = lash_list_next(list)) {
		conn = (conn_t *) list->data;

		LASH_DEBUGARGS("closing open connection %ld ('%s')", conn->id,
					   conn_get_str_id(conn));
		conn_destroy(conn);
	}

	lash_list_free(conn_mgr->open_connections);
	conn_mgr->open_connections = NULL;
	for (list = conn_mgr->connections; list; list = list->next) {
		conn = (conn_t *) list->data;

		LASH_DEBUGARGS("closing connection %ld ('%s')", conn->id,
					   conn_get_str_id(conn));
		conn_destroy(conn);
	}
	lash_list_free(conn_mgr->connections);
	conn_mgr->connections = NULL;

	conn_mgr->listen_socket = 0;
	FD_ZERO(&conn_mgr->sockets);
	conn_mgr->fd_max = 0;
	conn_mgr->client_events = NULL;

	pthread_mutex_destroy(&conn_mgr->connections_lock);
	pthread_mutex_destroy(&conn_mgr->client_event_lock);
	pthread_cond_destroy(&conn_mgr->client_event_cond);
}

conn_mgr_t *
conn_mgr_new(server_t * server)
{
	conn_mgr_t *mgr;
	int err;

	mgr = lash_malloc0(sizeof(conn_mgr_t));
	pthread_mutex_init(&mgr->connections_lock, NULL);
	pthread_mutex_init(&mgr->client_event_lock, NULL);
	pthread_cond_init(&mgr->client_event_cond, NULL);
	FD_ZERO(&mgr->sockets);

	mgr->server = server;

	err = conn_mgr_start(mgr);
	if (err) {
		conn_mgr_destroy(mgr);
		return NULL;
	}

	return mgr;
}

void
conn_mgr_destroy(conn_mgr_t * conn_mgr)
{
	LASH_PRINT_DEBUG("stopping");
	conn_mgr_stop(conn_mgr);
	LASH_PRINT_DEBUG("stopped");

	conn_mgr_free(conn_mgr);
	free(conn_mgr);
}

static int
conn_mgr_set_socket_opts(int sock)
{
	int err;
	int reuse;

	/*
	 * reuse ports.  this is so we can bind again quickly after shutting down.
	 */
	reuse = 1;
	err = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	if (err == -1) {
		fprintf(stderr, "%s: could not set SO_REUSEADDR on socket: %s\n",
				__FUNCTION__, strerror(errno));
	}

	/*
	 * make sure socket is closed on exec()
	 */
	err = fcntl(sock, F_SETFD, FD_CLOEXEC);
	if (err == -1) {
		fprintf(stderr,
				"%s: could not set close-on-exec on listen socket: %s\n",
				__FUNCTION__, strerror(errno));
		return -1;
	}

	return 0;
}

int
conn_mgr_bind_socket(int *sock, struct addrinfo *addr)
{
	int err;

	*sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (*sock == -1) {
		LASH_DEBUGARGS
			("could not create socket with params domain=%d, type=%d, protocol=%d: %s",
			 addr->ai_family, addr->ai_socktype, addr->ai_protocol,
			 strerror(errno));
		return -1;
	}

	conn_mgr_set_socket_opts(*sock);

	err = bind(*sock, addr->ai_addr, addr->ai_addrlen);
	if (err) {
		LASH_DEBUGARGS("could not bind socket: %s", strerror(errno));

		err = close(*sock);
		if (err) {
			fprintf(stderr, "%s: error closing unconnected socket: %s",
					__FUNCTION__, strerror(errno));
		}

		return -1;
	}

	return 0;
}

int
conn_mgr_start(conn_mgr_t * conn_mgr)
{
	struct addrinfo hints;
	struct addrinfo *addrs;
	struct addrinfo *addr;
	int bound = 0;
	int err;

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	err = getaddrinfo(NULL, "lash", &hints, &addrs);
	if (err) {
		fprintf(stderr, "%s: could not look up service name: %s\n",
				__FUNCTION__, gai_strerror(err));
		return -1;
	}

	/* try ipv6 first */
	if (!no_v6)
		for (addr = addrs; addr; addr = addr->ai_next) {
			if (addr->ai_family == AF_INET6) {
				err = conn_mgr_bind_socket(&conn_mgr->listen_socket, addr);
				if (err)
					continue;
				else {
					bound = 1;
					break;
				}
			}
		}

	if (!bound)
		for (addr = addrs; addr; addr = addr->ai_next) {
			if (no_v6 && addr->ai_family == AF_INET6)
				continue;

			err = conn_mgr_bind_socket(&conn_mgr->listen_socket, addr);
			if (err)
				continue;
			else {
				bound = 1;
				break;
			}
		}

	freeaddrinfo(addrs);

	if (!bound) {
		fprintf(stderr, "%s: could not create listen socket\n", __FUNCTION__);
		return -1;
	}

	/* start the listening */
	err = listen(conn_mgr->listen_socket, 20);
	if (err == -1) {
		fprintf(stderr, "%s: error setting socket to listen: %s\n",
				__FUNCTION__, strerror(errno));
		return -1;
	}
	printf("Listening for connections\n");

	FD_SET(conn_mgr->listen_socket, &conn_mgr->sockets);
	conn_mgr->fd_max = conn_mgr->listen_socket;

	/* start up the threads */
	LASH_PRINT_DEBUG("starting recv thread");
	err =
		pthread_create(&conn_mgr->recv_thread, NULL, conn_mgr_recv_run,
					   conn_mgr);
	if (err) {
		fprintf(stderr, "%s: could not start recv thread\n", __FUNCTION__);
		abort();
	}
	LASH_PRINT_DEBUG("recv thread started; starting send thread");
	pthread_create(&conn_mgr->send_thread, NULL, conn_mgr_send_run, conn_mgr);
	LASH_PRINT_DEBUG("send thread started");

	return 0;
}

void
conn_mgr_stop(conn_mgr_t * conn_mgr)
{
	conn_mgr->quit = 1;
	pthread_cond_signal(&conn_mgr->client_event_cond);
	pthread_join(conn_mgr->send_thread, NULL);
	pthread_join(conn_mgr->recv_thread, NULL);
}

void
conn_mgr_send_client_event(conn_mgr_t * conn_mgr, server_event_t * event)
{
	pthread_mutex_lock(&conn_mgr->client_event_lock);
	conn_mgr->client_events =
		lash_list_append(conn_mgr->client_events, event);
	pthread_mutex_unlock(&conn_mgr->client_event_lock);
	pthread_cond_signal(&conn_mgr->client_event_cond);
}

void
conn_mgr_send_client_lash_event(conn_mgr_t * conn_mgr, unsigned long conn_id,
								lash_event_t * lash_event)
{
	server_event_t *server_event;

	if (!lash_event)
		return;

	server_event = server_event_new();
	server_event_set_conn_id(server_event, conn_id);
	server_event_set_lash_event(server_event, lash_event);

	conn_mgr_send_client_event(conn_mgr, server_event);
}

void
conn_mgr_send_client_lash_config(conn_mgr_t * conn_mgr, unsigned int conn_id,
								 lash_config_t * lash_config)
{
	server_event_t *server_event;

	if (!lash_config)
		return;

	server_event = server_event_new();
	server_event_set_conn_id(server_event, conn_id);
	server_event_set_lash_config(server_event, lash_config);

	conn_mgr_send_client_event(conn_mgr, server_event);
}

void
conn_mgr_send_client_lash_comm_event(conn_mgr_t * conn_mgr,
									 unsigned int conn_id,
									 lash_comm_event_t * event)
{
	server_event_t *server_event;

	if (!event)
		return;

	server_event = server_event_new();
	server_event_set_conn_id(server_event, conn_id);
	server_event_set_lash_comm_event(server_event, event);

	conn_mgr_send_client_event(conn_mgr, server_event);
}

void
conn_mgr_send_server_lash_event(conn_mgr_t * conn_mgr, unsigned long conn_id,
								lash_event_t * lash_event)
{
	server_event_t *server_event;

	if (!lash_event)
		return;

	server_event = server_event_new();
	server_event_set_conn_id(server_event, conn_id);
	server_event_set_lash_event(server_event, lash_event);

	server_send_event(conn_mgr->server, server_event);
}

void
conn_mgr_send_server_lash_config(conn_mgr_t * conn_mgr, unsigned int conn_id,
								 lash_config_t * lash_config)
{
	server_event_t *server_event;

	if (!lash_config)
		return;

	server_event = server_event_new();
	server_event_set_conn_id(server_event, conn_id);
	server_event_set_lash_config(server_event, lash_config);

	server_send_event(conn_mgr->server, server_event);
}

void
conn_mgr_send_server_lash_comm_event(conn_mgr_t * conn_mgr,
									 unsigned int conn_id,
									 lash_comm_event_t * event)
{
	server_event_t *server_event;

	if (!event)
		return;

	server_event = server_event_new();
	server_event_set_conn_id(server_event, conn_id);
	server_event_set_lash_comm_event(server_event, event);

	server_send_event(conn_mgr->server, server_event);
}

/***********************************
 *********** recv thread ***********
 ***********************************/

void
conn_mgr_accept_connection(conn_mgr_t * conn_mgr)
{
	conn_t *conn;

	//struct sockaddr_storage ss;
	//size_t ss_len;
	int err;

	conn = conn_new();

	/* accept the connection */
	conn->socket = accept(conn_mgr->listen_socket, NULL, NULL);
	/*(struct sockaddr *) &ss,
	 * &ss_len); */

	if (conn->socket == -1) {
		fprintf(stderr,
				"%s: error accepting socket connection from '%s': %s\n",
				__FUNCTION__, conn_get_str_id(conn), strerror(errno));
		conn_destroy(conn);
		return;
	}
	LASH_DEBUGARGS("accepted connection from '%s' with id %ld",
				   conn_get_str_id(conn), conn->id);

	/* 
	 * make sure socket is closed on exec()
	 */
	err = fcntl(conn_mgr->listen_socket, F_SETFD, FD_CLOEXEC);
	if (err == -1) {
		fprintf(stderr,
				"%s: could not set close-on-exec on connection %ld ('%s'): %s\n",
				__FUNCTION__, conn->id, conn_get_str_id(conn),
				strerror(errno));
		exit(1);
	}

	/* socket stuff */
	FD_SET(conn->socket, &conn_mgr->sockets);
	if (conn->socket > conn_mgr->fd_max)
		conn_mgr->fd_max = conn->socket;

	/* add the connection to the open connection list */
	conn_mgr->open_connections =
		lash_list_append(conn_mgr->open_connections, conn);
}

void
conn_mgr_connect_client(conn_mgr_t * conn_mgr, conn_t * conn)
{
	lash_comm_event_t *event;
	server_event_t *server_event;
	int err;
	lash_connect_params_t *params;
	char id[37];

	/* read the Connect event */
	event = lash_comm_event_new();

	LASH_PRINT_DEBUG("recieving Connect");
	err = lash_comm_recv_event(conn->socket, event);
	if (err == -1 ||
		err == -2 ||
		err == -3 ||
		lash_comm_event_get_type(event) != LASH_Comm_Event_Connect ||
		!LASH_PROTOCOL_IS_VALID(event->event_data.connect->protocol_version))
	{
		/* there was an error */

		if (err == -1)
			fprintf(stderr,
					"%s: there was a recieve error from connection '%s': disconnecting client\n",
					__FUNCTION__, conn_get_str_id(conn));

		else if (err == -2)
			fprintf(stderr,
					"%s: connection '%s' closed before sending Connect event\n",
					__FUNCTION__, conn_get_str_id(conn));

		else if (err == -3)
			fprintf(stderr,
					"%s: connection '%s' is using the wrong low-level protocol version\n",
					__FUNCTION__, conn_get_str_id(conn));

		else if (lash_comm_event_get_type(event) != LASH_Comm_Event_Connect)
			fprintf(stderr,
					"%s: connection '%s' sent an event (of type %d) that wasn't Connect before it was connected; removing\n",
					__FUNCTION__, conn_get_str_id(conn),
					lash_comm_event_get_type(event));

		else if (!LASH_PROTOCOL_IS_VALID
				 (event->event_data.connect->protocol_version)) {
			lash_comm_event_t mismatch_event;

			fprintf(stderr,
					"%s: connection '%s' is using protocol %s; disconnecting\n",
					__FUNCTION__, conn_get_str_id(conn),
					lash_protocol_string(event->event_data.connect->
										 protocol_version));

			lash_comm_event_set_protocol_mismatch(&mismatch_event,
												  LASH_PROTOCOL_VERSION);
			lash_comm_send_event(conn->socket, &mismatch_event);
		}

		lash_comm_event_destroy(event);
		FD_CLR(conn->socket, &conn_mgr->sockets);
		conn_mgr->open_connections =
			lash_list_remove(conn_mgr->open_connections, conn);
		conn_destroy(conn);
		return;
	}

	params = event->event_data.connect;
	uuid_unparse(params->id, id);

	LASH_PRINT_DEBUG("connecting new client with:");
	LASH_DEBUGARGS("  connection id:  %ld", conn->id);
	LASH_DEBUGARGS("  source address: %s", conn_get_str_id(conn));
	LASH_DEBUGARGS("  protocol:       %s",
				   lash_protocol_string(event->event_data.connect->
										protocol_version));
	LASH_DEBUGARGS("  class:          '%s'", params->class);
	LASH_DEBUGARGS("  id:             '%s'", id);
	LASH_DEBUGARGS("  working dir:    '%s'", params->working_dir);
	LASH_DEBUGARGS("  requested proj: '%s'", params->project);
	LASH_DEBUGARGS("  flags:          %d", params->flags);
	LASH_DEBUGARGS("  %d args:", params->argc);
	for (err = 0; err < params->argc; err++) {
		LASH_DEBUGARGS("   arg %d: '%s'", err, params->argv[err]);
	}

	/* tell the server there's a new client connection */
	server_event = server_event_new();
	server_event_set_conn_id(server_event, conn->id);
	server_event_set_lash_connect_params(server_event,
										 event->event_data.connect);

	server_send_event(conn_mgr->server, server_event);

	event->event_data.connect = NULL;
	lash_comm_event_destroy(event);

	/* remove the connection from the open list */
	conn_mgr->open_connections =
		lash_list_remove(conn_mgr->open_connections, conn);

	/* add the connection to the connection list */
	pthread_mutex_lock(&conn_mgr->connections_lock);
	conn_mgr->connections = lash_list_append(conn_mgr->connections, conn);
	pthread_mutex_unlock(&conn_mgr->connections_lock);

	conn_set_recv_stamp(conn);
}

void
conn_mgr_disconnect_client(conn_mgr_t * conn_mgr, conn_t * conn,
						   int notify_server)
{
	unsigned long conn_id;

	/* clear up what we know of the connection */
	conn_mgr->connections = lash_list_remove(conn_mgr->connections, conn);

	conn_id = conn->id;
	FD_CLR(conn->socket, &conn_mgr->sockets);

	conn_unlock(conn);
	conn_destroy(conn);

	if (notify_server) {
		server_event_t *server_event;

		server_event = server_event_new();
		server_event_set_type(server_event, Client_Disconnect);
		server_event_set_conn_id(server_event, conn_id);

		server_send_event(conn_mgr->server, server_event);
	}

	LASH_DEBUGARGS("disconnected client on connection %ld", conn_id);
}

void
conn_mgr_deal_with_event(conn_mgr_t * conn_mgr, conn_t * conn,
						 lash_comm_event_t * event)
{
	switch (lash_comm_event_get_type(event)) {
	case LASH_Comm_Event_Event:
		conn_mgr_send_server_lash_event(conn_mgr, conn->id,
										lash_comm_event_take_event(event));
		lash_comm_event_destroy(event);
		break;
	case LASH_Comm_Event_Config:
		conn_mgr_send_server_lash_config(conn_mgr, conn->id,
										 lash_comm_event_take_config(event));
		lash_comm_event_destroy(event);
		break;
	case LASH_Comm_Event_Ping:
		lash_comm_event_set_type(event, LASH_Comm_Event_Pong);
		conn_mgr_send_client_lash_comm_event(conn_mgr, conn->id, event);
		break;
	case LASH_Comm_Event_Pong:
		lash_comm_event_destroy(event);
		break;
	default:
		break;
	}
}

void
conn_mgr_recv_event(conn_mgr_t * conn_mgr, conn_t * conn)
{
	int err;
	lash_comm_event_t *event;

	/* recieve the event */
	event = lash_comm_event_new();
	err = lash_comm_recv_event(conn->socket, event);

	if (err == -1) {
		fprintf(stderr, "%s: error recieving event from connection '%s'\n",
				__FUNCTION__, conn_get_str_id(conn));
		return;
	}

	/* connection disconnect */
	if (err == -2 || lash_comm_event_get_type(event) == LASH_Comm_Event_Close) {
		LASH_DEBUGARGS("%s: connection '%s' disconnected",
					   conn_get_str_id(conn));

		if (lash_comm_event_get_type(event) == LASH_Comm_Event_Close)
			lash_comm_event_destroy(event);

		conn_mgr_disconnect_client(conn_mgr, conn, 1);

		/* done what we might have done */
		pthread_mutex_unlock(&conn_mgr->connections_lock);
		return;
	}

	/* give up the lock here, as we're definately not messing with it now */
	pthread_mutex_unlock(&conn_mgr->connections_lock);

	/* this function will destroy the event */
	conn_mgr_deal_with_event(conn_mgr, conn, event);

	/* reset ping */
	conn_set_recv_stamp(conn);

	/* unlock the connection */
	conn_unlock(conn);
}

void
conn_mgr_read_socket(conn_mgr_t * conn_mgr, int socket)
{
	lash_list_t *list;
	conn_t *conn;

	/* check if it's a new connection */
	if (socket == conn_mgr->listen_socket) {
		conn_mgr_accept_connection(conn_mgr);
		return;
	}

	/* see if it's an open connection */
	list = conn_mgr->open_connections;
	while (list) {
		conn = (conn_t *) list->data;
		if (conn->socket == socket) {
			conn_mgr_connect_client(conn_mgr, conn);
			return;
		}

		list = list->next;
	}

	/* else find it in the existing connections */
	pthread_mutex_lock(&conn_mgr->connections_lock);
	list = conn_mgr->connections;
	while (list) {
		conn = (conn_t *) list->data;
		if (conn->socket == socket) {
			conn_lock(conn);

			/* must unlock connection! */
			conn_mgr_recv_event(conn_mgr, conn);
			return;
		}

		list = list->next;
	}

	fprintf(stderr,
			"%s: could not find a connection with socket %d!  this should, like, never happen.  you shouldn't be reading this.  if you are, bad things have happened.  your computer will likely melt after about 20 minutes.",
			__FUNCTION__, socket);
	abort();
}

void
conn_mgr_check_timeouts(conn_mgr_t * conn_mgr)
{
	lash_list_t *list, *next;
	conn_t *conn;
	time_t now;

	now = time(NULL);
	if (now == (time_t) - 1) {
		fprintf(stderr, "%s: could not get time, aborting!: %s\n",
				__FUNCTION__, strerror(errno));
		abort();
	}

	pthread_mutex_lock(&conn_mgr->connections_lock);
	list = conn_mgr->connections;
	while (list) {
		conn = (conn_t *) list->data;
		next = list->next;

		conn_lock(conn);

		if (conn_get_pinged(conn)) {
			if (conn_ping_timed_out(conn, now)) {
				fprintf(stderr,
						"%s: connection '%ld' has not responded to ping for %ld seconds, disconnecting it\n",
						__FUNCTION__, conn->id, CONN_TIMEOUT);
				conn_mgr_disconnect_client(conn_mgr, conn, 1);
			} else
				conn_unlock(conn);
		} else {
			if (conn_recv_timed_out(conn, now)) {
				/* send a ping */
				lash_comm_event_t *event;

/*              LASH_DEBUGARGS ("pinging connection '%ld'", conn_get_id (conn)); */

				event = lash_comm_event_new();
				lash_comm_event_set_type(event, LASH_Comm_Event_Ping);
				conn_mgr_send_client_lash_comm_event(conn_mgr, conn->id,
													 event);
				conn_set_ping_stamp(conn);
			}

			conn_unlock(conn);
		}

		list = next;
	}
	pthread_mutex_unlock(&conn_mgr->connections_lock);
}

void *
conn_mgr_recv_run(void *data)
{
	conn_mgr_t *conn_mgr;
	fd_set sockets;
	int err, i;
	struct timeval select_timeout;

	conn_mgr = (conn_mgr_t *) data;

	while (!conn_mgr->quit) {
		sockets = conn_mgr->sockets;
		select_timeout.tv_sec = 1;
		select_timeout.tv_usec = 0;

		err =
			select(conn_mgr->fd_max + 1, &sockets, NULL, NULL,
				   &select_timeout);
		if (err == -1) {
			if (errno == EINTR)
				continue;

			fprintf(stderr, "%s: error calling select(): %s\n", __FUNCTION__,
					strerror(errno));
			return NULL;
		}

		if (conn_mgr->quit)
			break;

		for (i = 0; i <= conn_mgr->fd_max; i++) {
			if (FD_ISSET(i, &sockets)) {
				conn_mgr_read_socket(conn_mgr, i);
			}
		}

		conn_mgr_check_timeouts(conn_mgr);
	}

	LASH_PRINT_DEBUG("finished");
	return NULL;
}

/****************************************
 ************ send thread ***************
 ****************************************/

void
conn_mgr_send_lash_comm_event_to_client(conn_t * conn,
										lash_comm_event_t * lash_comm_event)
{
	int err;

	err = lash_comm_send_event(conn->socket, lash_comm_event);

	if (err == -1) {
		fprintf(stderr, "%s: could not send event to client\n", __FUNCTION__);
	}

	lash_comm_event_destroy(lash_comm_event);
}

void
conn_mgr_send_disconnect_client(conn_mgr_t * conn_mgr, unsigned long conn_id)
{
	lash_list_t *list;
	conn_t *conn;

	pthread_mutex_lock(&conn_mgr->connections_lock);
	for (list = conn_mgr->connections; list; list = list->next) {
		conn = (conn_t *) list->data;

		if (conn->id == conn_id) {
			/* remove it */
			conn_lock(conn);
			conn_mgr_disconnect_client(conn_mgr, conn, 0);
			pthread_mutex_unlock(&conn_mgr->connections_lock);
			return;
		}
	}

	pthread_mutex_unlock(&conn_mgr->connections_lock);
	fprintf(stderr,
			"%s: request from server to remove unknown connection %ld\n",
			__FUNCTION__, conn_id);
}

void
conn_mgr_send_server_event_to_client(conn_mgr_t * conn_mgr,
									 server_event_t * server_event)
{
	lash_comm_event_t *lash_comm_event = NULL;
	unsigned long conn_id;
	lash_list_t *list;
	conn_t *conn;

	conn_id = server_event->conn_id;

	/* extract the comm event */
	switch (server_event->type) {
	case Client_Event:
		lash_comm_event = lash_comm_event_new();
		lash_comm_event_set_event(lash_comm_event,
								  server_event_take_lash_event(server_event));
		break;

	case Client_Config:
		lash_comm_event = lash_comm_event_new();
		lash_comm_event_set_config(lash_comm_event,
								   server_event_take_lash_config
								   (server_event));
		break;

	case Client_Comm_Event:
		lash_comm_event = server_event_take_lash_comm_event(server_event);
		break;

	case Client_Disconnect:
		conn_mgr_send_disconnect_client(conn_mgr, server_event->conn_id);
		server_event_destroy(server_event);
		return;

	case Client_Connect:
	default:
		fprintf(stderr,
				"%s: recieved unknown send request of type %d from server\n",
				__FUNCTION__, server_event->type);
		server_event_destroy(server_event);
		return;
	}
	server_event_destroy(server_event);

	/* attempt to find the connection */
	pthread_mutex_lock(&conn_mgr->connections_lock);
	list = conn_mgr->connections;
	while (list) {
		conn = (conn_t *) list->data;

		if (conn->id == conn_id) {
			/* send it */
			conn_lock(conn);
			pthread_mutex_unlock(&conn_mgr->connections_lock);
			conn_mgr_send_lash_comm_event_to_client(conn, lash_comm_event);
			conn_unlock(conn);
			return;
		}

		list = list->next;
	}

	pthread_mutex_unlock(&conn_mgr->connections_lock);

	fprintf(stderr, "%s: could not send event to unknown connection id %ld\n",
			__FUNCTION__, conn_id);
}

void *
conn_mgr_send_run(void *data)
{
	conn_mgr_t *conn_mgr;
	lash_list_t *list;
	server_event_t *server_event;

	LASH_PRINT_DEBUG("send thread starting");

	conn_mgr = (conn_mgr_t *) data;

	while (!conn_mgr->quit) {
		pthread_mutex_lock(&conn_mgr->client_event_lock);
		list = conn_mgr->client_events;
		if (list)
			conn_mgr->client_events = NULL;
		else {
			pthread_cond_wait(&conn_mgr->client_event_cond,
							  &conn_mgr->client_event_lock);
			list = conn_mgr->client_events;
			conn_mgr->client_events = NULL;
		}

		pthread_mutex_unlock(&conn_mgr->client_event_lock);

		if (conn_mgr->quit) {
			break;
		}

		while (list) {
			server_event = (server_event_t *) list->data;

			conn_mgr_send_server_event_to_client(conn_mgr, server_event);

			list = list->next;
		}
	}

	LASH_PRINT_DEBUG("finished");
	return NULL;
}

/* EOF */
