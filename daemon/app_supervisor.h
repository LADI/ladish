/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to app supervisor object
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

#ifndef APP_SUPERVISOR_H__712E6589_DCB1_4CE9_9812_4F250D55E8A2__INCLUDED
#define APP_SUPERVISOR_H__712E6589_DCB1_4CE9_9812_4F250D55E8A2__INCLUDED

#include "common.h"

typedef struct ladish_app_supervisor_tag { int unused; } * ladish_app_supervisor_handle;
typedef struct ladish_app_tag { int unused; } * ladish_app_handle;

/* create app supervisor */
bool
ladish_app_supervisor_create(
  ladish_app_supervisor_handle * supervisor_handle_ptr,
  const char * opath,
  const char * name,
  void * context,
  void (* on_app_renamed)(void * context, const char * old_name, const char * new_app_name));

/* destroy app supervisor */
void
ladish_app_supervisor_destroy(
  ladish_app_supervisor_handle supervisor_handle);

/* mark that app has quit */
bool
ladish_app_supervisor_child_exit(
  ladish_app_supervisor_handle supervisor_handle,
  pid_t pid);

/* iterate apps */
bool
ladish_app_supervisor_enum(
  ladish_app_supervisor_handle supervisor_handle,
  void * context,
  bool (* callback)(void * context, const char * name, bool running, const char * command, bool terminal, uint8_t level, pid_t pid));

/* It is not clear what this function is supposed to do */
void
ladish_app_supervisor_clear(
  ladish_app_supervisor_handle supervisor_handle);

/* Add app; on success, app handle is returned; on failure, NULL is returned;
   apps are added in stopped state; autorun parameter is just a hint for ladish_app_supervisor_autorun() */
ladish_app_handle
ladish_app_supervisor_add(
  ladish_app_supervisor_handle supervisor_handle,
  const char * name,
  bool autorun,
  const char * command,
  bool terminal,
  uint8_t level);

/* Initiate stop of all apps owned by this supervisor */
void
ladish_app_supervisor_stop(
  ladish_app_supervisor_handle supervisor_handle);

/* Start all apps that were added with autorun enabled */
void
ladish_app_supervisor_autorun(
  ladish_app_supervisor_handle supervisor_handle);

/* TODO: this should be renamed to match the fact that it returns app name and not app handle */
/* Implementing ladish_app_supervisor_find_app_by_pid() makes sense as well */
char *
ladish_app_supervisor_search_app(
  ladish_app_supervisor_handle supervisor_handle,
  pid_t pid);

/* TODO: This should be probably removed in favour of ladish_app_supervisor_get_opath(); it is used for debuging purposes only */
const char * ladish_app_supervisor_get_name(ladish_app_supervisor_handle supervisor_handle);

/* Get number of apps that are currently running */
unsigned int ladish_app_supervisor_get_running_app_count(ladish_app_supervisor_handle supervisor_handle);

/* TODO: ladish_app_supervisor_check_app_name() should be created and exported instead */
ladish_app_handle ladish_app_supervisor_find_app_by_name(ladish_app_supervisor_handle supervisor_handle, const char * name);

/* find app by id; returns NULL if app is not found */
ladish_app_handle ladish_app_supervisor_find_app_by_id(ladish_app_supervisor_handle supervisor_handle, uint64_t id);

/* the D-Bus object path for the supervisor */
const char * ladish_app_supervisor_get_opath(ladish_app_supervisor_handle supervisor_handle);

/* start an app; returns success status; app must be in stopped state */
bool ladish_app_supervisor_run(ladish_app_supervisor_handle supervisor_handle, ladish_app_handle app_handle);

/* remove an app; app must be in stopped state */
void ladish_app_supervisor_remove(ladish_app_supervisor_handle supervisor_handle, ladish_app_handle app_handle);

/* get commandline for app */
const char * ladish_app_get_commandline(ladish_app_handle app_handle);

/* check whether app is currently running */
bool ladish_app_is_running(ladish_app_handle app_handle);

/* get app name; the returned pointer is owned by the app supervisor */
const char * ladish_app_get_name(ladish_app_handle app_handle);

extern const struct dbus_interface_descriptor g_iface_app_supervisor;

#endif /* #ifndef APP_SUPERVISOR_H__712E6589_DCB1_4CE9_9812_4F250D55E8A2__INCLUDED */
