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
#include <stdarg.h>

/* ret=true, err=0 - directory was created successfully */
/* ret=true, err=EEXIST - directory already exists */
/* ret=true, err=ENOENT -  A directory component in dirname does not exist or is a dangling symbolic link */
/* ret=true, err=error - directory creation failed in a bad way */
/* ret=false, err=error - dirname is not valid directory path */
static bool safe_mkdir(const char * dirname, int mode, int * err)
{
  struct stat st;

  ASSERT(*dirname);             /* empty string? */

  if (mkdir(dirname, mode) == 0)
  {
    *err = 0;
    return true;
  }

  if (errno != EEXIST)
  {
    *err = errno;
    return true;
  }

  /* dirname path exists, not necessarily as a directory.
     This includes the case where pathname is a symbolic link,
     dangling or not. */

  if (stat(dirname, &st) != 0)
  {
    *err = errno;
    log_error("Failed to stat \"%s\": %d (%s)", dirname, errno, strerror(errno));
    return false;
  }
  else if (!S_ISDIR(st.st_mode))
  {
    *err = ENOTDIR;
    log_error("\"%s\" exists but is not directory.", dirname);
    return false;
  }

  *err = EEXIST;
  return true;
}

static bool rmkdir(char * buffer, int mode)
{
  int err;
  char * p;
  size_t len;
  bool last;

  len = 0;
  p = buffer;
loop:
  last = *p == 0;
  if (!last)
  {
    if (*p != '/')
    {
      len++;
      p++;
      goto loop;
    }

    if (len == 0)
    {
      /* skip extra '/' chars */
      p++;
      goto loop;
    }

    *p = 0;
  }
  else if (len == 0)
  {
    return true;
  }

  if (!safe_mkdir(buffer, mode, &err))
  {
    return false;
  }

  switch (err)
  {
  case 0:
    log_info("Directory \"%s\" created", buffer);
    /* fall through */
  case EEXIST:
    break;
  default:
    log_error("Failed to create \"%s\" directory: %d (%s)", buffer, errno, strerror(errno));
    return false;
  }

  if (!last)
  {
    *p = '/';
    len = 0;
    p++;
    goto loop;
  }

  return true;
}

bool ensure_dir_exist(const char * dirname, int mode)
{
  size_t len;
  char * buffer;
  int err;
  bool ret;

  if (!safe_mkdir(dirname, mode, &err))
  {
    return false;
  }

  if (err == 0 || err == EEXIST)
  {
    return true;
  }

  if (errno != ENOENT)
  {
    log_error("Failed to create \"%s\" directory: %d (%s)", dirname, errno, strerror(errno));
    return false;
  }

  /* A directory component in dirname does not exist or is a dangling symbolic link */

  len = strlen(dirname);

  buffer = malloc(len + 1);
  if (buffer == NULL)
  {
    log_error("malloc(%zu) failed.", len);
    return false;
  }

  memcpy(buffer, dirname, len);
  buffer[len] = 0;

  ret = rmkdir(buffer, mode);

  free(buffer);

  return ret;
}

bool ensure_dir_exist_varg(int mode, ...)
{
  va_list ap;
  const char * str;
  size_t len;
  char * buffer;
  char * p;
  int ret;

  len = 0;
  va_start(ap, mode);
  while ((str = va_arg(ap, const char *)) != NULL)
  {
    len += strlen(str);
  }
  va_end(ap);
  ASSERT(len > 0);
  len++;

  buffer = malloc(len);
  if (buffer == NULL)
  {
    log_error("malloc(%zu) failed.", len);
    return false;
  }

  p = buffer;
  va_start(ap, mode);
  while ((str = va_arg(ap, const char *)) != NULL)
  {
    len = strlen(str);
    memcpy(buffer, str, len);
    p += len;
  }
  va_end(ap);
  ASSERT(p != buffer);
  *p = 0;

  ret = rmkdir(buffer, mode);

  free(buffer);

  return ret;
}
