/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 * DESCRIPTION:
 *  Helper functionality for accessing JACK through D-Bus
 *
 *****************************************************************************/

#include "jack_proxy.h"

bool
jack_proxy_init(
  void)
{
  return false;
}

void
jack_proxy_uninit(
  void)
{
}

bool
jack_proxy_is_started(
  bool * started_ptr)
{
  return false;
}

bool
jack_proxy_get_client_pid(
  uint64_t client_id,
  pid_t * pid_ptr)
{
  return false;
}

bool
jack_proxy_connect_ports(
  uint64_t port1_id,
  uint64_t port2_id)
{
  return false;
}
