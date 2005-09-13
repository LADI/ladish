/*
 *   LASH
 *    
 *   Copyright (C) 2003 Robert Ham <rah@bash.sh>
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

#include <lash/lash.h>
#include <lash/internal_headers.h>

#include "project.h"
#include "client.h"

project_t *
project_new (const char * name)
{
  project_t *project;
  project = lash_malloc0 (sizeof (project_t));

  project_set_name (project, name);

  return project;
}

void
project_destroy (project_t * project)
{
  lash_list_t * node;
  project_set_name (project, NULL);
  project_set_dir (project, NULL);

  for (node = project->clients; node; node = lash_list_next (node))
    client_destroy ((client_t *) node->data);

  lash_list_free (project->clients);

  free (project);
}

void
project_set_name (project_t * project, const char * name)
{
  set_string_property (project->name, name);
}

void
project_set_dir (project_t * project, const char * dir)
{
  set_string_property (project->dir, dir);
}



/* EOF */
