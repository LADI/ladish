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
/** @file app_supervisor.h */

#ifndef APP_SUPERVISOR_H__712E6589_DCB1_4CE9_9812_4F250D55E8A2__INCLUDED
#define APP_SUPERVISOR_H__712E6589_DCB1_4CE9_9812_4F250D55E8A2__INCLUDED

#include "common.h"

#define LADISH_APP_STATE_STOPPED    0 /**< @brief app is stopped (not running) */
#define LADISH_APP_STATE_STARTED    1 /**< @brief app is running and not stopping */
#define LADISH_APP_STATE_STOPPING   2 /**< @brief app is stopping */
#define LADISH_APP_STATE_KILL       3 /**< @brief app is being force killed */

/**
 * App supervisor object handle (pointer to opaque data)
 */
typedef struct ladish_app_supervisor_tag { int unused; /**< fake */ } * ladish_app_supervisor_handle;

/**
 * App object handle (pointer to opaque data)
 *
 * The app objects are owned by the app supervisor objects (there is not refcounting)
 */
typedef struct ladish_app_tag { int unused; /**< fake */ } * ladish_app_handle;

/**
 * Type of function that is called when app is renamed
 *
 * @param[in] context User defined context that was supplied to ladish_app_supervisor_create()
 * @param[in] old_name Old name of the app
 * @param[in] new_name New name of the app
 */
typedef void (* ladish_app_supervisor_on_app_renamed_callback)(
  void * context,
  const char * old_name,
  const char * new_app_name);

/**
 * Type of function that is called during app enumeration
 *
 * @param[in] context User defined context that was supplied to ladish_app_supervisor_enum()
 * @param[in] name Name of the app
 * @param[in] running Whether the app is currently running
 * @param[in] command Commandline that is used to start the app
 * @param[in] terminal Whether the app is started in terminal
 * @param[in] level The level that app was started in
 * @param[in] pid PID of the app; Zero if app is not start
 */
typedef bool (* ladish_app_supervisor_enum_callback)(
  void * context,
  const char * name,
  bool running,
  const char * command,
  bool terminal,
  uint8_t level,
  pid_t pid);

/**
 * Create app supervisor object
 *
 * @param[out] supervisor_handle_ptr Pointer to variable that will receive supervisor handle
 * @param[in] opath Unique D-Bus object path for supervisor being created
 * @param[in] name Name of the supervisor
 * @param[in] context User defined context to be supplied when the callback suppiled through the @c on_app_renamed parameter is called
 * @param[in] on_app_renamed Callback to call when app is renamed
 *
 * @return success status
 */
bool
ladish_app_supervisor_create(
  ladish_app_supervisor_handle * supervisor_handle_ptr,
  const char * opath,
  const char * name,
  void * context,
  ladish_app_supervisor_on_app_renamed_callback on_app_renamed);

/**
 * Destroy app supervisor object
 *
 * @param[in] supervisor_handle supervisor object handle
 */
void
ladish_app_supervisor_destroy(
  ladish_app_supervisor_handle supervisor_handle);

/**
 * Mark that app has quit
 *
 * This function is called to mark that app has quit. Must not be called from signal handler because app supervisor object is not thread safe.
 *
 * @param[in] supervisor_handle supervisor object handle
 * @param[in] pid of the app whose termination was detected
 */
bool
ladish_app_supervisor_child_exit(
  ladish_app_supervisor_handle supervisor_handle,
  pid_t pid);

/**
 * Iterate apps that are owned by supervisor
 *
 * @param[in] supervisor_handle supervisor object handle
 * @param[in] context User defined context to be supplied when the callback suppiled through the @c callback parameter is called
 * @param[in] callback Callback to call for each app
 */
bool
ladish_app_supervisor_enum(
  ladish_app_supervisor_handle supervisor_handle,
  void * context,
  ladish_app_supervisor_enum_callback callback);

/**
 * It is not clear what this function is supposed to do
 *
 * @param[in] supervisor_handle supervisor object handle
 */
void
ladish_app_supervisor_clear(
  ladish_app_supervisor_handle supervisor_handle);

/**
 * Add app. Apps are added in stopped state
 *
 * @param[in] supervisor_handle supervisor object handle
 * @param[in] name Name of the app
 * @param[in] autorun whether to start the app when ladish_app_supervisor_autorun() is called
 * @param[in] command Commandline that is used to start the app
 * @param[in] terminal Whether the app is started in terminal
 * @param[in] level The level that app was started in
 *
 * @return app handle on success; NULL on failure; the app handle is owned by the app supervisor object
 */
ladish_app_handle
ladish_app_supervisor_add(
  ladish_app_supervisor_handle supervisor_handle,
  const char * name,
  bool autorun,
  const char * command,
  bool terminal,
  uint8_t level);

/**
 * Initiate stop of all apps owned by this supervisor
 *
 * @param[in] supervisor_handle supervisor object handle
 */
void
ladish_app_supervisor_stop(
  ladish_app_supervisor_handle supervisor_handle);

/**
 * Start all apps that were added with autorun enabled
 *
 * @param[in] supervisor_handle supervisor object handle
 */
void
ladish_app_supervisor_autorun(
  ladish_app_supervisor_handle supervisor_handle);

/**
 * Search app by pid and return its name
 *
 * TODO: this should be renamed to match the fact that it returns app name and not app handle.
 * Implementing ladish_app_supervisor_find_app_by_pid() makes sense as well.
 *
 * @param[in] supervisor_handle supervisor object handle
 * @param[in] pid pid of the app to search for
 *
 * @return app name on success; NULL if app is not found
 */
char *
ladish_app_supervisor_search_app(
  ladish_app_supervisor_handle supervisor_handle,
  pid_t pid);

/**
 * Get name of the supervisor
 *
 * TODO: This should be probably removed in favour of ladish_app_supervisor_get_opath(); it is used for debuging purposes only
 *
 * @param[in] supervisor_handle supervisor object handle
 *
 * @retval app name; the buffer is owned by the app supervisor object
 */
const char * ladish_app_supervisor_get_name(ladish_app_supervisor_handle supervisor_handle);

/**
 * Get number of apps that are currently running
 *
 * @param[in] supervisor_handle supervisor object handle
 *
 * @return Number of apps that are currently running
 */
unsigned int ladish_app_supervisor_get_running_app_count(ladish_app_supervisor_handle supervisor_handle);

/**
 * Check whether app with name supplied name exists
 *
 * @param[in] supervisor_handle supervisor object handle
 * @param[in] name name of the app to search for
 *
 * @retval true app with supplied name exists
 * @retval false app with supplied name does not exist
 */
bool ladish_app_supervisor_check_app_name(ladish_app_supervisor_handle supervisor_handle, const char * name);

/**
 * Find app by id (as exposed through the D-Bus interface)
 *
 * @param[in] supervisor_handle supervisor object handle
 * @param[in] id id of the app
 *
 * @return app handle on success; NULL if app is not found; the app handle is owned by the app supervisor object
 */
ladish_app_handle ladish_app_supervisor_find_app_by_id(ladish_app_supervisor_handle supervisor_handle, uint64_t id);

/**
 * The the D-Bus object path for the supervisor.
 *
 * @param[in] supervisor_handle supervisor object handle
 * 
 * @retval supervisor name; the buffer is owned by the app supervisor object
 */
const char * ladish_app_supervisor_get_opath(ladish_app_supervisor_handle supervisor_handle);

/**
 * Start an app. The app must be in stopped state.
 *
 * @param[in] supervisor_handle supervisor object handle
 * @param[in] app_handle Handle of app to start
 *
 * @return success status
 */
bool ladish_app_supervisor_start_app(ladish_app_supervisor_handle supervisor_handle, ladish_app_handle app_handle);

/**
 * Stop an app. The app must be in started state.
 *
 * @param[in] supervisor_handle supervisor object handle
 * @param[in] app_handle Handle of app to stop
 */
void ladish_app_supervisor_stop_app(ladish_app_supervisor_handle supervisor_handle, ladish_app_handle app_handle);

/**
 * Force kill an app. The app must be in started state.
 *
 * @param[in] supervisor_handle supervisor object handle
 * @param[in] app_handle Handle of app to stop
 */
void ladish_app_supervisor_kill_app(ladish_app_supervisor_handle supervisor_handle, ladish_app_handle app_handle);

/**
 * Remove an app. The app must be in stopped state.
 *
 * @param[in] supervisor_handle supervisor object handle
 * @param[in] app_handle Handle of app to start
 */
void ladish_app_supervisor_remove_app(ladish_app_supervisor_handle supervisor_handle, ladish_app_handle app_handle);

/**
 * Get commandline for an app.
 *
 * @param[in] app_handle app object handle
 *
 * @retval app commandline; the buffer is owned by the app supervisor object
 */
const char * ladish_app_get_commandline(ladish_app_handle app_handle);

/**
 * Get app state
 *
 * @param[in] app_handle app object handle
 *
 * @return app state
 * @retval LADISH_APP_STATE_STOPPED app is stopped (not running)
 * @retval LADISH_APP_STATE_STARTED app is running and not stopping or being killed
 * @retval LADISH_APP_STATE_STOPPING app is stopping
 * @retval LADISH_APP_STATE_KILL app is being force killed
 */
unsigned int ladish_app_get_state(ladish_app_handle app_handle);

/**
 * Check whether app is currently running
 *
 * @param[in] app_handle app object handle
 *
 * @retval true App is running
 * @retval false App is not running
 */
bool ladish_app_is_running(ladish_app_handle app_handle);

/**
 * Get app name
 *
 * @param[in] app_handle app object handle
 *
 * @retval app name; the buffer is owned by the app supervisor
 */
const char * ladish_app_get_name(ladish_app_handle app_handle);

/**
 * D-Bus interface descriptor for the app supervisor interface. The call context must be a ::ladish_app_supervisor_handle
 */
extern const struct dbus_interface_descriptor g_iface_app_supervisor;

#endif /* #ifndef APP_SUPERVISOR_H__712E6589_DCB1_4CE9_9812_4F250D55E8A2__INCLUDED */
