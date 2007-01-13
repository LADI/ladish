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

#ifndef __LASH_ARGS_H__
#define __LASH_ARGS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <uuid/uuid.h>

struct _lash_args
{
  char *  project;
  char *  server;
  uuid_t  id;
  int     flags;
  
  int     argc;
  char ** argv;
};

void lash_args_free (lash_args_t * args);

lash_args_t * lash_args_new ();
lash_args_t * lash_args_duplicate (const lash_args_t *const src);
void          lash_args_destroy (lash_args_t * args);

void lash_args_set_project (lash_args_t * args, const char * project);
void lash_args_set_server  (lash_args_t * args, const char * server);
void lash_args_set_port    (lash_args_t * args, int port);
void lash_args_set_id      (lash_args_t * args, uuid_t id);
void lash_args_set_flags   (lash_args_t * args, int flags);
void lash_args_set_flag    (lash_args_t * args, int flag);
void lash_args_set_args    (lash_args_t * args, int argc, const char ** argv);

const char *         lash_args_get_project (const lash_args_t * args);
const char *         lash_args_get_server  (const lash_args_t * args);
int                  lash_args_get_port    (const lash_args_t * args);
void                 lash_args_get_id      (const lash_args_t * args, uuid_t id);
int                  lash_args_get_flags   (const lash_args_t * args);
int                  lash_args_get_argc    (const lash_args_t * args);
char **              lash_args_take_argv   (lash_args_t * args);
const char * const * lash_args_get_argv    (const lash_args_t * args);

#ifdef __cplusplus
}
#endif

#endif /* __LASH_ARGS_H__ */
