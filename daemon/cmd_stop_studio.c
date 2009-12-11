/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the "stop studio" command
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

#include <unistd.h>

#include "cmd.h"
#include "studio_internal.h"

#define STOP_STATE_WAITING_FOR_JACK_CLIENTS_DISAPPEAR   1
#define STOP_STATE_WAITING_FOR_JACK_SERVER_STOP         2

struct ladish_command_stop_studio
{
  struct ladish_command command;
  int stop_waits;
  unsigned int stop_state;
};

#define cmd_ptr ((struct ladish_command_stop_studio *)context)

static bool run(void * context)
{
  bool jack_server_started;
  unsigned int clients_count;

  switch (cmd_ptr->command.state)
  {
  case LADISH_COMMAND_STATE_PENDING:
    if (!studio_is_started())
    {
      log_info("Ignoring stop request because studio is already stopped.");
      /* nothing to do, studio is not running */
      cmd_ptr->command.state = LADISH_COMMAND_STATE_DONE;
      return true;
    }

    ladish_graph_dump(g_studio.jack_graph);
    ladish_graph_dump(g_studio.studio_graph);

    ladish_app_supervisor_stop(g_studio.app_supervisor);

    cmd_ptr->command.state = LADISH_COMMAND_STATE_WAITING;
    cmd_ptr->stop_state = STOP_STATE_WAITING_FOR_JACK_CLIENTS_DISAPPEAR;
    /* fall through */
  case LADISH_COMMAND_STATE_WAITING:
    if (cmd_ptr->stop_state == STOP_STATE_WAITING_FOR_JACK_CLIENTS_DISAPPEAR)
    {
      clients_count = ladish_virtualizer_get_our_clients_count(g_studio.virtualizer);
      log_info("%u JACK clients started by ladish are visible", clients_count);
      if (clients_count != 0)
      {
        return true;
      }

      log_info("Stopping JACK server...");

      ladish_graph_dump(g_studio.studio_graph);

      cmd_ptr->stop_state = STOP_STATE_WAITING_FOR_JACK_SERVER_STOP;

      if (!jack_proxy_stop_server())
      {
        log_error("Stopping JACK server failed. Waiting stop for 5 seconds anyway...");

        /* JACK server stop sometimes fail, even if it actually succeeds after some time */
        /* Reproducable with yoshimi-0.0.45 */

        cmd_ptr->stop_waits = 5000;
        return true;
      }
    }

    ASSERT(cmd_ptr->stop_state == STOP_STATE_WAITING_FOR_JACK_SERVER_STOP);

    if (cmd_ptr->stop_waits > 0)
    {
      if (!jack_proxy_is_started(&jack_server_started))
      {
        log_error("jack_proxy_is_started() failed.");
        return false;
      }

      if (!jack_server_started)
      {
        ladish_environment_reset_stealth(&g_studio.env_store, ladish_environment_jack_server_started);
        goto done;
      }

      if (cmd_ptr->stop_waits == 1)
      {
        log_error("JACK server stop wait after stop request expired.");
        return false;
      }

      cmd_ptr->stop_waits--;
      usleep(1000);
      return true;
    }

    if (!ladish_environment_consume_change(&g_studio.env_store, ladish_environment_jack_server_started, &jack_server_started))
    {
      /* we are still waiting for the JACK server stop */
      ASSERT(ladish_environment_get(&g_studio.env_store, ladish_environment_jack_server_started)); /* someone else consumed the state change? */
      return true;
    }

  done:
    log_info("Wait for JACK server stop complete.");

    ladish_graph_hide_all(g_studio.jack_graph);
    ladish_graph_hide_all(g_studio.studio_graph);
    ASSERT(!jack_server_started);

    ladish_graph_dump(g_studio.studio_graph);

    on_event_jack_stopped();

    cmd_ptr->command.state = LADISH_COMMAND_STATE_DONE;
    return true;
  }

  ASSERT_NO_PASS;
  return false;
}

#undef cmd_ptr

bool ladish_command_stop_studio(void * call_ptr, struct ladish_cqueue * queue_ptr)
{
  struct ladish_command_stop_studio * cmd_ptr;

  cmd_ptr = ladish_command_new(sizeof(struct ladish_command_stop_studio));
  if (cmd_ptr == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "ladish_command_new() failed.");
    goto fail;
  }

  cmd_ptr->command.run = run;
  cmd_ptr->stop_waits = -1;

  if (!ladish_cqueue_add_command(queue_ptr, &cmd_ptr->command))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "ladish_cqueue_add_command() failed.");
    goto fail_destroy_command;
  }

  return true;

fail_destroy_command:
  free(cmd_ptr);

fail:
  return false;
}
