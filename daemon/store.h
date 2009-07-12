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

#ifndef __LASHD_STORE_H__
#define __LASHD_STORE_H__

#include <stdbool.h>
#include <sys/types.h>
#include <dbus/dbus.h>

#include "../common/klist.h"

#include "types.h"

/* When a store is created, it will load the data from a directory if one
 * exists, but it won't create it, or the directory.  It will create files
 * when told to write to disk.
 */

struct _store
{
  char             *dir;
  unsigned long     num_keys;
  struct list_head  keys;
  struct list_head  removed_keys;
  struct list_head  unstored_configs;
};

store_t *
store_new(void);

void
store_destroy(store_t *store);

bool
store_open(store_t *store);

bool
store_write(store_t *store);

bool
store_set_config(store_t    *store,
                 const char *key_name,
                 const void *value,
                 size_t      size,
                 int         type);

bool
store_create_config_array(store_t         *store,
                          DBusMessageIter *iter);

#endif /* __LASHD_STORE_H__ */
