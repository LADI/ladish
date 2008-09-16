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
#include "common/list.h"

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

	char             *name;
	bool             move_on_close;
	char             *directory;
	char             *description;
	char             *notes;
	bool              modified_status;
	time_t            last_modify_time;

	struct list_head  clients;
	struct list_head  lost_clients;

	/* For task progress feedback (LASH_Percentage) */
	int               task_type;
	uint32_t          client_tasks_total;
	uint32_t          client_tasks_pending;
	uint32_t          client_tasks_progress; // Min is 0, max is client_tasks_total*100
};


project_t *
project_new(void);

project_t *
project_new_from_disk(const char *parent_dir,
                      const char *project_dir);

void
project_destroy(project_t *project);

void
project_unload(project_t *project);

bool
project_load(project_t *project);

bool
project_is_loaded(project_t *project);

client_t *
project_get_client_by_id(struct list_head *client_list,
                         uuid_t            id);

void
project_move(project_t  *project,
             const char *new_dir);

void
project_client_progress(project_t *project,
                        client_t  *client,
                        uint8_t    percentage);

void
project_client_task_completed(project_t *project,
                              client_t  *client);

void
project_rename(project_t  *project,
               const char *new_name);

void
project_launch_client(project_t *project,
                      client_t  *client);

void
project_add_client(project_t *project,
                  client_t   *client);

void
project_lose_client(project_t *project,
                    client_t  *client);

void
project_save(project_t *project);

void
project_satisfy_client_dependency(project_t *project,
                                  client_t  *client);

void
project_set_description(project_t  *project,
                        const char *description);

void
project_set_notes(project_t  *project,
                  const char *notes);

void
project_set_description(project_t  *project,
                        const char *description);

void
project_set_notes(project_t  *project,
                  const char *notes);

void
project_rename_client(project_t  *project,
                      client_t   *client,
                      const char *name);

void
project_clear_id_dir(project_t *project);

void
project_set_modified_status(project_t *project,
                            bool       new_status);

#endif /* __LASHD_PROJECT_H__ */
