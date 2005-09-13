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

#ifndef __LASH_JACK_MGR_CLIENT_H__
#define __LASH_JACK_MGR_CLIENT_H__

#include <pthread.h>
#include <uuid/uuid.h>

#include "jack_patch.h"

typedef struct _jack_mgr_client jack_mgr_client_t;

struct _jack_mgr_client
{
  char *          name;
  uuid_t          id;
  lash_list_t *    patches;
  lash_list_t *    old_patches;
  lash_list_t *    backup_patches;
};

void jack_mgr_client_init (jack_mgr_client_t * client);
void jack_mgr_client_free (jack_mgr_client_t * client);

jack_mgr_client_t * jack_mgr_client_new ();
void                jack_mgr_client_destroy (jack_mgr_client_t * client);

void jack_mgr_client_set_id          (jack_mgr_client_t * client, uuid_t id);
void jack_mgr_client_set_name        (jack_mgr_client_t * client, const char * name);
void jack_mgr_client_add_patch       (jack_mgr_client_t * client, jack_patch_t * patch);

lash_list_t * jack_mgr_client_dup_patches     (const jack_mgr_client_t * client);
lash_list_t * jack_mgr_client_get_patches     (jack_mgr_client_t * client);
const char * jack_mgr_client_get_name        (const jack_mgr_client_t * client);
void         jack_mgr_client_get_id          (const jack_mgr_client_t * client, uuid_t id);

void jack_mgr_client_free_patches (jack_mgr_client_t * client);
void jack_mgr_client_free_backup_patches (jack_mgr_client_t * client);

#endif /* __LASH_JACK_MGR_CLIENT_H__ */
