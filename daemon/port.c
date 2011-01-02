/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010, 2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the implementation of the port objects
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

#include "port.h"

/* JACK port */
struct ladish_port
{
  int refcount;
  uuid_t uuid;                             /* The UUID of the port */
  uuid_t app_uuid;                         /* The UUID of the app that owns this client */
  bool link;                               /* Whether the port is studio-room link port */
  uint64_t jack_id;                        /* JACK port ID. */
  uint64_t jack_id_room;                   /* JACK port ID in room. valid only for link ports */

  pid_t pid;                               /* process id. */

  void * vgraph;                /* virtual graph */

  ladish_dict_handle dict;
};

bool
ladish_port_create(
  const uuid_t uuid_ptr,
  bool link,
  ladish_port_handle * port_handle_ptr)
{
  struct ladish_port * port_ptr;

  port_ptr = malloc(sizeof(struct ladish_port));
  if (port_ptr == NULL)
  {
    log_error("malloc() failed to allocate struct ladish_port");
    return false;
  }

  if (!ladish_dict_create(&port_ptr->dict))
  {
    log_error("ladish_dict_create() failed for port");
    free(port_ptr);
    return false;
  }

  if (uuid_ptr == NULL)
  {
    uuid_generate(port_ptr->uuid);
  }
  else
  {
    uuid_copy(port_ptr->uuid, uuid_ptr);
  }

  uuid_clear(port_ptr->app_uuid);

  port_ptr->jack_id = 0;
  port_ptr->jack_id_room = 0;
  port_ptr->link = link;
  port_ptr->refcount = 0;

  port_ptr->vgraph = NULL;

  log_info("port %p created", port_ptr);
  *port_handle_ptr = (ladish_port_handle)port_ptr;
  return true;
}

#define port_ptr ((struct ladish_port * )port_handle)

bool ladish_port_create_copy(ladish_port_handle port_handle, ladish_port_handle * port_handle_ptr)
{
  return ladish_port_create(port_ptr->uuid, port_ptr->link, port_handle_ptr);
}

void ladish_port_destroy(ladish_port_handle port_handle)
{
  log_info("port %p destroy", port_ptr);
  ASSERT(port_ptr->refcount == 0);
  ladish_dict_destroy(port_ptr->dict);
  free(port_ptr);
}

ladish_dict_handle ladish_port_get_dict(ladish_port_handle port_handle)
{
  return port_ptr->dict;
}

void ladish_port_get_uuid(ladish_port_handle port_handle, uuid_t uuid)
{
  uuid_copy(uuid, port_ptr->uuid);
}

void ladish_port_set_jack_id(ladish_port_handle port_handle, uint64_t jack_id)
{
  log_info("port %p jack id set to %"PRIu64, port_handle, jack_id);
  port_ptr->jack_id = jack_id;
}

uint64_t ladish_port_get_jack_id(ladish_port_handle port_handle)
{
  return port_ptr->jack_id;
}

void ladish_port_set_jack_id_room(ladish_port_handle port_handle, uint64_t jack_id)
{
  log_info("port %p jack id (room) set to %"PRIu64, port_handle, jack_id);
  ASSERT(port_ptr->link);
  port_ptr->jack_id_room = jack_id;
}

uint64_t ladish_port_get_jack_id_room(ladish_port_handle port_handle)
{
  if (port_ptr->link)
  {
    return port_ptr->jack_id_room;
  }
  else
  {
    return port_ptr->jack_id;
  }
}

void ladish_port_add_ref(ladish_port_handle port_handle)
{
  port_ptr->refcount++;
}

void ladish_port_del_ref(ladish_port_handle port_handle)
{
  ASSERT(port_ptr->refcount > 0);
  port_ptr->refcount--;

  if (port_ptr->refcount == 0)
  {
    ladish_port_destroy(port_handle);
  }
}

bool ladish_port_is_link(ladish_port_handle port_handle)
{
  return port_ptr->link;
}

void ladish_port_set_vgraph(ladish_port_handle port_handle, void * vgraph)
{
  port_ptr->vgraph = vgraph;
}

void * ladish_port_get_vgraph(ladish_port_handle port_handle)
{
  return port_ptr->vgraph;
}

void ladish_port_set_app(ladish_port_handle port_handle, const uuid_t app_uuid)
{
  uuid_copy(port_ptr->app_uuid, app_uuid);
}

bool ladish_port_get_app(ladish_port_handle port_handle, uuid_t app_uuid)
{
  if (uuid_is_null(port_ptr->app_uuid))
  {
    return false;
  }

  uuid_copy(app_uuid, port_ptr->app_uuid);
  return true;
}

bool ladish_port_has_app(ladish_port_handle port_handle)
{
  return !uuid_is_null(port_ptr->app_uuid);
}

bool ladish_port_belongs_to_app(ladish_port_handle port_handle, const uuid_t app_uuid)
{
  if (uuid_is_null(port_ptr->app_uuid))
  {
    return false;
  }

  return uuid_compare(port_ptr->app_uuid, app_uuid) == 0;
}

void ladish_port_set_pid(ladish_port_handle port_handle, pid_t pid)
{
  port_ptr->pid = pid;
}

pid_t ladish_port_get_pid(ladish_port_handle port_handle)
{
  return port_ptr->pid;
}

#undef port_ptr
