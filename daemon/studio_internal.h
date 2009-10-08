/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
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

#include "../jack_proxy.h"
#include "../dbus/error.h"
#include "jack_dispatch.h"

#define JACK_CONF_MAX_ADDRESS_SIZE 1024

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
  bool jack_running:1;          /* JACK server is running */
  bool jack_pending:1;          /* JACK server start/stop is pending */
  bool clear_on_jack_stop:1;    /* clear studio when JACK is stopped */

  struct list_head jack_conf;   /* root of the conf tree */
  struct list_head jack_params; /* list of conf tree leaves */

  dbus_object_path dbus_object;

  struct list_head event_queue;

  char * name;
  char * filename;

  graph_proxy_handle jack_graph_proxy;
  ladish_graph_handle jack_graph;
  ladish_graph_handle studio_graph;
  ladish_jack_dispatcher_handle jack_dispatcher;
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
bool studio_clear(bool defer_if_started);
bool studio_publish(void);
bool studio_start(void);
bool studio_save(void * call_ptr);

#endif /* #ifndef STUDIO_INTERNAL_H__B4CB73EC_8E89_401A_9E49_F0AEEF361D09__INCLUDED */
