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

#include <stdlib.h>

#include <lash/lash.h>
#include <lash/args.h>
#include <lash/internal.h>

void
lash_args_free_argv (lash_args_t * args)
{
  if (args->argv)
    {
      int i;
      for (i = 0; i < args->argc; i++)
        free (args->argv[i]);
      free (args->argv);
      args->argv = NULL;
    }
}

void
lash_args_free (lash_args_t * args)
{
  if (args->project)
    free (args->project);
  if (args->server)
    free (args->server);
  lash_args_free_argv (args);
}

lash_args_t *
lash_args_new ()
{
  lash_args_t * args;
  args = lash_malloc0 (sizeof (lash_args_t));
  uuid_clear (args->id);
  return args;
}

void
lash_args_destroy (lash_args_t * args)
{
  lash_args_free (args);
  free (args);
}

void
lash_args_set_project (lash_args_t * args, const char * project)
{
  set_string_property (args->project, project);
}

void
lash_args_set_server  (lash_args_t * args, const char * server)
{
  set_string_property (args->server, server);
}
void
lash_args_set_id      (lash_args_t * args, uuid_t id)
{
  uuid_copy (args->id, id);
}

void
lash_args_set_flags (lash_args_t * args, int flags)
{
  args->flags = flags;
}

void
lash_args_set_flag (lash_args_t * args, int flag)
{
  args->flags |= flag;
}

const char *
lash_args_get_project (const lash_args_t * args)
{
  return args->project;
}

const char *
lash_args_get_server  (const lash_args_t * args)
{
  return args->server;
}

void
lash_args_get_id      (const lash_args_t * args, uuid_t id)
{
  uuid_copy (id, ((lash_args_t *)args)->id);
}

int
lash_args_get_flags    (const lash_args_t * args)
{
  return args->flags;
}

void lash_args_set_args    (lash_args_t * args, int argc, const char * const * argv)
{
  int i;
  
  lash_args_free_argv (args);
  
  args->argc = argc;
  args->argv = lash_malloc (sizeof (char *) * argc);

  for (i = 0; i < argc; i++)
    args->argv[i] = lash_strdup (argv[i]);
}

int
lash_args_get_argc    (const lash_args_t * args)
{
  return args->argc;
}

char **
lash_args_take_argv   (lash_args_t * args)
{
  char ** argv;
  argv = args->argv;
  args->argv = NULL;
  return argv;
}

const char * const *
lash_args_get_argv   (const lash_args_t * args)
{
  return (const char * const *) args->argv;
}


/* EOF */
