/* -*- Mode: C -*- */
/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
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

#ifndef __LASHD_JACKDBUS_MGR_H__
#define __LASHD_JACKDBUS_MGR_H__

#include <stdbool.h>
#include <uuid/uuid.h>
#include <dbus/dbus.h>

#include "../common/klist.h"
#include "types.h"
#include "client.h"

struct _lashd_jackdbus_mgr
{
  struct list_head  clients;
  DBusMessage      *graph;
  dbus_uint64_t     graph_version;
};

lashd_jackdbus_mgr_t *
lashd_jackdbus_mgr_new(void);

void
lashd_jackdbus_mgr_destroy(lashd_jackdbus_mgr_t *mgr);

bool
lashd_jackdbus_mgr_remove_client(lashd_jackdbus_mgr_t *mgr,
                                 uuid_t                id,
                                 struct list_head     *backup_patches);

/** Find client with UUID @a id in jackdbus manager @a mgr and append its
 * patches to @a list. \a list must be properly initialized before calling.
 * If the operation fails the contents of \a list is undefined.
 * @param mgr Pointer to JACK D-Bus manager.
 * @param id UUID of client.
 * @param dest Pointer to target list head. Must be initialized.
 * @return True on success, false otherwise.
 */
bool
lashd_jackdbus_mgr_get_client_patches(lashd_jackdbus_mgr_t *mgr,
                                      uuid_t                id,
                                      struct list_head     *dest);

void
lashd_jackdbus_mgr_get_graph(lashd_jackdbus_mgr_t *mgr);

#endif /* __LASHD_JACKDBUS_MGR_H__ */
