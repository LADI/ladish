/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010, 2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains alsapid helper functionality
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

#include "alsapid.h"

#include <string.h>             /* GNU basename() */

#include <stdio.h>
#include <stdlib.h>             /* atoll */
#include <unistd.h>
#include <sys/types.h>

#include <libgen.h>

void alsapid_compose_src_link(int alsa_client_id, char * buffer)
{
  sprintf(buffer, "/tmp/alsapid-%lld-%d", (long long)getuid(), alsa_client_id);
}

void alsapid_compose_dst_link(char * buffer)
{
  sprintf(buffer, "/proc/%lld", (long long)getpid());
}

bool alsapid_get_pid(int alsa_client_id, pid_t * pid_ptr)
{
  char src[MAX_ALSAPID_PATH];
  char dst[MAX_ALSAPID_PATH + 1];
  ssize_t ret;
  pid_t pid;

  alsapid_compose_src_link(alsa_client_id, src);

  ret = readlink(src, dst, MAX_ALSAPID_PATH);
  if (ret == -1)
  {
    return false;
  }

  dst[ret] = 0;

  pid = (pid_t)atoll(basename(dst));
  if (pid <= 1)
  {
    return false;
  }

  *pid_ptr = pid;
  return true;
}
