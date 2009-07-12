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

#ifndef __LIBLASH_ARGS_H__
#define __LIBLASH_ARGS_H__

#include "config.h"

#ifdef LASH_OLD_API

# include "lash/types.h"

struct _lash_args
{
	char  *project;
	char  *server;
	int    flags;

	int    argc;
	char **argv;
};

# define lash_args_set_flag(args, flag) \
  (args)->flags |= flag

lash_args_t *
lash_args_new(void);

lash_args_t *
lash_args_dup(const lash_args_t *const src);

void
lash_args_destroy(lash_args_t *args);

void
lash_args_set_args(lash_args_t  *args,
                   int           argc,
                   const char  **argv);

#endif /* LASH_OLD_API */

#endif /* __LIBLASH_ARGS_H__ */
