/*
 *   LASH
 *    
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

#include <lash/list.h>

#include "client.h"
#include "conn_mgr.h"

#define UI_PROJECT_NAME "ui-project"
#define UI_PROJECT_DIR  ".lashd/ui-project"

typedef struct _project project_t;

struct _project
{
  char *           name;
  char *           directory;
  lash_list_t *     clients;
  lash_list_t *     lost_clients;
  struct _server * server;

  /* stuff for operation completion feedback (LASH_Percentage) */
  int              saves;
  int              pending_saves;
};


project_t * project_new (struct _server * server);
void        project_destroy (project_t * project);
project_t * project_restore (struct _server * server, const char * dir);

client_t *project_get_client_by_id (project_t * project, uuid_t id);


void project_set_name (project_t * project, const char * name);
void project_set_directory (project_t * project, const char * directory);

void project_move              (project_t * project, const char * new_dir);
void project_add_client        (project_t * project, client_t * client);
void project_remove_client     (project_t * project, client_t * client);
void project_lose_client       (project_t * project, client_t * client, lash_list_t * jack_patches, lash_list_t * alsa_patches);
void project_name_client       (project_t * project, client_t * client, const char * name);
void project_move_client       (project_t * project, client_t * client, const char * new_project_dir);
void project_restore_data_set  (project_t * project, client_t * client);
void project_save              (project_t * project);
void project_file_complete     (project_t * project, client_t * client);
void project_data_set_complete (project_t * project, client_t * client);
void project_resume_client_alsa_patches (project_t * project, client_t * client);
void project_resume_client_jack_patches (project_t * project, client_t * client);


int project_name_exists (lash_list_t * projects, const char * name);
  
const char * project_get_client_id_dir (project_t * project, client_t * client);
const char * project_get_client_name_dir (project_t * project, client_t * client);
const char * project_get_client_config_dir (project_t * project, client_t * client);


#endif /* __LASHD_PROJECT_H__ */
