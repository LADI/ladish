/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010,2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the "change app state" command
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
#include "../proxies/notify_proxy.h"
#include "virtualizer.h"

struct ladish_command_change_app_state
{
  struct ladish_command command; /* must be the first member */
  char * opath;
  uint64_t id;
  unsigned int target_state;
  const char * target_state_description;
  bool (* run_target)(struct ladish_command_change_app_state *, ladish_app_supervisor_handle, ladish_app_handle);
  void (* initiate_stop)(ladish_app_handle app);
};

static bool run_target_start(struct ladish_command_change_app_state * cmd_ptr, ladish_app_supervisor_handle supervisor, ladish_app_handle app)
{
  ASSERT(cmd_ptr->command.state == LADISH_COMMAND_STATE_PENDING);
  ASSERT(cmd_ptr->initiate_stop == NULL);

  if (!ladish_studio_is_started())
  {
    log_error("cannot start app because studio is not started", cmd_ptr->opath);
    ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "Cannot start app because studio is not started", NULL);
    return false;
  }

  if (ladish_app_is_running(app))
  {
    log_error("App %s is already running", ladish_app_get_name(app));
    ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "Cannot start app because it is already running", NULL);
    return false;
  }

  if (!ladish_app_supervisor_start_app(supervisor, app))
  {
    ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "Cannot start app (execution failed)", NULL);
    return false;
  }

  cmd_ptr->command.state = LADISH_COMMAND_STATE_DONE;
  return true;
}

static bool run_target_stop(struct ladish_command_change_app_state * cmd_ptr, ladish_app_supervisor_handle supervisor, ladish_app_handle app)
{
  const char * app_name;
  uuid_t app_uuid;

  ASSERT(cmd_ptr->initiate_stop != NULL);

  app_name = ladish_app_get_name(app);
  ladish_app_get_uuid(app, app_uuid);

  if (ladish_app_is_running(app))
  {
    if (cmd_ptr->command.state == LADISH_COMMAND_STATE_PENDING)
    {
      cmd_ptr->initiate_stop(app);
      cmd_ptr->command.state = LADISH_COMMAND_STATE_WAITING;
      return true;
    }

    ASSERT(cmd_ptr->command.state == LADISH_COMMAND_STATE_WAITING);
    log_info("Waiting '%s' process termination (%s)...", app_name, cmd_ptr->target_state_description);
    return true;
  }

  if (!ladish_virtualizer_is_hidden_app(ladish_studio_get_jack_graph(), app_uuid, app_name))
  {
    log_info("Waiting '%s' client disappear (%s)...", app_name, cmd_ptr->target_state_description);
    return true;
  }

  if (cmd_ptr->command.state == LADISH_COMMAND_STATE_PENDING)
  {
    log_info("App %s is already stopped (%s)", app_name, cmd_ptr->target_state_description);
  }

  cmd_ptr->command.state = LADISH_COMMAND_STATE_DONE;
  return true;
}

#define cmd_ptr ((struct ladish_command_change_app_state *)context)

static bool run(void * context)
{
  ladish_app_supervisor_handle supervisor;
  ladish_app_handle app;

  if (cmd_ptr->command.state == LADISH_COMMAND_STATE_PENDING)
  {
    log_info("%s app command. opath='%s'", cmd_ptr->target_state_description, cmd_ptr->opath);
  }

  supervisor = ladish_studio_find_app_supervisor(cmd_ptr->opath);
  if (supervisor == NULL)
  {
    log_error("cannot find supervisor '%s' to %s app", cmd_ptr->opath, cmd_ptr->target_state_description);
    ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "Cannot change app state because of internal error (unknown supervisor)", NULL);
    return false;
  }

  app = ladish_app_supervisor_find_app_by_id(supervisor, cmd_ptr->id);
  if (app == NULL)
  {
    log_error("App with ID %"PRIu64" not found (%s)", cmd_ptr->id, cmd_ptr->target_state_description);
    ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "Cannot change app state because it is not found", NULL);
    return false;
  }

  return cmd_ptr->run_target(cmd_ptr, supervisor, app);
}

static void destructor(void * context)
{
  log_info("change_app_state command destructor");
  free(cmd_ptr->opath);
}

#undef cmd_ptr

bool ladish_command_change_app_state(void * call_ptr, struct ladish_cqueue * queue_ptr, const char * opath, uint64_t id, unsigned int target_state)
{
  struct ladish_command_change_app_state * cmd_ptr;
  char * opath_dup;

  opath_dup = strdup(opath);
  if (opath_dup == NULL)
  {
    cdbus_error(call_ptr, DBUS_ERROR_FAILED, "strdup('%s') failed.", opath);
    goto fail;
  }

  cmd_ptr = ladish_command_new(sizeof(struct ladish_command_change_app_state));
  if (cmd_ptr == NULL)
  {
    cdbus_error(call_ptr, DBUS_ERROR_FAILED, "ladish_command_new() failed.");
    goto fail_free_opath;
  }

  cmd_ptr->command.run = run;
  cmd_ptr->command.destructor = destructor;
  cmd_ptr->opath = opath_dup;
  cmd_ptr->id = id;
  cmd_ptr->target_state = target_state;

  switch (target_state)
  {
  case LADISH_APP_STATE_STARTED:
    cmd_ptr->target_state_description = "start";
    cmd_ptr->run_target = run_target_start;
    cmd_ptr->initiate_stop = NULL;
    break;
  case LADISH_APP_STATE_STOPPED:
    cmd_ptr->target_state_description = "stop";
    cmd_ptr->run_target = run_target_stop;
    cmd_ptr->initiate_stop = ladish_app_stop;
    break;
  case LADISH_APP_STATE_KILL:
    cmd_ptr->target_state_description = "kill";
    cmd_ptr->run_target = run_target_stop;
    cmd_ptr->initiate_stop = ladish_app_kill;
    break;
  default:
    ASSERT_NO_PASS;
    cdbus_error(call_ptr, DBUS_ERROR_FAILED, "Invalid target state (internal error).", opath);
    goto fail_destroy_command;
  }

  if (!ladish_cqueue_add_command(queue_ptr, &cmd_ptr->command))
  {
    cdbus_error(call_ptr, DBUS_ERROR_FAILED, "ladish_cqueue_add_command() failed.");
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
