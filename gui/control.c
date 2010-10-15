/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains code related to the ladishd control object
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

#include "internal.h"
#include "studio.h"
#include "../proxies/control_proxy.h"
#include "../proxies/studio_proxy.h"
#include "world_tree.h"
#include "ask_dialog.h"

static guint g_ladishd_poll_source_tag;

static gboolean poll_ladishd(gpointer data)
{
  control_proxy_ping();
  return TRUE;
}

void control_proxy_on_daemon_appeared(void)
{
  if (get_studio_state() == STUDIO_STATE_NA || get_studio_state() == STUDIO_STATE_SICK)
  {
    log_info("ladishd appeared");
    g_source_remove(g_ladishd_poll_source_tag);
  }

  set_studio_state(STUDIO_STATE_UNLOADED);
  studio_state_changed(NULL);
}

void control_proxy_on_daemon_disappeared(bool clean_exit)
{
  log_info("ladishd disappeared");

  if (!clean_exit)
  {
    error_message_box("ladish daemon crashed");
    set_studio_state(STUDIO_STATE_SICK);
  }
  else
  {
    set_studio_state(STUDIO_STATE_NA);
  }

  studio_state_changed(NULL);

  if (studio_loaded())
  {
    destroy_studio_view();
  }

  world_tree_destroy_room_views();

  g_ladishd_poll_source_tag = g_timeout_add(500, poll_ladishd, NULL);
}

void control_proxy_on_studio_appeared(bool initial)
{
  char * name;
  bool started;

  set_studio_state(STUDIO_STATE_STOPPED);

  if (initial)
  {
    if (!studio_proxy_is_started(&started))
    {
      log_error("intially, studio is present but is_started() check failed.");
      return;
    }

    if (started)
    {
      set_studio_state(STUDIO_STATE_STARTED);
    }
  }

  if (studio_state_changed(&name))
  {
    if (studio_loaded())
    {
      log_error("studio appear signal received but studio already exists");
    }
    else
    {
      create_studio_view(name);
    }

    free(name);
  }
}

void control_proxy_on_studio_disappeared(void)
{
  set_studio_state(STUDIO_STATE_UNLOADED);
  studio_state_changed(NULL);

  if (!studio_loaded())
  {
    log_error("studio disappear signal received but studio does not exists");
    return;
  }

  destroy_studio_view();
}

void menu_request_daemon_exit(void)
{
  log_info("Daemon exit request");

  if (!control_proxy_exit())
  {
    error_message_box("Daemon exit command failed, please inspect logs.");
  }
}

void menu_request_new_studio(void)
{
  char * new_name;

  log_info("new studio request");

  if (name_dialog("New studio", "Studio name", "", &new_name))
  {
    if (!control_proxy_new_studio(new_name))
    {
      error_message_box("Creation of new studio failed, please inspect logs.");
    }

    free(new_name);
  }
}

void on_load_studio(const char * studio_name)
{
  log_info("Load studio \"%s\"", studio_name);

  if (!control_proxy_load_studio(studio_name))
  {
    error_message_box("Studio load failed, please inspect logs.");
  }
}

void on_delete_studio(const char * studio_name)
{
  bool result;

  if (!ask_dialog(&result, "<b><big>Confirm studio delete</big></b>", "Studio \"%s\" will be deleted. Are you sure?", studio_name) || !result)
  {
    return;
  }

  log_info("Delete studio \"%s\"", studio_name);

  if (!control_proxy_delete_studio(studio_name))
  {
    error_message_box("Studio delete failed, please inspect logs.");
  }
}
