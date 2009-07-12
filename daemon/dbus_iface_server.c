/* -*- Mode: C -*- */
/*
 *   LASH
 *
 *   Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

#include "server.h"
#include "client.h"
#include "project.h"
#include "../common/safety.h"
#include "../common/debug.h"
#include "../common/klist.h"
#include "../dbus/interface.h"
#include "../dbus/error.h"
#include "store.h"

static void
lashd_dbus_ping(method_call_t *call)
{
  lash_debug("Pong");
  call->reply = dbus_message_new_method_return(call->message);
}

static void
lashd_dbus_connect(method_call_t *call)
{
  DBusError err;
  dbus_int32_t pid;
  const char *sender, *class, *id_str, *wd, *client_name, *project_name, *data_path;
  dbus_int32_t flags;
  int argc;
  char **argv;
  struct lash_client *client;

  sender = dbus_message_get_sender(call->message);
  if (!sender) {
    lash_error("Sender is NULL");
    return;
  }

  lash_debug("Received connection request from %s", sender);

  dbus_error_init(&err);

  if (!dbus_message_get_args(call->message, &err,
                             DBUS_TYPE_INT32, &pid,
                             DBUS_TYPE_STRING, &class,
                             DBUS_TYPE_INT32, &flags,
                             DBUS_TYPE_STRING, &wd,
                             DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &argv, &argc,
                             DBUS_TYPE_INVALID)) {
    lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_ARGS,
                    "Invalid arguments to method \"%s\": %s",
                    call->method_name, err.message);
    dbus_error_free(&err);
    return;
  }
#ifdef LASH_DEBUG
  char **ptr;
  for (ptr = argv; *ptr; ptr++)
    lash_debug("arg: %s", *ptr);
#endif

  lash_debug("Connected client PID is %u", pid);

  client = server_add_client(sender, (pid_t) pid, class,
                             (int) flags, wd, argc, argv);
  if (client == NULL)
  {
    lash_dbus_error(call, LASH_DBUS_ERROR_GENERIC, "server_add_client() failed");
    return;
  }

  id_str = (const char *) client->id_str;
  client_name = client->name ? client->name : "";
  project_name = client->project ? client->project->name : "";
  data_path = client->data_path ? client->data_path : "";
  flags = client->flags;
  wd = client->working_dir ? client->working_dir : "";

  method_return_new_valist(call,
                           DBUS_TYPE_STRING, &id_str,
                           DBUS_TYPE_STRING, &client_name,
                           DBUS_TYPE_STRING, &project_name,
                           DBUS_TYPE_STRING, &data_path,
                           DBUS_TYPE_INT32, &flags,
                           DBUS_TYPE_STRING, &wd,
                           DBUS_TYPE_INVALID);
}

static bool
get_message_sender(method_call_t  *call,
                   const char    **sender,
                   struct lash_client      **client)
{
  *sender = dbus_message_get_sender(call->message);
  if (!*sender) {
    lash_dbus_error(call, LASH_DBUS_ERROR_GENERIC,
                    "Sender is NULL");
    return false;
  }

  *client = server_find_client_by_dbus_name(*sender);
  if (!*client) {
    lash_dbus_error(call, LASH_DBUS_ERROR_UNKNOWN_CLIENT,
                    "Sender %s is not a LASH client",
                    *sender);
    return false;
  }

  return true;
}

static void
lashd_dbus_jack_name(method_call_t *call)
{
  const char *sender, *jack_name;
  struct lash_client *client;
  DBusError err;

  if (!get_message_sender(call, &sender, &client))
    return;

  dbus_error_init(&err);

  if (!dbus_message_get_args(call->message, &err,
                             DBUS_TYPE_STRING, &jack_name,
                             DBUS_TYPE_INVALID)) {
    lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_ARGS,
                    "Invalid arguments to method \"%s\": %s",
                    call->method_name, err.message);
    dbus_error_free(&err);
    return;
  }

  client_maybe_fill_class(client);

  // TODO: Send ClientJackNameChanged signal
}

static void
lashd_dbus_get_name(method_call_t *call)
{
  const char *sender, *name;
  struct lash_client *client;

  if (!get_message_sender(call, &sender, &client))
    return;

  name = client->name ? (const char *) client->name : "";

  lash_debug("Telling client '%s' its LASH name '%s'",
             client_get_identity(client), name);

  method_return_new_single(call, DBUS_TYPE_STRING, &name);
}

static void
lashd_dbus_get_jack_name(method_call_t *call)
{
  const char *sender, *name;
  struct lash_client *client;

  if (!get_message_sender(call, &sender, &client))
    return;

  name = client->jack_client_name
         ? (const char *) client->jack_client_name
         : "";

  lash_debug("Telling client '%s' its JACK name '%s'",
             client_get_identity(client), name);

  method_return_new_single(call, DBUS_TYPE_STRING, &name);
}

#if 0
static void
lashd_dbus_save_project(method_call_t *call)
{
  const char *sender;
  struct lash_client *client;

  if (!get_message_sender(call, &sender, &client))
    return;

  if (!client->project) {
    lash_dbus_error(call, LASH_DBUS_ERROR_UNKNOWN_PROJECT,
                    "Client's project pointer is NULL");
    return;
  }

  lash_debug("Saving project '%s' by request of client '%s'",
             client->project->name, client_get_identity(client));

  project_save(client->project);
}

static void
lashd_dbus_close_project(method_call_t *call)
{
  const char *sender;
  struct lash_client *client;

  if (!get_message_sender(call, &sender, &client))
    return;

  if (!client->project) {
    lash_dbus_error(call, LASH_DBUS_ERROR_UNKNOWN_PROJECT,
                    "Client's project pointer is NULL");
    return;
  }

  lash_debug("Closing project '%s' by request of client '%s'",
             client->project->name, client_get_identity(client));

  server_close_project(client->project);
}
#endif

static bool
check_tasks(method_call_t   *call,
            struct lash_client        *client,
            DBusMessageIter *iter)
{
  DBusMessageIter xiter;
  dbus_uint64_t task_id;

  if (!iter)
    iter = &xiter;

  if (!dbus_message_iter_init(call->message, iter)
      || dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_UINT64) {
    lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_ARGS,
                    "Invalid arguments to method \"%s\": "
                    "Cannot find task ID in message",
                    call->method_name);
    return false;
  }

  dbus_message_iter_get_basic(iter, &task_id);
  dbus_message_iter_next(iter);

  if (!client->pending_task) {
    lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_TASK,
                    "Client '%s' has no pending task",
                    client->name);
    return false;
  } else if (client->pending_task != task_id) {
    lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_TASK,
                    "Client '%s' has a pending task, but it "
                    "does not match the one in the method call",
                    client->name);
    return false;
  }

  return true;
}

static void
lashd_dbus_progress(method_call_t *call)
{
  lash_debug("Progress");

  const char *sender;
  struct lash_client *client;
  uint8_t percentage;
  DBusMessageIter iter;

  if (!get_message_sender(call, &sender, &client))
    return;

  lash_debug("Received Progress report from client '%s'",
             client_get_identity(client));

  if (!check_tasks(call, client, &iter))
    return;

  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_BYTE) {
    lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_ARGS,
                    "Invalid arguments to method \"%s\": "
                    "Cannot find progress value in message",
                    call->method_name);
    return;
  }
  dbus_message_iter_get_basic(&iter, &percentage);

  client_task_progressed(client, percentage);
}

static void
lashd_dbus_commit_path_change(method_call_t *call)
{
  lash_debug("CommitPathChange");

  const char *sender;
  struct lash_client *client;

  if (!get_message_sender(call, &sender, &client))
    return;

  // Here check that there really is a pending path change
  // for the client, send error return if it isn't so

  // Try to commit the pending path change,
  // send error return if it fails

  // Let's pretend that we have succesfully changed the path
  // and are returning the new path name to the client
  const char *foobar = "/tmp";
  method_return_new_single(call, DBUS_TYPE_STRING,
                           (const void *) &foobar);
}

static __inline__ bool
get_and_set_config(store_t         *store,
                   DBusMessageIter *iter)
{
  const char *key;
  int type, size;

  union {
    double      d;
    uint32_t    u;
    const char *s;
    const void *v;
  } value;

  if (!method_iter_get_dict_entry(iter, &key, &value, &type, &size)) {
    lash_error("Cannot get dict entry from config message");
    return false;
  }

  if (type == LASH_TYPE_DOUBLE) {
    return store_set_config(store, key, &value, sizeof(double), type);
  } else if (type == LASH_TYPE_INTEGER) {
    return store_set_config(store, key, &value, sizeof(uint32_t), type);
  } else if (type == LASH_TYPE_STRING) {
    return store_set_config(store, key, value.s, strlen(value.s) + 1, type);
  } else if (type == LASH_TYPE_RAW) {
    return store_set_config(store, key, value.v, size, type);
  } else {
    lash_error("Unsupported config type '%c'", (char) type);
    return false;
  }
}

/* The CommitDataSet method is used by the client to deliver its data set
   to the server. The client will never call this method unless requested
   to do so by LASH. */
static void
lashd_dbus_commit_data_set(method_call_t *call)
{
  lash_debug("CommitDataSet");

  const char *sender;
  struct lash_client *client;
  DBusMessageIter iter, array_iter;
  bool was_successful = true;

  if (!get_message_sender(call, &sender, &client))
    return;

  lash_debug("Received data set from client '%s'",
             client_get_identity(client));

  if (!check_tasks(call, client, &iter))
    return;

  if (!client->store) {
    lash_dbus_error(call, LASH_DBUS_ERROR_GENERIC,
                    "Client's store pointer is NULL");
    return;
  }

  if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
    lash_dbus_error(call, LASH_DBUS_ERROR_INVALID_ARGS,
                    "Invalid arguments to method \"%s\"",
                    call->method_name);
    return;
  }

  dbus_message_iter_recurse(&iter, &array_iter);

  if (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_INVALID) {
    lash_dbus_error(call, LASH_DBUS_ERROR_GENERIC,
                    "Message contains no configs");
    return;
  }

  /* Loop through the configs and commit them to the store */
  do {
    if (!get_and_set_config(client->store, &array_iter)) {
      lash_dbus_error(call, LASH_DBUS_ERROR_GENERIC,
                      "Config data is corrupt");
      was_successful = false;
      break;
    }
  } while (dbus_message_iter_next(&array_iter));

  /* Sending a valid data set implies task completion */
  client_task_completed(client, was_successful);
}

/*
 * Interface methods.
 */

METHOD_ARGS_BEGIN(Ping)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Connect)
  METHOD_ARG_DESCRIBE("pid", "i", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("class", "s", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("flags", "i", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("working_dir", "s", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("args", "as", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("uuid", "s", DIRECTION_OUT)
  METHOD_ARG_DESCRIBE("client_name", "s", DIRECTION_OUT)
  METHOD_ARG_DESCRIBE("project_name", "s", DIRECTION_OUT)
  METHOD_ARG_DESCRIBE("data_path", "s", DIRECTION_OUT)
  METHOD_ARG_DESCRIBE("final_flags", "i", DIRECTION_OUT)
  METHOD_ARG_DESCRIBE("final_working_dir", "s", DIRECTION_OUT)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(JackName)
  METHOD_ARG_DESCRIBE("jack_name", "s", DIRECTION_IN)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(GetName)
  METHOD_ARG_DESCRIBE("client_name", "s", DIRECTION_OUT)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(GetJackName)
  METHOD_ARG_DESCRIBE("jack_name", "s", DIRECTION_OUT)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(SaveProject)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(CloseProject)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Progress)
  METHOD_ARG_DESCRIBE("task_id", "t", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("percentage", "y", DIRECTION_IN)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(CommitDataSet)
  METHOD_ARG_DESCRIBE("task_id", "t", DIRECTION_IN)
  METHOD_ARG_DESCRIBE("configs", "a{sv}", DIRECTION_IN)
METHOD_ARGS_END

METHOD_ARGS_BEGIN(CommitPathChange)
METHOD_ARGS_END

METHODS_BEGIN
  METHOD_DESCRIBE(Ping, lashd_dbus_ping)
  METHOD_DESCRIBE(Connect, lashd_dbus_connect)
  METHOD_DESCRIBE(JackName, lashd_dbus_jack_name)
  METHOD_DESCRIBE(GetName, lashd_dbus_get_name)
  METHOD_DESCRIBE(GetJackName, lashd_dbus_get_jack_name)
  METHOD_DESCRIBE(Progress, lashd_dbus_progress)
  METHOD_DESCRIBE(CommitDataSet, lashd_dbus_commit_data_set)
  METHOD_DESCRIBE(CommitPathChange, lashd_dbus_commit_path_change)
METHODS_END

/*
 * Interface signals.
 */

SIGNAL_ARGS_BEGIN(Save)
  SIGNAL_ARG_DESCRIBE("project_name", "s")
  SIGNAL_ARG_DESCRIBE("task_id", "t")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(Load)
  SIGNAL_ARG_DESCRIBE("project_name", "s")
  SIGNAL_ARG_DESCRIBE("task_id", "t")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(Quit)
  SIGNAL_ARG_DESCRIBE("project_name", "s")
SIGNAL_ARGS_END

SIGNALS_BEGIN
  SIGNAL_DESCRIBE(Save)
  SIGNAL_DESCRIBE(Quit)
SIGNALS_END

/*
 * Interface description.
 */

INTERFACE_BEGIN(g_lashd_interface_server, DBUS_NAME_BASE ".Server")
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
  INTERFACE_EXPOSE_SIGNALS
INTERFACE_END
