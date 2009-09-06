/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the implementation of the dictionary objects
 **************************************************************************
 *
 * LADI Session Handler is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * LADI Session Handler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LADI Session Handler. If not, see <http://www.gnu.org/licenses/>
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "dict.h"

struct ladish_dict_entry
{
  struct list_head siblings;
  char * key;
  char * value;
};

struct ladish_dict
{
  struct list_head entries;
};

bool ladish_dict_create(ladish_dict_handle * dict_handle_ptr)
{
  struct ladish_dict * dict_ptr;

  dict_ptr = malloc(sizeof(struct ladish_dict));
  if (dict_ptr == NULL)
  {
    lash_error("malloc() failed to allocate struct ladish_dict");
    return false;
  }

  INIT_LIST_HEAD(&dict_ptr->entries);

  *dict_handle_ptr = (ladish_dict_handle)dict_ptr;

  return true;
}

static struct ladish_dict_entry * ladish_dict_find_key(struct ladish_dict * dict_ptr, const char * key)
{
  struct list_head * node_ptr;
  struct ladish_dict_entry * entry_ptr;

  list_for_each(node_ptr, &dict_ptr->entries)
  {
    entry_ptr = list_entry(node_ptr, struct ladish_dict_entry, siblings);
    if (strcmp(entry_ptr->key, key) == 0)
    {
      return entry_ptr;
    }
  }

  return NULL;
}

static void ladish_dict_drop_entry(struct ladish_dict_entry * entry_ptr)
{
  list_del(&entry_ptr->siblings);
  free(entry_ptr->key);
  free(entry_ptr->value);
  free(entry_ptr);
}

#define dict_ptr ((struct ladish_dict *)dict_handle)

void ladish_dict_destroy(ladish_dict_handle dict_handle)
{
  ladish_dict_clear(dict_handle);
  free(dict_ptr);
}

bool ladish_dict_set(ladish_dict_handle dict_handle, const char * key, const char * value)
{
  struct ladish_dict_entry * entry_ptr;
  char * new_value;

  entry_ptr = ladish_dict_find_key(dict_ptr, key);
  if (entry_ptr != NULL)
  {
    new_value = strdup(value);
    if (new_value == NULL)
    {
      lash_error("strdup() failed to duplicate dict value");
      return false;
    }

    free(entry_ptr->value);
    entry_ptr->value = new_value;
    return true;
  }

  entry_ptr = malloc(sizeof(struct ladish_dict_entry));
  if (entry_ptr == NULL)
  {
    lash_error("malloc() failed to allocate struct ladish_dict_entry");
    return false;
  }

  entry_ptr->key = strdup(key);
  if (entry_ptr->key == NULL)
  {
    lash_error("strdup() failed to duplicate dict key");
    free(entry_ptr);
    return false;
  }

  entry_ptr->value = strdup(value);
  if (entry_ptr->value == NULL)
  {
    lash_error("strdup() failed to duplicate dict value");
    free(entry_ptr->key);
    free(entry_ptr);
    return false;
  }

  list_add_tail(&entry_ptr->siblings, &dict_ptr->entries);

  return true;
}

const char * ladish_dict_get(ladish_dict_handle dict_handle, const char * key)
{
  struct ladish_dict_entry * entry_ptr;

  entry_ptr = ladish_dict_find_key(dict_ptr, key);
  if (entry_ptr == NULL)
  {
    return NULL;
  }

  assert(entry_ptr->value != NULL);
  return entry_ptr->value;
}

void ladish_dict_drop(ladish_dict_handle dict_handle, const char * key)
{
  struct ladish_dict_entry * entry_ptr;

  entry_ptr = ladish_dict_find_key(dict_ptr, key);
  if (entry_ptr != NULL)
  {
    ladish_dict_drop_entry(entry_ptr);
  }
}

void ladish_dict_clear(ladish_dict_handle dict_handle)
{
  struct ladish_dict_entry * entry_ptr;

  while (!list_empty(&dict_ptr->entries))
  {
    entry_ptr = list_entry(dict_ptr->entries.next, struct ladish_dict_entry, siblings);
    ladish_dict_drop_entry(entry_ptr);
  }
}

bool ladish_dict_iterate(ladish_dict_handle dict_handle, void * context, bool (* callback)(void * context, const char * key, const char * value))
{
  struct list_head * node_ptr;
  struct ladish_dict_entry * entry_ptr;

  list_for_each(node_ptr, &dict_ptr->entries)
  {
    entry_ptr = list_entry(node_ptr, struct ladish_dict_entry, siblings);
    if (!callback(context, entry_ptr->key, entry_ptr->value))
    {
      return false;
    }
  }

  return true;
}

#undef dict_ptr
