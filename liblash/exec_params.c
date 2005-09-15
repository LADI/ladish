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

#include <uuid/uuid.h>

#include <lash/lash.h>
#include <lash/internal_headers.h>

lash_exec_params_t *
lash_exec_params_new()
{
	lash_exec_params_t *params;

	params = lash_malloc0(sizeof(lash_exec_params_t));
	uuid_clear(params->id);
	return params;
}

void
lash_exec_params_destroy(lash_exec_params_t * params)
{
	lash_exec_params_set_working_dir(params, NULL);
	lash_exec_params_set_server(params, NULL);
	lash_exec_params_set_project(params, NULL);
	lash_exec_params_set_args(params, 0, NULL);
}

void
lash_exec_params_set_working_dir(lash_exec_params_t * params,
								 const char *working_dir)
{
	set_string_property(params->working_dir, working_dir);
}

void
lash_exec_params_set_server(lash_exec_params_t * params, const char *server)
{
	set_string_property(params->server, server);
}

void
lash_exec_params_set_project(lash_exec_params_t * params, const char *project)
{
	set_string_property(params->project, project);
}

void
lash_exec_params_set_args(lash_exec_params_t * params, int argc,
						  const char *const *argv)
{
	int i;

	if (params->argv) {
		for (i = 0; i < params->argc; i++)
			free(params->argv[i]);
		free(params->argv);
		params->argc = 0;
		params->argv = NULL;
	}

	if (argv) {
		params->argc = argc;
		params->argv = lash_malloc(sizeof(char *) * argc);
		for (i = 0; i < argc; i++)
			params->argv[i] = lash_strdup(argv[i]);
	}
}
