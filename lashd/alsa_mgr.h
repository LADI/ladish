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

#ifndef __LASHD_ALSA_MGR_H__
#define __LASHD_ALSA_MGR_H__

#include "../config.h"

#ifdef HAVE_ALSA

# include <pthread.h>
# include <uuid/uuid.h>
# include <alsa/asoundlib.h>

# include "types.h"
# include "common/list.h"

struct _alsa_mgr
{
  pthread_mutex_t  lock;

  snd_seq_t *      seq;

  pthread_t        event_thread;
  
  lash_list_t *     clients;
  lash_list_t *     foreign_ports;
};

alsa_mgr_t * alsa_mgr_new     (void);
void         alsa_mgr_destroy (alsa_mgr_t * alsa_mgr);

void         alsa_mgr_add_client         (alsa_mgr_t * alsa_mgr,
                                          uuid_t id,
                                          unsigned char alsa_client_id,
                                          lash_list_t * alsa_patches);
lash_list_t * alsa_mgr_remove_client      (alsa_mgr_t * alsa_mgr, uuid_t id);
lash_list_t * alsa_mgr_get_client_patches (alsa_mgr_t * alsa_mgr, uuid_t id);

void alsa_mgr_lock (alsa_mgr_t * alsa_mgr);
void alsa_mgr_unlock (alsa_mgr_t * alsa_mgr);

const char * get_alsa_port_name_only (const char * port_name);

#endif /* HAVE_ALSA */

#endif /* __LASHD_ALSA_MGR_H__ */
