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

#ifndef __LASHD_STORE_H__
#define __LASHD_STORE_H__

#include <lash/lash.h>

/* When a store is created, it will load the data from a directory if one
 * exists, but it won't create it, or the directory.  It will create files
 * when told to write to disk.
 */

typedef struct _store store_t;

struct _store
{
  char *        dir;
  lash_list_t *  keys;
  unsigned long key_count;
  lash_list_t *  unstored_configs;
  lash_list_t *  removed_configs;
};

store_t *  store_new          ();
void       store_destroy      (store_t * store);

void         store_set_dir (store_t * store, const char * dir);



int store_open           (store_t * store);
int store_write          (store_t * store);

void store_remove_config (store_t * store, const char * key);
void store_set_config    (store_t * store, const lash_config_t * config);

unsigned long  store_get_key_count (const store_t * store);
lash_list_t *   store_get_keys      (store_t * store);
lash_config_t * store_get_config    (store_t * store, const char * key);

#endif /* __LASHD_STORE_H__ */
