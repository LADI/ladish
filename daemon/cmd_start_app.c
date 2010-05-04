/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the "start app" command
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
#include "../proxies/notify_proxy.h"

struct ladish_command_start_app
{
  struct ladish_command command; /* must be the first member */
  char * opath;
  uint64_t id;
};

#define cmd_ptr ((struct ladish_command_start_app *)context)

static bool run(void * context)
{
  ladish_app_supervisor_handle supervisor;
  ladish_app_handle app;

  ASSERT(cmd_ptr->command.state == LADISH_COMMAND_STATE_PENDING);

  log_info("start_app command. opath='%s'", cmd_ptr->opath);

  if (!ladish_studio_is_started())
  {
    log_error("cannot start app because studio is not started", cmd_ptr->opath);
    ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "Cannot start app because studio is not started", NULL);
    return false;
  }

  supervisor = ladish_studio_find_app_supervisor(cmd_ptr->opath);
  if (supervisor == NULL)
  {
    log_error("cannot find supervisor '%s' to start app", cmd_ptr->opath);
    ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "Cannot start app because of internal error (unknown supervisor)", NULL);
    return false;
  }

  app = ladish_app_supervisor_find_app_by_id(supervisor, cmd_ptr->id);
  if (app == NULL)
  {
    log_error("App with ID %"PRIu64" not found", cmd_ptr->id);
    ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "Cannot start app because it is not found", NULL);
    return false;
  }

  if (ladish_app_is_running(app))
  {
    log_error("App %s is already running", ladish_app_get_name(app));
    ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "Cannot start app because it is already running", NULL);
    return false;
  }

  if (!ladish_app_supervisor_run(supervisor, app))
  {
    ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "Cannot start app (execution failed)", NULL);
    return false;
  }

  cmd_ptr->command.state = LADISH_COMMAND_STATE_DONE;
  return true;
}

static void destructor(void * context)
{
  log_info("start_app command destructor");
  free(cmd_ptr->opath);
}

#undef cmd_ptr

bool ladish_command_start_app(void * call_ptr, struct ladish_cqueue * queue_ptr, const char * opath, uint64_t id)
{
  struct ladish_command_start_app * cmd_ptr;
  char * opath_dup;

  opath_dup = strdup(opath);
  if (opath_dup == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup('%s') failed.", opath_dup);
    goto fail;
  }

  cmd_ptr = ladish_command_new(sizeof(struct ladish_command_start_app));
  if (cmd_ptr == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "ladish_command_new() failed.");
    goto fail_free_opath;
  }

  cmd_ptr->command.run = run;
  cmd_ptr->command.destructor = destructor;
  cmd_ptr->opath = opath_dup;
  cmd_ptr->id = id;

  if (!ladish_cqueue_add_command(queue_ptr, &cmd_ptr->command))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "ladish_cqueue_add_command() failed.");
    goto fail_destroy_command;
  }

  return true;

fail_destroy_command:
  free(cmd_ptr);
fail_free_opath:
  free(opath_dup);
fail:
  return false;
}
