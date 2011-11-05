/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010,2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the "create room" command
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
#include "../proxies/notify_proxy.h"

struct ladish_command_create_room
{
  struct ladish_command command; /* must be the first member */
  char * room_name;
  char * template_name;
};

#define cmd_ptr ((struct ladish_command_create_room *)cmd_context)

static bool run(void * cmd_context)
{
  ladish_room_handle room;

  ASSERT(cmd_ptr->command.state == LADISH_COMMAND_STATE_PENDING);

  log_info("Request to create new studio room \"%s\" from template \"%s\".", cmd_ptr->room_name, cmd_ptr->template_name);

  room = find_room_template_by_name(cmd_ptr->template_name);
  if (room == NULL)
  {
    log_error("Unknown room template \"%s\"",  cmd_ptr->template_name);
    goto fail;
  }

  if (ladish_studio_check_room_name(cmd_ptr->room_name))
  {
    ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "Room with requested name already exists", cmd_ptr->room_name);
    goto fail;
  }

  if (!ladish_room_create(NULL, cmd_ptr->room_name, room, g_studio.studio_graph, &room))
  {
    log_error("ladish_room_create() failed.");
    goto fail;
  }

  if (ladish_studio_is_started())
  {
    if (!ladish_room_start(room, ladish_studio_get_virtualizer()))
    {
      log_error("ladish_room_start() failed");
      goto fail_destroy_room;
    }
  }

  cmd_ptr->command.state = LADISH_COMMAND_STATE_DONE;
  return true;

fail_destroy_room:
  ladish_room_destroy(room);
fail:
  return false;
}

static void destructor(void * cmd_context)
{
  log_info("create_room command destructor");
  free(cmd_ptr->room_name);
  free(cmd_ptr->template_name);
}

#undef cmd_ptr

bool ladish_command_create_room(void * call_ptr, struct ladish_cqueue * queue_ptr, const char * room_name, const char * template_name)
{
  struct ladish_command_create_room * cmd_ptr;
  char * room_name_dup;
  char * template_name_dup;

  room_name_dup = strdup(room_name);
  if (room_name_dup == NULL)
  {
    cdbus_error(call_ptr, DBUS_ERROR_FAILED, "strdup('%s') failed.", room_name);
    goto fail;
  }

  template_name_dup = strdup(template_name);
  if (template_name_dup == NULL)
  {
    cdbus_error(call_ptr, DBUS_ERROR_FAILED, "strdup('%s') failed.", template_name);
    goto fail_free_room_name;
  }

  cmd_ptr = ladish_command_new(sizeof(struct ladish_command_create_room));
  if (cmd_ptr == NULL)
  {
    cdbus_error(call_ptr, DBUS_ERROR_FAILED, "ladish_command_new() failed.");
    goto fail_free_template_name;
  }

  cmd_ptr->command.run = run;
  cmd_ptr->command.destructor = destructor;
  cmd_ptr->room_name = room_name_dup;
  cmd_ptr->template_name = template_name_dup;

  if (!ladish_cqueue_add_command(queue_ptr, &cmd_ptr->command))
  {
    cdbus_error(call_ptr, DBUS_ERROR_FAILED, "ladish_cqueue_add_command() failed.");
    goto fail_destroy_command;
  }

  return true;

fail_destroy_command:
  free(cmd_ptr);
fail_free_template_name:
  free(room_name_dup);
fail_free_room_name:
  free(room_name_dup);
fail:
  return false;
}
