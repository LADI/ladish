/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the interface to code that interfaces procfs
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

#ifndef PROCFS_H__604D0D94_1609_4BB4_BFA7_5DC47830011A__INCLUDED
#define PROCFS_H__604D0D94_1609_4BB4_BFA7_5DC47830011A__INCLUDED

#include "../common.h"

bool
procfs_get_process_cmdline(
  unsigned long long pid,
  int * argc_ptr,
  char *** argv_ptr);

char *
procfs_get_process_cwd(
  unsigned long long pid);

unsigned long long
procfs_get_process_parent(
  unsigned long long pid);

#endif /* #ifndef PROCFS_H__604D0D94_1609_4BB4_BFA7_5DC47830011A__INCLUDED */
