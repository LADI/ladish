/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010, 2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the recent items store
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "recent_store.h"
#include "save.h"

struct ladish_recent_store
{
  char * path;
  unsigned int max_items;
  char ** items;
};

static
void
ladish_recent_store_save(
  struct ladish_recent_store * store_ptr)
{
  unsigned int i;
  int fd;

  fd = open(store_ptr->path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd == -1)
  {
    log_error("open(%s) failed: %d (%s)", store_ptr->path, errno, strerror(errno));
    return;
  }

  for (i = 0; i < store_ptr->max_items && store_ptr->items[i] != NULL; i++)
  {
    if (!ladish_write_string(fd, store_ptr->items[i]))
    {
      log_error("write to file '%s' failed", store_ptr->path);
      break;
    }

    if (!ladish_write_string(fd, "\n"))
    {
      log_error("write to file '%s' failed", store_ptr->path);
      break;
    }
  }

  close(fd);
}

static
void
ladish_recent_store_load(
  struct ladish_recent_store * store_ptr)
{
  int fd;
  struct stat st;
  char * buffer;
  size_t buffer_size;
  ssize_t ret;
  char * temp_ptr;
  unsigned int i;

  fd = open(store_ptr->path, O_RDONLY);
  if (fd == -1)
  {
    if (errno != ENOENT)
    {
      log_error("open(%s) failed: %d (%s)", store_ptr->path, errno, strerror(errno));
    }

    goto exit;
  }

  if (fstat(fd, &st) != 0)
  {
    log_error("fstat(%s) failed: %d (%s)", store_ptr->path, errno, strerror(errno));
    goto close;
  }

  buffer_size = (size_t)st.st_size;
  buffer = malloc(buffer_size + 1);
  if (buffer == NULL)
  {
    log_error("malloc() failed to allocate %zy byte buffer for reading the contents of the recent items file '%s'", buffer_size + 1, store_ptr->path);
    goto close;
  }

  ret = read(fd, buffer, buffer_size);
  if (ret == -1)
  {
    log_error("read(%s) failed: %d (%s)", store_ptr->path, errno, strerror(errno));
    goto free;
  }

  if (ret < (size_t)buffer_size)
  {
    /* this could be handled better but we dont care because it is expected for the file to be small enough */
    log_error("read(%s) returned less bytes than requested", store_ptr->path);
    goto free;
  }

  buffer[buffer_size] = 0;

  /* read lines and store them in the item array */
  temp_ptr = strtok(buffer, "\n");
  i = 0;
  while (temp_ptr != NULL && i < store_ptr->max_items)
  {
    ASSERT(store_ptr->items[i] == NULL); /* filling an empty array */

    store_ptr->items[i] = strdup(temp_ptr);
    if (store_ptr->items[i] == NULL)
    {
      log_error("strdup(\"%s\") failed.", temp_ptr);

      /* free the already duplicated items */
      while (i > 0)
      {
        i--;
        free(store_ptr->items[i]);
        store_ptr->items[i] = NULL;
      }

      goto free;
    }

    i++;
    temp_ptr = strtok(NULL, "\n");
  }

free:
  free(buffer);
close:
  close(fd);
exit:
  return;
}

bool
ladish_recent_store_create(
  const char * file_path,
  unsigned int max_items,
  ladish_recent_store_handle * store_handle_ptr)
{
  struct ladish_recent_store * store_ptr;
  unsigned int i;

  store_ptr = malloc(sizeof(struct ladish_recent_store));
  if (store_ptr == NULL)
  {
    log_error("malloc() failed to allocate a ladish_recent_store struct");
    goto fail;
  }

  store_ptr->path = strdup(file_path);
  if (store_ptr->path == NULL)
  {
    log_error("strdup() failed for recent store path '%s'", file_path);
    goto free_store;
  }

  store_ptr->items = malloc(sizeof(char *) * max_items);
  if (store_ptr->items == NULL)
  {
    log_error("malloc() failed to array of %u pointers", max_items);
    goto free_path;
  }

  for (i = 0; i < max_items; i++)
  {
    store_ptr->items[i] = NULL;
  }

  store_ptr->max_items = max_items;

  /* try to load items from file */
  ladish_recent_store_load(store_ptr);

  *store_handle_ptr = (ladish_recent_store_handle)store_ptr;

  return true;

free_path:
  free(store_ptr->path);
free_store:
  free(store_ptr);
fail:
  return false;
}

#define store_ptr ((struct ladish_recent_store *)store_handle)

void
ladish_recent_store_destroy(
  ladish_recent_store_handle store_handle)
{
  unsigned int i;

  for (i = 0; i < store_ptr->max_items && store_ptr->items[i] != NULL; i++)
  {
    free(store_ptr->items[i]);
  }

  free(store_ptr->items);
  free(store_ptr->path);
  free(store_ptr);
}

void
ladish_recent_store_use_item(
  ladish_recent_store_handle store_handle,
  const char * item_const)
{
  char * item;
  char * item_save;
  unsigned int i;

  for (i = 0; i < store_ptr->max_items && store_ptr->items[i] != NULL; i++)
  {
    if (strcmp(store_ptr->items[i], item_const) == 0)
    { /* already known item */

      /* if the item is already the most recent one, there is nothing to do */
      if (i != 0)
      {
        item = store_ptr->items[i];
        store_ptr->items[i] = NULL; /* mark end for reorder loop */
        goto reorder;
      }

      return;
    }
  }

  /* new item */
  item = strdup(item_const);
  if (item == NULL)
  {
    log_error("strdup() failed for recent item '%s'", item_const);
    return;
  }

reorder:
  /* we have valid non-NULL item pointer before the iteration begins */
  /* the item pointer is checked for NULL because that means either
     end of used items or a mark created when already known item
     was removed from the array */
  for (i = 0; i < store_ptr->max_items && item != NULL; i++)
  {
    item_save = store_ptr->items[i];
    store_ptr->items[i] = item;
    item = item_save;
  }

  if (item != NULL)
  { /* eventually free the oldest item */
    free(item);
  }

  ladish_recent_store_save(store_ptr);
}

bool
ladish_recent_store_check_known(
  ladish_recent_store_handle store_handle,
  const char * item)
{
  unsigned int i;

  for (i = 0; i < store_ptr->max_items && store_ptr->items[i] != NULL; i++)
  {
    if (strcmp(store_ptr->items[i], item) == 0)
    {
      return true;
    }
  }

  return false;
}

void
ladish_recent_store_iterate_items(
  ladish_recent_store_handle store_handle,
  void * callback_context,
  bool (* callback)(void * callback_context, const char * item))
{
  unsigned int i;

  for (i = 0; i < store_ptr->max_items && store_ptr->items[i] != NULL; i++)
  {
    if (!callback(callback_context, store_ptr->items[i]))
    {
      break;
    }
  }
}

#undef store_ptr
