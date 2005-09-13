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

#ifndef __LASH_EXEC_PARAMS_H__
#define __LASH_EXEC_PARAMS_H__

#include <uuid/uuid.h>

typedef struct _lash_exec_params lash_exec_params_t;

lash_exec_params_t * lash_exec_params_new ();
void                lash_exec_params_destroy (lash_exec_params_t * params);

void lash_exec_params_set_working_dir (lash_exec_params_t * params, const char * working_dir);
void lash_exec_params_set_server      (lash_exec_params_t * params, const char * server);
void lash_exec_params_set_project     (lash_exec_params_t * params, const char * project);
void lash_exec_params_set_args        (lash_exec_params_t * params, int argc, const char * const * argv);


#endif /* __LASH_EXEC_PARAMS_H__ */
