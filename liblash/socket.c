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

#define _POSIX_SOURCE /* addrinfo */

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
extern int h_errno;

#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>

#include <lash/xmalloc.h>
#include <lash/debug.h>

int
lash_sendall(int socket, const void *buf, size_t buf_size, int flags)
{
	int err;
	char *new_buf;
	size_t sent, new_buf_size;
	uint32_t *iptr;

	/* create the new buffer */
	new_buf_size = buf_size + sizeof(uint32_t);
	new_buf = lash_malloc(new_buf_size);

	/* create the packet header */
	iptr = (uint32_t *) new_buf;
	*iptr = htonl(new_buf_size);

	/* check to see if the size has been truncated
	 * (should never happen - we're not shifting much data around) */
	if (ntohl(*iptr) != new_buf_size) {
		fprintf(stderr, "%s: buf_size was truncated by htonl()!\n",
				__FUNCTION__);
		free(new_buf);
		return -1;
	}

	/* fill the packet */
	iptr++;
	memcpy(iptr, buf, buf_size);

	sent = 0;
	while (sent < new_buf_size) {
		err = send(socket, new_buf + sent, new_buf_size - sent, flags);

		if (err == 0) {			/* connection was closed */
			free(new_buf);
			return (-2);
		}

		if (err == -1) {
			if (errno == EINTR)
				continue;

			fprintf(stderr, "%s: error sending data: %s\n", __FUNCTION__,
					strerror(errno));
			free(new_buf);
			return (-1);
		}

		sent += err;
	}

	free(new_buf);
	return sent - sizeof(uint32_t);
}

int
lash_recvall(int socket, void **buf_ptr, size_t * buf_size_ptr, int flags)
{
	int err;
	char *buf = NULL;
	size_t recvd, buf_size;
	uint32_t *iptr;
	size_t packet_size;

	buf_size = sizeof(uint32_t);
	buf = lash_malloc(buf_size);

	recvd = 0;
	while (recvd < sizeof(uint32_t)) {
		err = recv(socket, buf + recvd, sizeof(uint32_t) - recvd, flags);

		/* check if the socket was closed */
		if (err == 0 && recvd == 0) {
			free(buf);
			return -2;
		}

		if (err == -1) {
			if (errno == EINTR)
				continue;

			fprintf(stderr, "%s: error recieving data: %s\n", __FUNCTION__,
					strerror(errno));
			free(buf);
			return (-1);
		}

		recvd += err;
	}

	iptr = (uint32_t *) buf;
	packet_size = ntohl(*iptr) - sizeof(uint32_t);
	if (buf_size != packet_size) {
		buf_size = packet_size;
		buf = lash_realloc(buf, buf_size);
	}

	recvd = 0;
	while (recvd < buf_size) {
		err = recv(socket, buf + recvd, buf_size - recvd, flags);
		if (err == -1) {
			fprintf(stderr, "%s: error recieving data: %s\n", __FUNCTION__,
					strerror(errno));
			free(buf);
			return (-1);
		}

		recvd += err;
	}

	*buf_ptr = buf;
	*buf_size_ptr = buf_size;

	return packet_size;
}

int
lash_open_socket(int *sockptr, const char *host, const char *service)
{
	struct addrinfo hints;
	struct addrinfo *addrs;
	struct addrinfo *addr;
	int sock = 0;
	int err;
	int connected = 0;

	LASH_DEBUGARGS("attempting to connect to host '%s', service '%s'", host,
				   service);

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;

	err = getaddrinfo(host, service, &hints, &addrs);
	if (err) {
		fprintf(stderr, "%s: could not look up host '%s': %s\n",
				__FUNCTION__, host, gai_strerror(err));
		return -1;
	}

	for (addr = addrs; addr; addr = addr->ai_next) {
#ifdef LASH_DEBUG
		{
			char num_addr[NI_MAXHOST];
			char port_addr[NI_MAXSERV];

			err = getnameinfo(addr->ai_addr, addr->ai_addrlen,
							  num_addr, sizeof(num_addr),
							  port_addr, sizeof(port_addr),
							  NI_NUMERICHOST | NI_NUMERICSERV);
			if (err) {
				fprintf(stderr,
						"%s: error looking up numeric host/port names: %s\n",
						__FUNCTION__, strerror(errno));
			} else {
				LASH_DEBUGARGS("attempting to connect to %s:%s", num_addr,
							   port_addr);
			}
		}
#endif

		sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
		if (sock == -1) {
			LASH_DEBUGARGS
				("could not create socket with params domain=%d, type=%d, protocol=%d: %s",
				 addr->ai_family, addr->ai_socktype, addr->ai_protocol,
				 strerror(errno));
			continue;
		}

		err = connect(sock, addr->ai_addr, addr->ai_addrlen);
		if (err) {
			LASH_DEBUGARGS("error connecting: %s", strerror(errno));

			err = close(sock);
			if (err) {
				fprintf(stderr, "%s: error closing unconnected socket: %s\n",
						__FUNCTION__, strerror(errno));
			}
		} else {
			connected = 1;
			break;
		}
	}

	freeaddrinfo(addrs);

	if (!connected) {
		fprintf(stderr, "%s: could not connect to host '%s', service '%s'\n",
				__FUNCTION__, host, service);
		return -1;
	}

	LASH_PRINT_DEBUG("socket connected");
	*sockptr = sock;
	return 0;
}

static int
lash_lookup_peer_info(int sock,
					  char *host, size_t host_len,
					  char *port, size_t port_len)
{
	struct sockaddr_storage ss;
	socklen_t ss_len = sizeof(ss);
	int err;

	err = getpeername(sock, (struct sockaddr *)&ss, &ss_len);
	if (err) {
		fprintf(stderr, "%s: could not get peer address: %s\n", __FUNCTION__,
				strerror(errno));
		return -1;
	}

	err = getnameinfo((struct sockaddr *)&ss, ss_len,
					  host, host_len, port, port_len, 0);
	if (err) {
		fprintf(stderr, "%s: could not look up peer name: %s\n", __FUNCTION__,
				strerror(errno));
		return -1;
	}

	return 0;
}

const char *
lash_lookup_peer_name(int sock)
{
	static char host[NI_MAXHOST];
	int err;

	err = lash_lookup_peer_info(sock, host, sizeof(host), NULL, 0);
	if (err)
		return NULL;

	return host;
}

const char *
lash_lookup_peer_port(int sock)
{
	static char port[NI_MAXSERV];
	int err;

	err = lash_lookup_peer_info(sock, NULL, 0, port, sizeof(port));
	if (err)
		return NULL;

	return port;
}

/* EOF */
