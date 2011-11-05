/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009,2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the "deactivate daemon" command
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
  cmd_ptr->state = LADISH_COMMAND_STATE_DONE;
  g_quit = true;                /* cause main loop quit */
  return false;                 /* cause command queue clear  */
}

#undef cmd_ptr

bool ladish_command_exit(void * call_ptr, struct ladish_cqueue * queue_ptr)
{
  struct ladish_command * cmd_ptr;

  if (!ladish_command_unload_studio(call_ptr, queue_ptr))
  {
    goto fail;
  }

  cmd_ptr = ladish_command_new(sizeof(struct ladish_command));
  if (cmd_ptr == NULL)
  {
    log_error("ladish_command_new() failed.");
    goto fail_drop_unload_command;
  }

  cmd_ptr->run = run;

  if (!ladish_cqueue_add_command(queue_ptr, cmd_ptr))
  {
    cdbus_error(call_ptr, DBUS_ERROR_FAILED, "ladish_cqueue_add_command() failed.");
    goto fail_destroy_command;
  }

  return true;

fail_destroy_command:
  free(cmd_ptr);

fail_drop_unload_command:
  ladish_cqueue_drop_command(queue_ptr);

fail:
  return false;
}
