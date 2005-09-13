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

#ifndef __LASH_CONNECT_PARAMS_H__
#define __LASH_CONNECT_PARAMS_H__

#include <uuid/uuid.h>

#include <lash/internal_headers.h>

typedef struct _lash_connect_params lash_connect_params_t;

struct _lash_connect_params
{
  lash_protocol_t protocol_version;
  int            flags;
  char          *project;
  char          *working_dir;
  char          *class;
  uuid_t         id;
  int            argc;
  char         **argv;
};

lash_connect_params_t * lash_connect_params_new ();
void                   lash_connect_params_destroy (lash_connect_params_t *);

void lash_connect_params_set_project     (lash_connect_params_t *, const char *project);
void lash_connect_params_set_working_dir (lash_connect_params_t *, const char *working_dir);
void lash_connect_params_set_class       (lash_connect_params_t *, const char *class);

#endif /* __LASH_CONNECT_PARAMS_H__ */
