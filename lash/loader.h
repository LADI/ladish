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

#ifndef __LASH_LOADER_H__
#define __LASH_LOADER_H__

#include <sys/types.h>

#include <lash/exec_params.h>

typedef struct _loader loader_t;

loader_t * loader_new     ();
void       loader_destroy (loader_t * loader);

int        loader_fork   (loader_t * loader);
void       loader_load   (loader_t * loader, const lash_exec_params_t * params);

#endif /* __LASH_LOADER_H__ */
