/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009,2010,2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the "load project" command
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

#include "cmd.h"
#include "room.h"
#include "studio.h"
#include "../proxies/notify_proxy.h"

struct ladish_command_load_project
{
  struct ladish_command command;
  uuid_t room_uuid;
  char * project_dir;
};

#define cmd_ptr ((struct ladish_command_load_project *)command_context)

static bool run(void * command_context)
{
  ladish_room_handle room;

  room = ladish_studio_find_room_by_uuid(cmd_ptr->room_uuid);
  if (room == NULL)
  {
    log_error("Cannot load project in unknown room");
    ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "Cannot load project in unknown room", NULL);
    return false;
  }

  if (!ladish_room_load_project(room, cmd_ptr->project_dir))
  {
    log_error("Project load failed.");
    return false;
  }

  ladish_notify_simple(LADISH_NOTIFY_URGENCY_NORMAL, "Project loaded successfully.", NULL);
  cmd_ptr->command.state = LADISH_COMMAND_STATE_DONE;

  return true;
}

static void destructor(void * command_context)
{
  log_info("load project command destructor");
  free(cmd_ptr->project_dir);
}

#undef cmd_ptr

bool
ladish_command_load_project(
  void * call_ptr,
  struct ladish_cqueue * queue_ptr,
  const uuid_t room_uuid_ptr,
  const char * project_dir)
{
  char * project_dir_dup;
  struct ladish_command_load_project * cmd_ptr;

  if (!ladish_command_unload_project(call_ptr, queue_ptr, room_uuid_ptr))
  {
    goto fail;
  }

  project_dir_dup = strdup(project_dir);
  if (project_dir_dup == NULL)
  {
    cdbus_error(call_ptr, DBUS_ERROR_FAILED, "strdup('%s') failed.", project_dir);
    goto fail_drop_unload_command;
  }

  cmd_ptr = ladish_command_new(sizeof(struct ladish_command_load_project));
  if (cmd_ptr == NULL)
  {
    log_error("ladish_command_new() failed.");
    goto fail_free_dir;
  }

  cmd_ptr->command.run = run;
  cmd_ptr->command.destructor = destructor;
  uuid_copy(cmd_ptr->room_uuid, room_uuid_ptr);
  cmd_ptr->project_dir = project_dir_dup;

  if (!ladish_cqueue_add_command(queue_ptr, &cmd_ptr->command))
  {
    cdbus_error(call_ptr, DBUS_ERROR_FAILED, "ladish_cqueue_add_command() failed.");
    goto fail_destroy_command;
  }

  return true;

fail_destroy_command:
  free(cmd_ptr);
fail_free_dir:
  free(project_dir_dup);
fail_drop_unload_command:
  ladish_cqueue_drop_command(queue_ptr);
fail:
  return false;
}
