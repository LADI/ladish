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

#ifndef __LASH_CONTROL_H__
#define __LASH_CONTROL_H__

#include <lash/lash.h>
#include <lash/list.h>

#include "project.h"

typedef struct _lash_control lash_control_t;

struct _lash_control
{
  lash_client_t * client;
  lash_list_t * projects;
  project_t * cur_project;
};

void lash_control_init (lash_control_t * control);
void lash_control_free (lash_control_t * control);

void lash_control_main (lash_control_t * control);

#endif /* __LASH_CONTROL_H__ */
