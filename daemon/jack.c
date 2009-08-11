/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains code for JACK server monitor and control
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

#include "jack.h"
#include "../jack_proxy.h"
#include "studio.h"
#include "dbus_iface_control.h"

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

  if (g_studio == NULL)
  {
    if (!studio_create(&g_studio))
    {
      lash_error("failed to create studio object");
      return;
    }
  }

  if (!studio_fetch_jack_settings(g_studio))
  {
    lash_error("studio_fetch_jack_settings() failed.");

    if (!studio_is_persisted(g_studio))
    {
      emit_studio_disappeared();
      studio_destroy(g_studio);
      g_studio = NULL;
      return;
    }
  }

  lash_info("jack conf successfully retrieved");
  studio_activate(g_studio);
  emit_studio_appeared();
  studio_save(g_studio, NULL);  /* XXX - to test save functionality, we should not save unless requested by user */
  return;
}

void
on_jack_server_stopped(
  void)
{
  lash_info("JACK server stop detected.");

  if (g_studio == NULL)
  {
    return;
  }

  if (!studio_is_persisted(g_studio))
  {
    emit_studio_disappeared();
    studio_destroy(g_studio);
    g_studio = NULL;
    return;
  }

  /* TODO: if user wants, restart jack server and reconnect all jack apps to it */
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

bool
jack_init(
  void)
{
  bool started;

  if (!jack_proxy_init(
        on_jack_client_appeared,
        on_jack_client_disappeared,
        on_jack_port_appeared,
        on_jack_port_disappeared,
        on_jack_ports_connected,
        on_jack_ports_disconnected,
        on_jack_server_started,
        on_jack_server_stopped,
        on_jack_server_appeared,
        on_jack_server_disappeared))
  {
    return false;
  }

  if (jack_proxy_is_started(&started) && started)
  {
    on_jack_server_started();
  }

  return true;
}

void
jack_uninit(
  void)
{
  jack_proxy_uninit();
}
