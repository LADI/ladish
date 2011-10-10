/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009,2010,2011 Nedko Arnaudov <nedko@arnaudov.name>
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
#include "catdup.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <dirent.h>

bool check_dir_exists(const char * dirname)
{
  struct stat st;

  ASSERT(*dirname);             /* empty string? */

  if (stat(dirname, &st) != 0)
  {
    return false;
  }

  return S_ISDIR(st.st_mode);
}

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
    memcpy(p, str, len);
    p += len;
  }
  va_end(ap);
  ASSERT(p != buffer);
  *p = 0;

  ret = rmkdir(buffer, mode);

  free(buffer);

  return ret;
}

bool ladish_rmdir_recursive(const char * dirpath)
{
  DIR * dir;
  struct dirent * dentry_ptr;
  char * entry_fullpath;
  struct stat st;
  bool success;

  success = false;

  dir = opendir(dirpath);
  if (dir == NULL)
  {
    log_error("Cannot open directory '%s': %d (%s)", dirpath, errno, strerror(errno));
    goto exit;
  }

  while ((dentry_ptr = readdir(dir)) != NULL)
  {
    if (strcmp(dentry_ptr->d_name, ".") == 0 ||
        strcmp(dentry_ptr->d_name, "..") == 0)
    {
      continue;
    }

    entry_fullpath = catdup3(dirpath, "/", dentry_ptr->d_name);
    if (entry_fullpath == NULL)
    {
      log_error("catdup() failed");
      goto close;
    }

    if (stat(entry_fullpath, &st) != 0)
    {
      log_error("failed to stat '%s': %d (%s)", entry_fullpath, errno, strerror(errno));
    }
    else
    {
      if (S_ISDIR(st.st_mode))
      {
        if (!ladish_rmdir_recursive(entry_fullpath))
        {
          goto free;
        }
      }
      else
      {
        if (unlink(entry_fullpath) < 0)
        {
          log_error("unlink('%s') failed. errno = %d (%s)", dirpath, errno, strerror(errno));
          goto free;
        }
      }
    }

    free(entry_fullpath);
  }

  if (rmdir(dirpath) < 0)
  {
    log_error("rmdir('%s') failed. errno = %d (%s)", dirpath, errno, strerror(errno));
  }
  else
  {
    success = true;
  }

  goto close;

free:
  free(entry_fullpath);
close:
  closedir(dir);
exit:
  return success;
}

bool ladish_rotate(const char * src, const char * dst, unsigned int max_backups)
{
  size_t len = strlen(dst) + 100;
  char paths[2][len];
  struct stat st;
  char * path;
  const char * older_path;
  bool oldest_found;
  unsigned int backup;

  ASSERT(max_backups > 0);

  oldest_found = false;
  path = NULL;
  older_path = NULL;
  backup = max_backups;
  while (backup > 0)
  {
    path = paths[backup % 2];
    snprintf(path, len, "%s.%u", dst, backup);

    if (stat(path, &st) != 0)
    {
      if (!oldest_found && errno == ENOENT)
      {
        log_info("\"%s\" does not exist", path);
        goto next;
      }

      log_error("Failed to stat \"%s\": %d (%s)", path, errno, strerror(errno));
      return false;
    }

    if (!S_ISDIR(st.st_mode))
    {
      log_error("\"%s\" exists but is not directory.", path);
      return false;
    }

    oldest_found = true;

    if (backup < max_backups)
    {
      ASSERT(older_path != NULL);
      log_info("rename '%s' -> '%s'", path, older_path);
      if (rename(path, older_path) != 0)
      {
        log_error("rename('%s' -> '%s') failed. errno = %d (%s)", path, older_path, errno, strerror(errno));
        return false;
      }
    }
    else
    {
      /* try to remove dst.max_backups */
      log_info("rmdir '%s'", path);
      if (!ladish_rmdir_recursive(path))
      {
        return false;
      }
    }

  next:
    older_path = path;
    backup--;
  }

  ASSERT(path != NULL);

  log_info("rename '%s' -> '%s'", dst, path);
  if (rename(dst, path) != 0 && errno != ENOENT)
  {
    log_error("rename('%s' -> '%s') failed. errno = %d (%s)", dst, path, errno, strerror(errno));
    return false;
  }

  log_info("rename '%s' -> '%s'", src, dst);
  if (rename(src, dst) != 0)
  {
    log_error("rename('%s' -> '%s') failed. errno = %d (%s)", src, dst, errno, strerror(errno));
    return false;
  }

  return true;
}
