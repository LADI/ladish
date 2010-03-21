/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the "unload studio" command
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

#include "cmd.h"
#include "studio_internal.h"
#include "control.h"

#define cmd_ptr ((struct ladish_command *)context)

static bool run(void * context)
{
  ASSERT(cmd_ptr->state == LADISH_COMMAND_STATE_PENDING);

  ladish_graph_dump(g_studio.studio_graph);
  ladish_graph_dump(g_studio.jack_graph);
  ladish_graph_clear(g_studio.studio_graph, false);
  ladish_graph_clear(g_studio.jack_graph, true);

  studio_remove_all_rooms();

  jack_conf_clear();

  g_studio.modified = false;
  g_studio.persisted = false;

  if (g_studio.dbus_object != NULL)
  {
    dbus_object_path_destroy(g_dbus_connection, g_studio.dbus_object);
    g_studio.dbus_object = NULL;
    emit_studio_disappeared();
  }

  if (g_studio.name != NULL)
  {
    free(g_studio.name);
    g_studio.name = NULL;
  }

  if (g_studio.filename != NULL)
  {
    free(g_studio.filename);
    g_studio.filename = NULL;
  }

  ladish_app_supervisor_clear(g_studio.app_supervisor);

  cmd_ptr->state = LADISH_COMMAND_STATE_DONE;
  return true;
}

#undef cmd_ptr

bool ladish_command_unload_studio(void * call_ptr, struct ladish_cqueue * queue_ptr)
{
  struct ladish_command * cmd_ptr;

  if (!ladish_command_stop_studio(call_ptr, queue_ptr))
  {
    goto fail;
  }

  cmd_ptr = ladish_command_new(sizeof(struct ladish_command));
  if (cmd_ptr == NULL)
  {
    log_error("ladish_command_new() failed.");
    goto fail_drop_stop_command;
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

fail_drop_stop_command:
  ladish_cqueue_drop_command(queue_ptr);

fail:
  return false;
}
