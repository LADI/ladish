/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010,2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the "remove app" command
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

struct ladish_command_remove_app
{
  struct ladish_command command; /* must be the first member */
  char * opath;
  uint64_t id;
};

#define cmd_ptr ((struct ladish_command_remove_app *)context)

static bool run(void * context)
{
  ladish_app_supervisor_handle supervisor;
  ladish_app_handle app;
  const char * app_name;
  uuid_t app_uuid;

  ASSERT(cmd_ptr->command.state == LADISH_COMMAND_STATE_PENDING);

  log_info("remove app command. opath='%s'", cmd_ptr->opath);

  supervisor = ladish_studio_find_app_supervisor(cmd_ptr->opath);
  if (supervisor == NULL)
  {
    log_error("cannot find supervisor '%s' to remove app from", cmd_ptr->opath);
    ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "Cannot remove app because of internal error (unknown supervisor)", NULL);
    return false;
  }

  app = ladish_app_supervisor_find_app_by_id(supervisor, cmd_ptr->id);
  if (app == NULL)
  {
    log_error("App with ID %"PRIu64" not found and (remove)", cmd_ptr->id);
    ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "Cannot remove app because it is not found", NULL);
    return false;
  }

  app_name = ladish_app_get_name(app);
  ladish_app_get_uuid(app, app_uuid);

  if (ladish_app_is_running(app))
  {
    if (cmd_ptr->command.state == LADISH_COMMAND_STATE_PENDING)
    {
      log_info("App %s is not stopped as it should", app_name);
      ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "Cannot remove app because it is not stopped (because of internal error)", NULL);
    }
    return false;
  }

  /* remove jclient and vclient for this app */
  log_info("Removing graph objects for app '%s'", app_name);
  ladish_virtualizer_remove_app(ladish_studio_get_jack_graph(), app_uuid, app_name);

  ladish_app_supervisor_remove_app(supervisor, app);
  cmd_ptr->command.state = LADISH_COMMAND_STATE_DONE;
  return true;
}

static void destructor(void * context)
{
  log_info("remove_app command destructor");
  free(cmd_ptr->opath);
}

#undef cmd_ptr

bool ladish_command_remove_app(void * call_ptr, struct ladish_cqueue * queue_ptr, const char * opath, uint64_t id)
{
  struct ladish_command_remove_app * cmd_ptr;
  char * opath_dup;

  if (!ladish_command_change_app_state(call_ptr, queue_ptr, opath, id, LADISH_APP_STATE_STOPPED))
  {
    goto fail;
  }

  opath_dup = strdup(opath);
  if (opath_dup == NULL)
  {
    cdbus_error(call_ptr, DBUS_ERROR_FAILED, "strdup('%s') failed.", opath);
    goto fail_drop_stop_command;
  }

  cmd_ptr = ladish_command_new(sizeof(struct ladish_command_remove_app));
  if (cmd_ptr == NULL)
  {
    cdbus_error(call_ptr, DBUS_ERROR_FAILED, "ladish_command_new() failed.");
    goto fail_free_opath;
  }

  cmd_ptr->command.run = run;
  cmd_ptr->command.destructor = destructor;
  cmd_ptr->opath = opath_dup;
  cmd_ptr->id = id;

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
fail_drop_stop_command:
  ladish_cqueue_drop_command(queue_ptr);
fail:
  return false;
}
