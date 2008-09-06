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

#include "../config.h"

#include <stdbool.h>
#include <uuid/uuid.h>
#include <libxml/tree.h>

#ifdef HAVE_JACK_DBUS
# include <dbus/dbus.h>
#endif

#include "types.h"
#include "common/list.h"

struct _jack_patch
{
	char   *src_client;
	char   *src_port;
	char   *dest_client;
	char   *dest_port;
	char   *src_desc;
	char   *dest_desc;
	uuid_t  src_client_id;
	uuid_t  dest_client_id;
};

jack_patch_t *
jack_patch_new(void);

void
jack_patch_destroy(jack_patch_t *patch);

void
jack_patch_create_xml(jack_patch_t *patch,
                      xmlNodePtr    parent);

void
jack_patch_parse_xml(jack_patch_t *patch,
                     xmlNodePtr    parent);

jack_patch_t *
jack_patch_dup(const jack_patch_t *patch);

#ifdef LASH_DEBUG
# include "common/debug.h"
static __inline__ void
jack_patch_list(lash_list_t *list)
{
	for (; list; list = lash_list_next(list))
		lash_debug("%s -> %s",
		           ((jack_patch_t *) list->data)->src_desc,
		           ((jack_patch_t *) list->data)->dest_desc);
}
#endif

#ifndef HAVE_JACK_DBUS

/* set/unset the lash IDs */
void
jack_patch_set(jack_patch_t *patch,
               lash_list_t  *jack_mgr_clients);

bool
jack_patch_unset(jack_patch_t *patch,
                 lash_list_t  *jack_mgr_clients);

void
jack_patch_switch_clients(jack_patch_t *patch);

/* Set both the client and port in/from a jack port name */
void
jack_patch_set_src(jack_patch_t *patch,
                   const char   *src);

void
jack_patch_set_dest(jack_patch_t *patch,
                    const char   *dest);

#endif

#endif /* __LASHD_JACK_PATCH_H__ */
