/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains studio object helpers
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

#include "common.h"
#include "jack.h"

bool
studio_create(
  struct studio ** studio_ptr_ptr)
{
  struct studio * studio_ptr;

  lash_info("studio object construct");

  studio_ptr = malloc(sizeof(struct studio));
  if (studio_ptr == NULL)
  {
    lash_error("malloc() failed to allocate struct studio");
    return false;
  }

  INIT_LIST_HEAD(&studio_ptr->all_connections);
  INIT_LIST_HEAD(&studio_ptr->all_ports);
  INIT_LIST_HEAD(&studio_ptr->all_clients);
  INIT_LIST_HEAD(&studio_ptr->jack_connections);
  INIT_LIST_HEAD(&studio_ptr->jack_ports);
  INIT_LIST_HEAD(&studio_ptr->jack_clients);
  INIT_LIST_HEAD(&studio_ptr->rooms);
  INIT_LIST_HEAD(&studio_ptr->clients);
  INIT_LIST_HEAD(&studio_ptr->ports);

  studio_ptr->modified = false;
  studio_ptr->persisted = false;
  studio_ptr->jack_conf_obsolete = false;
  studio_ptr->jack_conf_stable = false;

  INIT_LIST_HEAD(&studio_ptr->jack_conf);

  *studio_ptr_ptr = studio_ptr;

  return true;
}

void
studio_destroy(
  struct studio * studio_ptr)
{
  struct list_head * node_ptr;

  while (!list_empty(&studio_ptr->jack_conf))
  {
    node_ptr = studio_ptr->jack_conf.next;
    list_del(node_ptr);
    jack_conf_container_destroy(list_entry(node_ptr, struct jack_conf_container, siblings));
  }

  free(studio_ptr);
  lash_info("studio object destroy");
}
