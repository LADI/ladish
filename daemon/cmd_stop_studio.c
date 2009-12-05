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

#define cmd_ptr ((struct ladish_command *)context)

static bool run(void * context)
{
  bool jack_server_started;

  switch (cmd_ptr->state)
  {
  case LADISH_COMMAND_STATE_PENDING:
    if (!studio_is_started())
    {
      /* nothing to do, studio is not running */
      cmd_ptr->state = LADISH_COMMAND_STATE_DONE;
      return true;
    }

    ladish_app_supervisor_stop(g_studio.app_supervisor);

    usleep(3000000);

    log_info("Stopping JACK server...");

    ladish_graph_dump(g_studio.studio_graph);

    if (!jack_proxy_stop_server())
    {
      log_error("Stopping JACK server failed.");
      return false;
    }

    cmd_ptr->state = LADISH_COMMAND_STATE_WAITING;
    /* fall through */
  case LADISH_COMMAND_STATE_WAITING:
    if (!ladish_environment_consume_change(&g_studio.env_store, ladish_environment_jack_server_started, &jack_server_started))
    {
      /* we are still waiting for the JACK server stop */
      ASSERT(ladish_environment_get(&g_studio.env_store, ladish_environment_jack_server_started)); /* someone else consumed the state change? */
      return true;
    }

    log_info("Wait for JACK server stop complete.");

    ASSERT(!jack_server_started);

    ladish_graph_dump(g_studio.studio_graph);

    on_event_jack_stopped();

    cmd_ptr->state = LADISH_COMMAND_STATE_DONE;
    return true;
  }

  ASSERT_NO_PASS;
  return false;
}

#undef cmd_ptr

bool ladish_command_stop_studio(void * call_ptr, struct ladish_cqueue * queue_ptr)
{
  struct ladish_command * cmd_ptr;

  cmd_ptr = ladish_command_new(sizeof(struct ladish_command));
  if (cmd_ptr == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "ladish_command_new() failed.");
    goto fail;
  }

  cmd_ptr->run = run;

  if (!ladish_cqueue_add_command(queue_ptr, cmd_ptr))
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
