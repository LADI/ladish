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

#ifndef __LASHD_JACK_MGR_CLIENT_H__
#define __LASHD_JACK_MGR_CLIENT_H__

#include "config.h"

#include <sys/types.h>
#include <uuid/uuid.h>

#include <dbus/dbus.h>

#include "../common/klist.h"
#include "types.h"

struct _jack_mgr_client
{
	struct list_head  siblings;
	char             *name;
	// TODO: Add pointer to parent LASH client object, use that to reference the UUID
	uuid_t            id;
	struct list_head  old_patches;
	struct list_head  backup_patches;
	dbus_uint64_t     jackdbus_id;
	pid_t             pid; /**< Client PID. */
};

jack_mgr_client_t *
jack_mgr_client_new(void);

void
jack_mgr_client_destroy(jack_mgr_client_t *client);

void
jack_mgr_client_dup_patch_list(struct list_head *src,
                               struct list_head *dest);

void
jack_mgr_client_free_patch_list(struct list_head *patch_list);

jack_mgr_client_t *
jack_mgr_client_find_by_id(struct list_head *client_list,
                           uuid_t            id);

jack_mgr_client_t *
jack_mgr_client_find_by_jackdbus_id(struct list_head *client_list,
                                    dbus_uint64_t     id);

/** Find a JACK client in \a client_list whose PID matches \a pid.
 * @param client_list List of type \ref jack_mgr_client_t.
 * @param pid PID to search for.
 * @return Pointer to JACK client, or NULL if no matches were found.
 */
jack_mgr_client_t *
jack_mgr_client_find_by_pid(struct list_head *client_list,
                            pid_t             pid);

/** Notify the world that @a client 's parameters were modified.
 * @param client Pointer to JACK client.
 */
void
jack_mgr_client_modified(jack_mgr_client_t *client);

#endif /* __LASHD_JACK_MGR_CLIENT_H__ */
