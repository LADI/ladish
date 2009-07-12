/*
 *   LASH
 *
 *   Copyright (C) 2002, 2003 Robert Ham <rah@bash.sh>
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

#include "args.h"

#ifdef LASH_OLD_API

# include <stdlib.h>

# include "../../common/safety.h"

lash_args_t *
lash_args_new()
{
	return lash_calloc(1, sizeof(lash_args_t));
}

lash_args_t *
lash_args_dup(const lash_args_t *const src)
{
	if (src == NULL)
		return NULL;

	lash_args_t *result = lash_calloc(1, sizeof(lash_args_t));

	if (src->project)
		result->project = lash_strdup(src->project);
	if (src->server)
		result->server = lash_strdup(src->server);
	result->flags = src->flags;
	if (src->argc > 0 && src->argv)
		lash_args_set_args(result, src->argc, (const char**)src->argv);

	return result;
}

static void
lash_args_free_argv(lash_args_t *args)
{
	if (args->argv) {
		int i;

		for (i = 0; i < args->argc; i++)
			free(args->argv[i]);
		free(args->argv);
		args->argv = NULL;
	}
}

void
lash_args_destroy(lash_args_t *args)
{
	if (args) {
		lash_free(&args->project);
		lash_free(&args->server);
		lash_args_free_argv(args);
		free(args);
	}
}

void
lash_args_set_args(lash_args_t  *args,
                   int           argc,
                   const char  **argv)
{
	int i;

	lash_args_free_argv(args);

	args->argc = argc;
	args->argv = (argc > 0) ? lash_malloc(argc, sizeof(char *)) : NULL;

	for (i = 0; i < argc; i++)
		args->argv[i] = lash_strdup(argv[i]);
}

#endif /* LASH_OLD_API */
