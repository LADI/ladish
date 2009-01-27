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
#include "common/klist.h"

struct _jack_patch
{
	struct list_head  siblings;
	char             *src_client;
	char             *src_port;
	char             *dest_client;
	char             *dest_port;
	char             *src_desc;
	char             *dest_desc;
	uuid_t            src_client_id;
	uuid_t            dest_client_id;
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
jack_patch_list(struct list_head *list)
{
	struct list_head *node;
	list_for_each (node, list) {
		jack_patch_t *p = list_entry(node, jack_patch_t, siblings);
		lash_debug("%s -> %s", p->src_desc, p->dest_desc);
	}
}
#endif

#ifdef HAVE_JACK_DBUS

/** Create a new JACK patch with the given parameters and return a pointer to it.
 * The *_uuid and *_name parameters are mutually exclusive; for example if
 * src_uuid is not NULL then src_name must be NULL, and vice versa.
 * @param src_uuid Pointer to source client UUID, or NULL if \a src_name is given.
 * @param dest_uuid Pointer to destination client UUID, or NULL if \a dest_name is given.
 * @param src_name Source client name, or NULL if \a src_uuid is given.
 * @param dest_name Destination client name, or NULL if \a dest_uuid is given.
 * @param src_port Source port name.
 * @param dest_port Destination port name.
 * @return Pointer to newly allocated JACK patch.
 */
jack_patch_t *
jack_patch_new_with_all(uuid_t     *src_uuid,
                        uuid_t     *dest_uuid,
                        const char  *src_name,
                        const char  *dest_name,
                        const char  *src_port,
                        const char  *dest_port);

/** Search for the patch described by @a src_uuid, @a src_name, @a src_port,
 * @a dest_uuid, @a dest_name, and @a dest_port in @a patch_list and
 * return a pointer to it, or NULL if the patch was not found.
 * @param patch_list List of @ref jack_patch_t objects.
 * @param src_uuid Pointer to patch source client UUID, or NULL if unknown.
 * @param src_name Patch source client name.
 * @param src_port Patch source port name.
 * @param dest_uuid Pointer to patch destination client UUID, or NULL if unknown.
 * @param dest_name Patch destination client name.
 * @param dest_port Patch destination port name.
 * @return Pointer to patch or NULL if not found.
 */
jack_patch_t *
jack_patch_find_by_description(struct list_head *patch_list,
                               uuid_t           *src_uuid,
                               const char       *src__name,
                               const char       *src_port,
                               uuid_t           *dest_uuid,
                               const char       *dest_name,
                               const char       *dest_port);

#else /* !HAVE_JACK_DBUS */

/* set/unset the lash IDs */
void
jack_patch_set(jack_patch_t     *patch,
               struct list_head *jack_mgr_clients);

bool
jack_patch_unset(jack_patch_t     *patch,
                 struct list_head *jack_mgr_clients);

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
