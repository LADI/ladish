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

#ifndef __LASHD_JACK_PATCH_H__
#define __LASHD_JACK_PATCH_H__

#include <uuid/uuid.h>
#include <libxml/tree.h>

#include "project.h"

typedef struct _jack_patch jack_patch_t;

struct _jack_patch
{
  char * src_client;
  char * src_port;
  char * dest_client;
  char * dest_port;
  uuid_t src_client_id;
  uuid_t dest_client_id;
};

jack_patch_t * jack_patch_new ();
jack_patch_t * jack_patch_dup (const jack_patch_t * patch);
void           jack_patch_destroy (jack_patch_t * patch);

void jack_patch_set_src_client  (jack_patch_t * patch, const char * src_client);
void jack_patch_set_src_port    (jack_patch_t * patch, const char * src_port);
void jack_patch_set_dest_client (jack_patch_t * patch, const char * dest_client);
void jack_patch_set_dest_port   (jack_patch_t * patch, const char * dest_port);

/* get/set both the client and port in/from a jack port name */
const char * jack_patch_get_src  (jack_patch_t * patch);
const char * jack_patch_get_dest (jack_patch_t * patch);
void         jack_patch_set_src  (jack_patch_t * patch, const char * src);
void         jack_patch_set_dest (jack_patch_t * patch, const char * dest);

const char * jack_patch_get_desc (jack_patch_t * patch);

/* set/unset the lash IDs */
void jack_patch_set            (jack_patch_t * patch, lash_list_t * jack_mgr_clients);
int  jack_patch_unset          (jack_patch_t * patch, lash_list_t * jack_mgr_clients);

void jack_patch_create_xml     (jack_patch_t * patch, xmlNodePtr parent);
void jack_patch_parse_xml      (jack_patch_t * patch, xmlNodePtr parent);

void jack_patch_switch_clients (jack_patch_t * patch);

#endif /* __LASHD_JACK_PATCH_H__ */
