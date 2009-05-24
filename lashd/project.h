/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *   Copyright (C) 2002 Robert Ham <rah@bash.sh>
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

#ifndef __LASHD_PROJECT_H__
#define __LASHD_PROJECT_H__

#include <stdbool.h>
#include <stdint.h>
#include <uuid/uuid.h>
#include <libxml/tree.h>

#include "common/klist.h"

#include "types.h"

#define PROJECT_ID_DIR      ".id"
#define PROJECT_CONFIG_DIR  ".config"
#define PROJECT_INFO_FILE   ".lash_info"
#define PROJECT_NOTES_FILE  ".notes"
#define PROJECT_XML_VERSION "1.0"

enum
{
	LASH_TASK_SAVE = 1,
	LASH_TASK_LOAD
};

struct _project
{
	struct list_head  siblings_all;
	struct list_head  siblings_loaded;

	xmlDocPtr         doc;

	/** user-visible name */
	char             *name;
	/** invoke project_move on project_unload */
	bool             move_on_close;
	
	/** absolute path to project directory */
	char             *directory;
	char             *description;
	char             *notes;
	bool              modified_status;
	time_t            last_modify_time;

	/** Clients that are running in a session */
	struct list_head  clients;
	/** Clients that are supposed to be running in a session, but don't (because they haven't been
	 * loaded yet, or failed to run, or dropped out of the session */
	struct list_head  lost_clients;

	/* For task progress feedback (LASH_Percentage) */
	int               task_type;
	uint32_t          client_tasks_total;
	uint32_t          client_tasks_pending;
	uint32_t          client_tasks_progress; // Min is 0, max is client_tasks_total*100
};

/** Create a new, empty project object, without setting the directory. Initializes
 * all fields to zero, except for clients and lost_clients lists. */
project_t *
project_new(void);

/** Load project header from disk. Must be followed by project_load.
 *
 * @arg parent_dir    directory where projects generally reside (like $HOME/audio_projects)
 * @arg project_dir   directory name (relative to parent_dir) where the given project resides
 */
project_t *
project_new_from_disk(const char *parent_dir,
                      const char *project_dir);

/** Destroy the project, by unloading it (if it's loaded) and freeing data structures
 * associated with the project. 
 * @todo Free client lists
 */
void
project_destroy(project_t *project);

/** Unload the project. Steps involved:
 * - send a Quit server message
 * - for all clients:
 *   - call jack_mgr_remove_client (under a lock)
 *   - call jack_patch_destroy on all JACK patches and free JACK patch list
 *   - call alsa_mgr_remove_client (under a lock)
 *   - call alsa_patch_destroy on all JACK patches and free ALSA patch list
 *   - unlink and delete the client
 * - delete all lost_clients
 * - if move_on_close is set, do project_move on the project
 * - if project directory exists but doc == NULL (orphaned newly-created project),
 *   remove the directory
 */
void
project_unload(project_t *project);

/** Load some of the internal data of the project. Steps involved:
 * - create stub clients (fill the client info based on XML) and add them to lost_clients list
 * - load project notes
 * - print a debug header when compiled with LASH_DEBUG defined
 * - set modified state to false
 * Does not handle any further steps (dependency checking, starting clients, notifying about
 * newly appeared project, tracking progress etc.) - those are handled by server_project_restore.
 *
 * @arg project   Project pointer - must be created by project_new_from_disk?
 */
bool
project_load(project_t *project);

/** Checks if project has been loaded from disk (if there was any client to load).
 */
bool
project_is_loaded(project_t *project);

/** Retrieve a client from client_list based on UUID value.
 * @arg client_list    a list of clients (project->clients, project->lost_clients etc.)
 * @arg id             UUID to search for
 */
struct lash_client *
project_get_client_by_id(struct list_head *client_list,
                         uuid_t            id);

/** Rename the project directory to new_dir. Calls client_store_close on all 
 * clients before renaming, and client_store_open after renaming. Also, calls
 * lashd_dbus_signal_emit_project_path_changed with the new directory between
 * renaming and calling client_store_open.
 *
 * @arg project    project pointer
 * @arg new_dir    new directory (absolute path)
 */
void
project_move(project_t  *project,
             const char *new_dir);

/** Set the client's completion percentage to a given value, and update the
 * overall completion percentage accordingly. Calls 
 * lashd_dbus_signal_emit_progress to communicate the new overall percentage.
 * 
 * @arg project    project that the percentage change occured in
 * @arg client     the client for which a percentage value needs to be set
 * @arg percentage the current value of client's percentage
 */
void
project_client_progress(project_t *project,
                        struct lash_client  *client,
                        uint8_t    percentage);

/** Set client's percentage to 100% and, if this was the last client performing
 * the task, perform task finalization:
 * - reset the task-related fields in project struct
 * - call project_save or project_load (depending on a task)
 *
 * @arg project    project that the change occured in
 * @arg client     the client which completed the task
 */
void
project_client_task_completed(project_t *project,
                              struct lash_client  *client);

/** Set human-readable name for a project, call 
 * lashd_dbus_signal_emit_project_name_changed to notify about name change, and
 * set the modified state to true. Does not affect the project path.
 *
 * @arg project    project to rename
 * @arg new_name   new human-readable name
 */
void
project_rename(project_t  *project,
               const char *new_name);

void
project_load_file(project_t *project,
                  struct lash_client  *client);

void
project_load_data_set(project_t *project,
                      struct lash_client  *client);

/** Use loader_execute to run a client - then free/zero the argv/argc in the client.
 * @warning Does not check if the operation succeeded before clearing argc/argv.
 *
 * @arg project    project the client belongs to
 * @arg client     client to run
 */
void
project_launch_client(project_t *project,
                      struct lash_client  *client);

/** Called after client has disappeared. If client has left any data (keys), it calls
 * store_write to save them. If there are no data for the client, the client directory
 * is removed. If the project gets empty, its directory is removed, too. JACK and ALSA
 * "backup" patches are retrieved and stored in client's structure. Client instance
 * data (argc, argv and pid) are cleared. The modified flag is set.
 * lashd_dbus_signal_emit_client_disappeared is called to notify about client
 * disappearance.
 *
 * @arg project    project the client has disappeared from
 * @arg client     client that has disappeared
 */
void
project_lose_client(project_t *project,
                    struct lash_client  *client);

struct lash_client *
project_find_lost_client_by_class(project_t  *project,
                                  const char *class);

const char *
project_get_client_dir(project_t *project,
                       struct lash_client  *client);

/** Initiate the process of saving the project. Steps involved:
 * - add the project to project list (if it's new)
 * - create the project directory
 * - notify the clients to save their state
 * - write project metadata
 * - call project_clear_lost_clients(?)
 * - if no clients need to be saved, the save is complete, so project_saved is called.
 */
void
project_save(project_t *project);

void
project_satisfy_client_dependency(project_t *project,
                                  struct lash_client  *client);

void
project_set_description(project_t  *project,
                        const char *description);

void
project_set_notes(project_t  *project,
                  const char *notes);

void
project_rename_client(project_t  *project,
                      struct lash_client   *client,
                      const char *name);

void
project_clear_id_dir(project_t *project);

void
project_set_modified_status(project_t *project,
                            bool       new_status);

void
project_name_client(project_t  *project,
                    struct lash_client   *client);

void
project_new_client(
	project_t *project,
	struct lash_client  *client);

project_t *
server_get_newborn_project();

#endif /* __LASHD_PROJECT_H__ */
