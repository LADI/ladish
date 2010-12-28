/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the "new app" command
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

#include <ctype.h>
#include "cmd.h"
#include "studio.h"
#include "../dbus/error.h"

struct ladish_command_new_app
{
  struct ladish_command command; /* must be the first member */
  char * opath;
  bool terminal;
  char * commandline;
  char * name;
  uint8_t level;
};

#define cmd_ptr ((struct ladish_command_new_app *)context)

static bool run(void * context)
{
  char * name;
  char * name_buffer;
  size_t len;
  char * end;
  unsigned int index;
  ladish_app_supervisor_handle supervisor;
  ladish_app_handle app;

  ASSERT(cmd_ptr->command.state == LADISH_COMMAND_STATE_PENDING);

  log_info("new_app command. opath='%s', name='%s', %s, commandline='%s', level=%"PRIu8") called", cmd_ptr->opath, cmd_ptr->name, cmd_ptr->terminal ? "terminal" : "shell", cmd_ptr->commandline, cmd_ptr->level);

  supervisor = ladish_studio_find_app_supervisor(cmd_ptr->opath);
  if (supervisor == NULL)
  {
    log_error("cannot find supervisor '%s' to run app '%s'", cmd_ptr->opath, cmd_ptr->name);
    return false;
  }

  if (*cmd_ptr->name)
  {
    /* allocate and copy app name */
    len = strlen(cmd_ptr->name);
    name_buffer = malloc(len + 100);
    if (name_buffer == NULL)
    {
      log_error("malloc of app name failed");
      return false;
    }

    name = name_buffer;

    strcpy(name, cmd_ptr->name);

    end = name + len;
  }
  else
  {
    /* allocate app name */
    len = strlen(cmd_ptr->commandline) + 100;
    name_buffer = malloc(len);
    if (name_buffer == NULL)
    {
      log_error("malloc of app name failed");
      return false;
    }

    strcpy(name_buffer, cmd_ptr->commandline);

    /* use first word as name */
    end = name_buffer;
    while (*end)
    {
      if (isspace(*end))
      {
        *end = 0;
        break;
      }

      end++;
    }

    name = strrchr(name_buffer, '/');
    if (name == NULL)
    {
      name = name_buffer;
    }
    else
    {
      name++;
    }
  }

  /* make the app name unique */
  index = 2;
  while (ladish_app_supervisor_find_app_by_name(supervisor, name) != NULL)
  {
    sprintf(end, "-%u", index);
    index++;
  }

  app = ladish_app_supervisor_add(supervisor, name, NULL, true, cmd_ptr->commandline, cmd_ptr->terminal, cmd_ptr->level);

  free(name_buffer);

  if (app == NULL)
  {
    log_error("ladish_app_supervisor_add() failed");
    return false;
  }

  if (ladish_studio_is_started())
  {
    if (!ladish_app_supervisor_start_app(supervisor, app))
    {
      log_error("Execution of '%s' failed",  cmd_ptr->commandline);
      ladish_app_supervisor_remove_app(supervisor, app);
      return false;
    }
  }

  cmd_ptr->command.state = LADISH_COMMAND_STATE_DONE;
  return true;
}

static void destructor(void * context)
{
  log_info("new_app command destructor");
  free(cmd_ptr->opath);
  free(cmd_ptr->commandline);
  free(cmd_ptr->name);
}

#undef cmd_ptr

bool
ladish_command_new_app(
  void * call_ptr,
  struct ladish_cqueue * queue_ptr,
  const char * opath,
  bool terminal,
  const char * commandline,
  const char * name,
  uint8_t level)
{
  struct ladish_command_new_app * cmd_ptr;
  char * opath_dup;
  char * commandline_dup;
  char * name_dup;

  opath_dup = strdup(opath);
  if (opath_dup == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup('%s') failed.", opath);
    goto fail;
  }

  commandline_dup = strdup(commandline);
  if (commandline_dup == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup('%s') failed.", commandline);
    goto fail_free_opath;
  }

  name_dup = strdup(name);
  if (name_dup == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup('%s') failed.", name);
    goto fail_free_commandline;
  }

  cmd_ptr = ladish_command_new(sizeof(struct ladish_command_new_app));
  if (cmd_ptr == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "ladish_command_new() failed.");
    goto fail_free_name;
  }

  cmd_ptr->command.run = run;
  cmd_ptr->command.destructor = destructor;
  cmd_ptr->opath = opath_dup;
  cmd_ptr->commandline = commandline_dup;
  cmd_ptr->name = name_dup;
  cmd_ptr->terminal = terminal;
  cmd_ptr->level = level;

  if (!ladish_cqueue_add_command(queue_ptr, &cmd_ptr->command))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "ladish_cqueue_add_command() failed.");
    goto fail_destroy_command;
  }

  return true;

fail_destroy_command:
  free(cmd_ptr);
fail_free_name:
  free(name_dup);
fail_free_commandline:
  free(commandline_dup);
fail_free_opath:
  free(opath_dup);
fail:
  return false;
}
