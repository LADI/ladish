/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009,2010,2012 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the code that interfaces procfs
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

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "procfs.h"

#define BUFFER_SIZE 4096

static char g_buffer[BUFFER_SIZE];

static
bool
procfs_get_process_file(
  unsigned long long pid,
  const char * filename,
  char ** buffer_ptr_ptr,
  size_t * size_ptr)
{
  int fd;
  ssize_t ret;
  size_t max;
  char * buffer_ptr;
  char * read_ptr;
  size_t buffer_size;
  size_t used_size;

  sprintf(g_buffer, "/proc/%llu/%s", pid, filename);

  fd = open(g_buffer, O_RDONLY);
  if (fd == -1)
  {
    return false;
  }

  buffer_size = BUFFER_SIZE;
  buffer_ptr = malloc(buffer_size);
  if (buffer_ptr == NULL)
  {
    log_error("malloc failed to allocate buffer with size %zu", buffer_size);
    close(fd);
    return false;
  }

  used_size = 0;
  read_ptr = buffer_ptr;
loop:
  max = buffer_size - used_size;
  if (max < BUFFER_SIZE / 4)
  {
    buffer_size = used_size + BUFFER_SIZE;
    read_ptr = realloc(buffer_ptr, buffer_size);
    if (read_ptr == NULL)
    {
      log_error("realloc failed to allocate buffer with size %zu", buffer_size);
      free(buffer_ptr);
      close(fd);
      return false;
    }

    buffer_ptr = read_ptr;
    read_ptr = buffer_ptr + used_size;
    max = BUFFER_SIZE;
  }

  ret = read(fd, read_ptr, max);
  if (ret > 0)
  {
    ASSERT((size_t)ret <= max);
    read_ptr += ret;
    used_size += ret;
    ASSERT(used_size <= buffer_size);
    goto loop;
  }

  close(fd);

  if (ret < 0)
  {
    ASSERT(ret == -1);
    free(buffer_ptr);
    return false;
  }

  if (used_size == buffer_size)
  {
    read_ptr = realloc(buffer_ptr, buffer_size + 1);
    if (read_ptr == NULL)
    {
      log_error("realloc failed to allocate buffer with size %zu", buffer_size + 1);
      free(buffer_ptr);
      return false;
    }

    buffer_ptr = read_ptr;
  }

  buffer_ptr[used_size] = 0;

  *buffer_ptr_ptr = buffer_ptr;
  *size_ptr = used_size;

  return true;
}

static
char *
procfs_get_process_link(
  unsigned long long pid,
  const char * filename)
{
  int fd;
  ssize_t ret;
  char * buffer_ptr;

  sprintf(g_buffer, "/proc/%llu/%s", pid, filename);

  fd = open(g_buffer, O_RDONLY);
  if (fd == -1)
  {
    return NULL;
  }

  ret = readlink(g_buffer, g_buffer, sizeof(g_buffer));
  if (ret != 0)
  {
    g_buffer[ret] = 0;
    buffer_ptr = strdup(g_buffer);
    log_debug("process %llu %s symlink points to \"%s\"", pid, filename, buffer_ptr);
  }
  else
  {
    ASSERT(ret == -1);
    buffer_ptr = NULL;
  }

  close(fd);

  return buffer_ptr;
}

bool
procfs_get_process_cmdline(
  unsigned long long pid,
  int * argc_ptr,
  char *** argv_ptr)
{
  char * cmdline_ptr;
  char * temp_ptr;
  size_t cmdline_size;
  int i;
  int argc;
  char ** argv;

  if (!procfs_get_process_file(pid, "cmdline", &cmdline_ptr, &cmdline_size))
  {
    return false;
  }

  argc = 0;
  temp_ptr = cmdline_ptr;

  while ((size_t)(temp_ptr - cmdline_ptr) < cmdline_size)
  {
    if (*temp_ptr == 0)
    {
      argc++;
    }

    temp_ptr++;
  }

  ASSERT(*(temp_ptr - 1) == 0); /* the last nul char */

  argv = malloc((argc + 1) * sizeof(char *));
  if (argv == NULL)
  {
    free(cmdline_ptr);
    return false;
  }

  temp_ptr = cmdline_ptr;

  for (i = 0; i < argc; i++)
  {
    ASSERT((size_t)(temp_ptr - cmdline_ptr) < cmdline_size);

    argv[i] = strdup(temp_ptr);
    if (argv[i] == NULL)
    {
      /* rollback */
      while (i > 0)
      {
        i--;
        free(argv[i]);
      }

      free(argv);
      free(cmdline_ptr);
      return false;
    }

    temp_ptr += strlen(temp_ptr) + 1;
  }

  /* Make sure that the array is NULL terminated */
  argv[argc] = NULL;

  *argc_ptr = argc;
  *argv_ptr = argv;

  free(cmdline_ptr);

  return true;
}

char *
procfs_get_process_cwd(
  unsigned long long pid)
{
  return procfs_get_process_link(pid, "cwd");
}

unsigned long long
procfs_get_process_parent(
  unsigned long long pid)
{
  char * buffer_ptr;
  size_t buffer_size;
  char * begin;
  char * end;
  unsigned long long ppid;

  if (!procfs_get_process_file(pid, "status", &buffer_ptr, &buffer_size))
  {
    return 0;
  }

  begin = strstr(buffer_ptr, "\nPPid:\t");
  if (begin == NULL)
  {
    log_error("parent pid not parsed for %llu", pid);
    log_error("-----------------------------");
    log_error("%s", buffer_ptr);
    log_error("-----------------------------");
    free(buffer_ptr);
    return 0;
  }

  begin += 7;

  end = strchr(begin, '\n');
  if (end == NULL)
  {
    log_error("parent pid not parsed for %llu (end)", pid);
    log_error("-----------------------------");
    log_error("%s", buffer_ptr);
    log_error("-----------------------------");
    free(buffer_ptr);
    return 0;
  }

  *end = 0;
  //log_info("parent pid is %s", begin);

  errno = 0;
  ppid = strtoull(begin, &end, 10);
  if (errno != 0)
  {
    log_error("strtoull failed to convert '%s' to integer", begin);
    ppid = 0;
  }
  else
  {
    //log_info("parent pid is %llu", ppid);
  }

  /* avoid infinite cycles (should not happen because init has pid 1 and parent 0) */
  if (ppid == pid)
  {
    ppid = 0;
  }

  free(buffer_ptr);

  return ppid;
}
