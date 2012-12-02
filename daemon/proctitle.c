/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008,2009,2012 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the code that sets process "title"
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

/* This implementation should work on Linux, on FreeBSD we could use setprotitile(3) */

#include <stdarg.h>
#include <string.h>

#include "../common.h"

static char * g_argv_begin;
static char * g_argv_end;

void
lash_init_setproctitle(
  int argc,
  char ** argv,
  char ** UNUSED(envp))
{
  char * last_arg;

  g_argv_begin = argv[0];

  last_arg = argv[argc-1];

  g_argv_end = last_arg + strlen(last_arg);

  //lash_info("%u chars available for title", g_argv_end - g_argv_begin);
}

void
lash_setproctitile(
  const char * format,
  ...)
{
  va_list ap;

  va_start(ap, format);

  memset(g_argv_begin, 0, g_argv_end - g_argv_begin);

  vsnprintf(g_argv_begin, g_argv_end - g_argv_begin, format, ap);

  va_end(ap);
}
