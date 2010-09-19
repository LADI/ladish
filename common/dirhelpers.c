/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009,2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the directory helper functions
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

#include "../common.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

bool
ensure_dir_exist(
  const char * dirname,
  int mode)
{
  struct stat st;
  if (stat(dirname, &st) != 0)
  {
    if (errno == ENOENT)
    {
      log_info("Directory \"%s\" does not exist. Creating...", dirname);
      if (mkdir(dirname, mode) != 0)
      {
        log_error("Failed to create \"%s\" directory: %d (%s)", dirname, errno, strerror(errno));
        return false;
      }
    }
    else
    {
      log_error("Failed to stat \"%s\": %d (%s)", dirname, errno, strerror(errno));
      return false;
    }
  }
  else
  {
    if (!S_ISDIR(st.st_mode))
    {
      log_error("\"%s\" exists but is not directory.", dirname);
      return false;
    }
  }

  return true;
}
