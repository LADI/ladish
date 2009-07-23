/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 * DESCRIPTION:
 *  JACK server monitor and control
 *
 *****************************************************************************/

#include "jack.h"
#include "jack_proxy.h"

bool
jack_init(
  void)
{
  if (!jack_proxy_init())
  {
    return false;
  }

  return true;
}

void
jack_uninit(
  void)
{
  jack_proxy_uninit();
}

void
on_jack_client_appeared(
  uint64_t client_id,
  const char * client_name)
{
  lash_info("JACK client appeared.");
}

void
on_jack_client_disappeared(
  uint64_t client_id)
{
  lash_info("JACK client disappeared.");
}

void
on_jack_port_appeared(
  uint64_t client_id,
  uint64_t port_id,
  const char * port_name)
{
  lash_info("JACK port appeared.");
}

void
on_jack_port_disappeared(
  uint64_t client_id,
  uint64_t port_id,
  const char * port_name)
{
  lash_info("JACK port disappeared.");
}

void
on_jack_ports_connected(
  uint64_t client1_id,
  uint64_t port1_id,
  uint64_t client2_id,
  uint64_t port2_id)
{
  lash_info("JACK ports connected.");
}

void
on_jack_ports_disconnected(
  uint64_t client1_id,
  uint64_t port1_id,
  uint64_t client2_id,
  uint64_t port2_id)
{
  lash_info("JACK ports disconnected.");
}

void
on_jack_server_started(
  void)
{
  lash_info("JACK server start detected.");
}

void
on_jack_server_stopped(
  void)
{
  lash_info("JACK server stop detected.");
}

void
on_jack_server_appeared(
  void)
{
  lash_info("JACK controller appeared.");
}

void
on_jack_server_disappeared(
  void)
{
  lash_info("JACK controller disappeared.");
}
