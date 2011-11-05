/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009,2010,2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the "unload project" command
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
#include "room.h"
#include "studio.h"
#include "../proxies/notify_proxy.h"

struct ladish_command_unload_project
{
  struct ladish_command command;
  uuid_t room_uuid;
  ladish_room_handle room;
};

#define cmd_ptr ((struct ladish_command_unload_project *)command_context)

static bool run(void * command_context)
{
  if (cmd_ptr->command.state == LADISH_COMMAND_STATE_PENDING)
  {
    ASSERT(cmd_ptr->room == NULL);
    cmd_ptr->room = ladish_studio_find_room_by_uuid(cmd_ptr->room_uuid);
    if (cmd_ptr->room == NULL)
    {
      log_error("Cannot unload project of unknown room");
      ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "Cannot unload project of unknown room", NULL);
      return false;
    }

    if (ladish_room_unload_project(cmd_ptr->room))
    {
      cmd_ptr->command.state = LADISH_COMMAND_STATE_DONE;
    }
    else
    {
      cmd_ptr->command.state = LADISH_COMMAND_STATE_WAITING;
    }

    return true;
  }

  ASSERT(cmd_ptr->command.state == LADISH_COMMAND_STATE_WAITING);

  if (ladish_room_unload_project(cmd_ptr->room))
  {
    cmd_ptr->command.state = LADISH_COMMAND_STATE_DONE;
  }

  return true;
}

#undef cmd_ptr

bool
ladish_command_unload_project(
  void * call_ptr,
  struct ladish_cqueue * queue_ptr,
  const uuid_t room_uuid_ptr)
{
  struct ladish_command_unload_project * cmd_ptr;

  cmd_ptr = ladish_command_new(sizeof(struct ladish_command_unload_project));
  if (cmd_ptr == NULL)
  {
    log_error("ladish_command_new() failed.");
    goto fail;
  }

  cmd_ptr->command.run = run;
  uuid_copy(cmd_ptr->room_uuid, room_uuid_ptr);
  cmd_ptr->room = NULL;

  if (!ladish_cqueue_add_command(queue_ptr, &cmd_ptr->command))
  {
    cdbus_error(call_ptr, DBUS_ERROR_FAILED, "ladish_cqueue_add_command() failed.");
    goto fail_destroy_command;
  }

  return true;

fail_destroy_command:
  free(cmd_ptr);
fail:
  return false;
}
