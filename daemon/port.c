/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
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

/* JACK port or virtual port */
struct ladish_port
{
  struct list_head siblings_studio_all;    /* link for the studio::all_ports list */
  struct list_head siblings_studio;        /* link for the studio::ports list */
  struct list_head siblings_room;          /* link for the room::ports list */
  struct list_head siblings_client;        /* link for the port list of the client */
  struct list_head siblings_vclient;       /* link for the port list of the virtual client */

  uuid_t uuid;                             /* The UUID of the port */
  bool virtual;                            /* Whether the port is virtual or JACK port */
  char * jack_name;                        /* JACK name (short). Not valid for virtual ports. */
  uint64_t jack_id;                        /* JACK port ID. Not valid for virtual ports. */
  char * human_name;                       /* User assigned name */

  struct client * client_ptr;              /* JACK client this port belongs to. Not valid for virtual ports. */
  struct client * vclient_ptr;             /* Virtual client this port belongs to. NULL if there is no virtual client associated. */

  /* superconnections are not in these lists */
  struct list_head input_connections;      /* list of input connections, i.e. connections that play to this port */
  struct list_head output_connections;     /* list of output connections, i.e. connections that capture from this port */

  ladish_dict_handle dict;
};

bool
ladish_port_create(
  uuid_t uuid_ptr,
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

  INIT_LIST_HEAD(&port_ptr->siblings_studio_all);
  INIT_LIST_HEAD(&port_ptr->siblings_room);
  port_ptr->jack_id = 0;
  port_ptr->jack_name = NULL;
  port_ptr->human_name = NULL;
  port_ptr->virtual = true;

  log_info("port %p created", port_ptr);
  *port_handle_ptr = (ladish_port_handle)port_ptr;
  return true;
}

#define port_ptr ((struct ladish_port * )port_handle)

void ladish_port_destroy(ladish_port_handle port_handle)
{
  log_info("port %p destroy", port_ptr);
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
  port_ptr->jack_id = jack_id;
}

uint64_t ladish_port_get_jack_id(ladish_port_handle port_handle)
{
  return port_ptr->jack_id;
}

#undef port_ptr
