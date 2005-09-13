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

#define _GNU_SOURCE

#include <lash/lash.h>
#include <lash/internal_headers.h>

#include "jack_mgr_client.h"

static void
jack_mgr_client_free_patch_list (lash_list_t ** list_ptr)
{
  lash_list_t * list;
  jack_patch_t * patch;

  for (list = *list_ptr; list; list = lash_list_next (list))
    {
      patch = (jack_patch_t *) list->data;
      if (!patch)
        {
          LASH_PRINT_DEBUG ("NULL patch!")
        }
      else
        jack_patch_destroy (patch);
    }
    
  lash_list_free (*list_ptr);
  
  *list_ptr = NULL;
}

void
jack_mgr_client_free_patches (jack_mgr_client_t * client)
{
  if (!client->patches)
    return;
  
  jack_mgr_client_free_patch_list (&client->patches);
}

void
jack_mgr_client_free_backup_patches (jack_mgr_client_t * client)
{
  if (!client->backup_patches)
    return;
  
  jack_mgr_client_free_patch_list (&client->backup_patches);
}

static void
jack_mgr_client_free_old_patches (jack_mgr_client_t * client)
{
  jack_mgr_client_free_patch_list (&client->old_patches);
}

void
jack_mgr_client_free (jack_mgr_client_t * client)
{
  jack_mgr_client_set_name (client, NULL);
  jack_mgr_client_free_patches (client);
  jack_mgr_client_free_old_patches (client);
}

jack_mgr_client_t *
jack_mgr_client_new ()
{
  jack_mgr_client_t * client;
  client = lash_malloc0 (sizeof (jack_mgr_client_t));
  uuid_clear (client->id);
  return client;
}

void
jack_mgr_client_destroy (jack_mgr_client_t * client)
{
  jack_mgr_client_free (client);
  free (client);
}

void
jack_mgr_client_set_id          (jack_mgr_client_t * client, uuid_t id)
{
  uuid_copy (client->id, id);
}

void
jack_mgr_client_set_name        (jack_mgr_client_t * client, const char * name)
{
  set_string_property (client->name, name);
}

lash_list_t *
jack_mgr_client_dup_patches	(const jack_mgr_client_t * client)
{
  lash_list_t * list = NULL, * exlist;
  jack_patch_t * patch, * expatch;
  
  for (exlist = client->patches; exlist; exlist = lash_list_next (exlist))
    {
      expatch = (jack_patch_t *) exlist->data;
      
      patch = jack_patch_dup (expatch);
      list = lash_list_append (list, patch);

      LASH_DEBUGARGS ("duplicated jack patch '%s': '%s'",
                      jack_patch_get_desc (expatch),
                      jack_patch_get_desc (patch));
    }
  
  return list;
}

lash_list_t *
jack_mgr_client_get_patches	(jack_mgr_client_t * client)
{
  return client->patches;
}

const char *
jack_mgr_client_get_name        (const jack_mgr_client_t * client)
{
  return client->name;
}

void
jack_mgr_client_get_id          (const jack_mgr_client_t * client, uuid_t id)
{
  uuid_copy (id, ((jack_mgr_client_t *)client)->id);
}

/* EOF */

