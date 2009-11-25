/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the "rename studio" command
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

struct ladish_command_rename_studio
{
  struct ladish_command command; /* must be the first member */
  char * studio_name;
};

#define cmd_ptr ((struct ladish_command_rename_studio *)context)

static bool run(void * context)
{
  ASSERT(cmd_ptr->command.state == LADISH_COMMAND_STATE_PENDING);

  if (g_studio.name == NULL)
  {
    log_error("cannot rename not loaded studio");
    return false;
  }

  free(g_studio.name);
  g_studio.name = cmd_ptr->studio_name;
  cmd_ptr->studio_name = NULL;
  emit_studio_renamed();
  cmd_ptr->command.state = LADISH_COMMAND_STATE_DONE;
  return true;
}

static void destructor(void * context)
{
  log_info("rename studio command destructor");
  if (cmd_ptr->studio_name != NULL)
  {
    free(cmd_ptr->studio_name);
  }
}

#undef cmd_ptr

bool ladish_command_rename_studio(void * call_ptr, struct ladish_cqueue * queue_ptr, const char * studio_name)
{
  struct ladish_command_rename_studio * cmd_ptr;
  char * studio_name_dup;

  studio_name_dup = strdup(studio_name);
  if (studio_name_dup == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup('%s') failed.", studio_name);
    goto fail;
  }

  cmd_ptr = ladish_command_new(sizeof(struct ladish_command_rename_studio));
  if (cmd_ptr == NULL)
  {
    log_error("ladish_command_new() failed.");
    goto fail_free_name;
  }

  cmd_ptr->command.run = run;
  cmd_ptr->command.destructor = destructor;
  cmd_ptr->studio_name = studio_name_dup;

  if (!ladish_cqueue_add_command(queue_ptr, &cmd_ptr->command))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "ladish_cqueue_add_command() failed.");
    goto fail_destroy_command;
  }

  return true;

fail_destroy_command:
  free(cmd_ptr);

fail_free_name:
  free(studio_name_dup);

fail:
  return false;
}
