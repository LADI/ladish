/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010, 2011, 2012 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to the code that starts programs
 **************************************************************************
 *
 * LADI Session Handler is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * LADI Session Handler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LADI Session Handler. If not, see <http://www.gnu.org/licenses/>
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __LASHD_LOADER_H__
#define __LASHD_LOADER_H__

void loader_init(void (* on_child_exit)(pid_t pid, int exit_status));

bool
loader_execute(
  const char * vgraph_name,
  const char * project_name,
  const char * app_name,
  const char * working_dir,
  const char * session_dir,
  bool run_in_terminal,
  const char * commandline,
  bool set_env_vars,
  pid_t * pid_ptr);

void loader_run(void);

void loader_uninit(void);

unsigned int loader_get_app_count(void);

#endif /* __LASHD_LOADER_H__ */
