/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to the helper functionality for accessing
 * JACK through D-Bus
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

#ifndef JACK_PROXY_H__88702EEC_4B82_407F_A664_AD70C1E14D02__INCLUDED
#define JACK_PROXY_H__88702EEC_4B82_407F_A664_AD70C1E14D02__INCLUDED

#include "common.h"

bool
jack_proxy_init(
  void);

void
jack_proxy_uninit(
  void);

bool
jack_proxy_is_started(
  bool * started_ptr);

bool
jack_proxy_get_client_pid(
  uint64_t client_id,
  pid_t * pid_ptr);

bool
jack_proxy_connect_ports(
  uint64_t port1_id,
  uint64_t port2_id);

bool
jack_proxy_read_conf_container(
  const char * address,
  void * callback_context,
  bool (* callback)(void * context, bool leaf, const char * address, char * child));

bool
jack_proxy_get_parameter_value(
  const char * address,
  bool * is_set_ptr,
  struct jack_parameter_variant * parameter_ptr);

void
on_jack_client_appeared(
  uint64_t client_id,
  const char * client_name);

void
on_jack_client_disappeared(
  uint64_t client_id);

void
on_jack_port_appeared(
  uint64_t client_id,
  uint64_t port_id,
  const char * port_name);

void
on_jack_port_disappeared(
  uint64_t client_id,
  uint64_t port_id,
  const char * port_name);

void
on_jack_ports_connected(
  uint64_t client1_id,
  uint64_t port1_id,
  uint64_t client2_id,
  uint64_t port2_id);

void
on_jack_ports_disconnected(
  uint64_t client1_id,
  uint64_t port1_id,
  uint64_t client2_id,
  uint64_t port2_id);

void
on_jack_server_started(
  void);

void
on_jack_server_stopped(
  void);

void
on_jack_server_appeared(
  void);

void
on_jack_server_disappeared(
  void);

#endif /* #ifndef JACK_PROXY_H__88702EEC_4B82_407F_A664_AD70C1E14D02__INCLUDED */
