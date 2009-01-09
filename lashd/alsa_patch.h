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

#ifndef __LASHD_ALSA_PATCH_H__
#define __LASHD_ALSA_PATCH_H__

#include "../config.h"

#ifdef HAVE_ALSA

# include <uuid/uuid.h>
# include <alsa/asoundlib.h>
# include <libxml/tree.h>

# include "types.h"
# include "common/klist.h"

struct _alsa_patch
{
  struct list_head siblings;
  uuid_t src_id;
  uuid_t dest_id;
  snd_seq_port_subscribe_t * sub;
};

alsa_patch_t * alsa_patch_new            ();
alsa_patch_t * alsa_patch_dup            (const alsa_patch_t * patch);
alsa_patch_t * alsa_patch_new_with_sub   (const snd_seq_port_subscribe_t * sub);
alsa_patch_t * alsa_patch_new_with_query (const snd_seq_query_subscribe_t * query);
void           alsa_patch_destroy        (alsa_patch_t * patch);

unsigned char alsa_patch_get_src_client  (const alsa_patch_t * patch);
unsigned char alsa_patch_get_src_port    (const alsa_patch_t * patch);
unsigned char alsa_patch_get_dest_client (const alsa_patch_t * patch);
unsigned char alsa_patch_get_dest_port   (const alsa_patch_t * patch);

int  alsa_patch_src_id_is_null  (alsa_patch_t * patch);
int  alsa_patch_dest_id_is_null (alsa_patch_t * patch);

void alsa_patch_set_sub            (alsa_patch_t * patch, const snd_seq_port_subscribe_t * sub);
void alsa_patch_set_sub_from_query (alsa_patch_t * patch, const snd_seq_query_subscribe_t * query);

void alsa_patch_set   (alsa_patch_t * patch, struct list_head * alsa_mgr_clients);
int  alsa_patch_unset (alsa_patch_t * patch, struct list_head * alsa_mgr_clients);

const snd_seq_port_subscribe_t * query_to_sub (const snd_seq_query_subscribe_t * query);

/* this isn't ordered; 0 == same; 1 == different */
int  alsa_patch_compare (alsa_patch_t * a, alsa_patch_t * b);

void alsa_patch_switch_clients (alsa_patch_t * patch);

void alsa_patch_create_xml (alsa_patch_t * patch, xmlNodePtr parent);
void alsa_patch_parse_xml  (alsa_patch_t * patch, xmlNodePtr parent);

const char * alsa_patch_get_desc (alsa_patch_t * patch);

#endif /* HAVE_ALSA */

#endif /* __LASHD_ALSA_PATCH_H__ */
