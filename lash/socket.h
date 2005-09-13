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

#ifndef __LASH_SOCKET_H__
#define __LASH_SOCKET_H__

/* returns -1 on error, -2 if the socket was closed, or buf_size */
int lash_sendall (int socket, void * buf, size_t buf_size, int flags);
int lash_recvall (int socket, void ** bufptr, size_t * buf_size, int flags);

/* returns 0 on success, non-0 on error */
int lash_open_socket (int * socket, const char * host, const char * service);

/* return hostname on success, or NULL on error */
const char * lash_lookup_peer_name (int sock);
const char * lash_lookup_peer_port (int sock);


#endif /* __LASH_SOCKET_H__ */
