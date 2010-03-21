/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains declaration of internal stuff used by
 * studio object implementation
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

#ifndef STUDIO_INTERNAL_H__B4CB73EC_8E89_401A_9E49_F0AEEF361D09__INCLUDED
#define STUDIO_INTERNAL_H__B4CB73EC_8E89_401A_9E49_F0AEEF361D09__INCLUDED

#include "../../proxies/jack_proxy.h"
#include "../dbus/error.h"
#include "virtualizer.h"
#include "app_supervisor.h"
#include "cmd.h"

#define JACK_CONF_MAX_ADDRESS_SIZE 1024

typedef int ladish_environment_id;

typedef struct
{
  uint64_t state;
  uint64_t changed;
} ladish_environment_store;

#define ladish_environment_jack_server_present    ((ladish_environment_id)0)
#define ladish_environment_jack_server_started    ((ladish_environment_id)1)

static inline void ladish_environment_init(ladish_environment_store * store_ptr)
{
  store_ptr->state = 0;
  store_ptr->changed = 0;
}

static inline uint64_t ladish_environment_state(ladish_environment_id id)
{
  ASSERT(sizeof(id) < 8);
  return (uint64_t)1 << id;
}

static inline void ladish_environment_set(ladish_environment_store * store_ptr, ladish_environment_id id)
{
  uint64_t state = ladish_environment_state(id);
  store_ptr->state |= state;
  store_ptr->changed |= state;
}

static inline void ladish_environment_set_stealth(ladish_environment_store * store_ptr, ladish_environment_id id)
{
  uint64_t state = ladish_environment_state(id);
  store_ptr->state |= state;
}

static inline void ladish_environment_reset(ladish_environment_store * store_ptr, ladish_environment_id id)
{
  uint64_t state = ladish_environment_state(id);
  store_ptr->state &= ~state;
  store_ptr->changed |= state;
}

static inline void ladish_environment_reset_stealth(ladish_environment_store * store_ptr, ladish_environment_id id)
{
  uint64_t state = ladish_environment_state(id);
  store_ptr->state &= ~state;
}

static inline bool ladish_environment_get(ladish_environment_store * store_ptr, ladish_environment_id id)
{
  uint64_t state = ladish_environment_state(id);

  return (store_ptr->state & state) == state;
}

static inline bool ladish_environment_consume_change(ladish_environment_store * store_ptr, ladish_environment_id id, bool * new_state)
{
  uint64_t state = ladish_environment_state(id);

  if ((store_ptr->changed & state) == 0)
  {
    return false;
  }

  *new_state = (store_ptr->state & state) == state;
  store_ptr->changed &= ~state;
  return true;
}

static inline void ladish_environment_ignore(ladish_environment_store * store_ptr, ladish_environment_id id)
{
  uint64_t state = ladish_environment_state(id);

  if ((store_ptr->changed & state) != 0)
  {
    store_ptr->changed &= ~state;
  }
}

#define ladish_environment_assert_consumed(store_ptr) ASSERT((store_ptr)->changed == 0)

struct studio
{
  struct list_head all_connections;        /* All connections (studio guts and all rooms). Including superconnections. */
  struct list_head all_ports;              /* All ports (studio guts and all rooms) */
  struct list_head all_clients;            /* All clients (studio guts and all rooms) */
  struct list_head jack_connections;       /* JACK connections (studio guts and all rooms). Including superconnections, excluding virtual connections. */
  struct list_head jack_ports;             /* JACK ports (studio guts and all rooms). Excluding virtual ports. */
  struct list_head jack_clients;           /* JACK clients (studio guts and all rooms). Excluding virtual clients. */
  struct list_head rooms;                  /* Rooms connected to the studio */
  struct list_head clients;                /* studio clients (studio guts and room links) */
  struct list_head ports;                  /* studio ports (studio guts and room links) */

  bool automatic:1;             /* Studio was automatically created because of external JACK start */
  bool persisted:1;             /* Studio has on-disk representation, i.e. can be reloaded from disk */
  bool modified:1;              /* Studio needs saving */
  bool jack_conf_valid:1;       /* JACK server configuration obtained successfully */

  struct list_head jack_conf;   /* root of the conf tree */
  struct list_head jack_params; /* list of conf tree leaves */

  dbus_object_path dbus_object;

  struct ladish_cqueue cmd_queue;
  ladish_environment_store env_store;

  char * name;
  char * filename;

  graph_proxy_handle jack_graph_proxy;
  ladish_graph_handle jack_graph;
  ladish_graph_handle studio_graph;
  ladish_virtualizer_handle virtualizer;
  ladish_app_supervisor_handle app_supervisor;

  unsigned int room_count;
};

struct jack_conf_parameter
{
  struct list_head siblings;    /* siblings in container children list */
  struct list_head leaves;      /* studio::jack_param siblings */
  char * name;
  struct jack_conf_container * parent_ptr;
  char address[JACK_CONF_MAX_ADDRESS_SIZE];
  struct jack_parameter_variant parameter;
};

struct jack_conf_container
{
  struct list_head siblings;
  char * name;
  struct jack_conf_container * parent_ptr;
  bool children_leafs;          /* if true, children are "jack_conf_parameter"s, if false, children are "jack_conf_container"s */
  struct list_head children;
};

struct conf_callback_context
{
  char address[JACK_CONF_MAX_ADDRESS_SIZE];
  struct list_head * container_ptr;
  struct jack_conf_container * parent_ptr;
};

extern struct studio g_studio;

extern const struct dbus_interface_descriptor g_interface_studio;

void jack_conf_clear(void);
bool studio_fetch_jack_settings(void);
bool studio_compose_filename(const char * name, char ** filename_ptr_ptr, char ** backup_filename_ptr_ptr);
bool studio_publish(void);
bool studio_name_generate(char ** name_ptr);
bool studio_is_started(void);
void emit_studio_started(void);
void emit_studio_stopped(void);
void emit_studio_renamed(void);
void on_event_jack_started(void);
void on_event_jack_stopped(void);
void studio_remove_all_rooms(void);

#endif /* #ifndef STUDIO_INTERNAL_H__B4CB73EC_8E89_401A_9E49_F0AEEF361D09__INCLUDED */
