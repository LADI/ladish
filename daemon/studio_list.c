/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010, 2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains studio list implementation
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

#include "common.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "studio_internal.h"
#include "../common/catdup.h"
#include "escape.h"

struct ladish_studios_iterate_context
{
  void * call_ptr;
  void * context;
  bool (* callback)(void * call_ptr, void * context, const char * studio, uint32_t modtime);
  unsigned int counter;
};

#define ctx_ptr ((struct ladish_studios_iterate_context *)callback_context)

bool recent_studio_callback(void * callback_context, const char * item)
{
  char * path;
  struct stat st;

  if (!ladish_studio_compose_filename(item, &path, NULL))
  {
    log_error("failed to compose path of (recent) studio \%s\" file", item);
    goto exit;
  }

  if (stat(path, &st) != 0)
  {
    if (errno != ENOENT)
    {
      log_error("failed to stat '%s': %d (%s)", path, errno, strerror(errno));
    }

    goto free;
  }

  if (!S_ISREG(st.st_mode))
  {
    log_info("Ignoring recent studio that is not regular file. Mode is %07o", st.st_mode);
    goto free;
  }

  ctx_ptr->callback(ctx_ptr->call_ptr, ctx_ptr->context, item, st.st_mtime);
  ctx_ptr->counter++;

free:
  free(path);
exit:
  return true;
}

#undef ctx_ptr

bool ladish_studios_iterate(void * call_ptr, void * context, bool (* callback)(void * call_ptr, void * context, const char * studio, uint32_t modtime))
{
  DIR * dir;
  struct dirent * dentry;
  size_t len;
  struct stat st;
  char * path;
  char * name;
  struct ladish_studios_iterate_context ctx;

  ctx.call_ptr = call_ptr;
  ctx.context = context;
  ctx.callback = callback;
  ctx.counter = 0;

  ladish_recent_store_iterate_items(g_studios_recent_store, &ctx, recent_studio_callback);

  /* TODO: smarter error handling based on ctx.counter (dbus error vs just logged error) */

  dir = opendir(g_studios_dir);
  if (dir == NULL)
  {
    cdbus_error(call_ptr, DBUS_ERROR_FAILED, "Cannot open directory '%s': %d (%s)", g_studios_dir, errno, strerror(errno));
    return false;
  }

  while ((dentry = readdir(dir)) != NULL)
  {
    len = strlen(dentry->d_name);
    if (len <= 4 || strcmp(dentry->d_name + (len - 4), ".xml") != 0)
      continue;

    path = catdup(g_studios_dir, dentry->d_name);
    if (path == NULL)
    {
      cdbus_error(call_ptr, DBUS_ERROR_FAILED, "catdup() failed");
      closedir(dir);
      return false;
    }

    if (stat(path, &st) != 0)
    {
      cdbus_error(call_ptr, DBUS_ERROR_FAILED, "failed to stat '%s': %d (%s)", path, errno, strerror(errno));
      free(path);
      closedir(dir);
      return false;
    }

    free(path);

    if (!S_ISREG(st.st_mode))
    {
      //log_info("Ignoring direntry that is not regular file. Mode is %07o", st.st_mode);
      continue;
    }

    name = malloc(len - 4 + 1);
    if (name == NULL)
    {
      log_error("malloc() failed.");
      closedir(dir);
      return false;
    }

    name[unescape(dentry->d_name, len - 4, name)] = 0;
    //log_info("name = '%s'", name);

    if (!ladish_recent_store_check_known(g_studios_recent_store, name))
    {
      if (!callback(call_ptr, context, name, st.st_mtime))
      {
        free(name);
        closedir(dir);
        return false;
      }
    }

    free(name);
  }

  closedir(dir);
  return true;
}
