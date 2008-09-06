/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
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

#ifndef __LASHD_JACK_MGR_H__
#define __LASHD_JACK_MGR_H__

#include <pthread.h>
#include <uuid/uuid.h>
#include <jack/jack.h>

#include "common/list.h"

#include "types.h"

struct _jack_mgr
{
	pthread_mutex_t  lock;
	jack_client_t   *jack_client;
	pthread_t        callback_thread;
	int              callback_write_socket;
	int              callback_read_socket;
	lash_list_t     *clients;
	lash_list_t     *foreign_ports;
	int              quit;
};

jack_mgr_t *
jack_mgr_new(void);

void
jack_mgr_destroy(jack_mgr_t *jack_mgr);

void
jack_mgr_lock(jack_mgr_t *jack_mgr);

void
jack_mgr_unlock(jack_mgr_t *jack_mgr);

void
jack_mgr_add_client(jack_mgr_t  *jack_mgr,
                    uuid_t       id,
                    const char  *jack_client_name,
                    lash_list_t *jack_patches);

lash_list_t *
jack_mgr_remove_client(jack_mgr_t *jack_mgr,
                       uuid_t      id);

lash_list_t *
jack_mgr_get_client_patches(jack_mgr_t *jack_mgr,
                            uuid_t      id);

#endif /* __LASHD_JACK_MGR_H__ */
