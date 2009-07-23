/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 * DESCRIPTION:
 *  Helper functionality for accessing JACK through D-Bus
 *
 *****************************************************************************/

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
  char * child_buffer,
  size_t child_buffer_size,
  void * callback_context,
  bool (* callback)(void * context, bool leaf, const char * address, char * child));

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
