/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2010 Nikita Zlobin <nick87720z@gmail.com>
 *
 **************************************************************************
 * This file contains the code that implements the about dialog
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

#include "file.h"

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

char * read_file_contents(const char * filename)
{
  int fd;
  struct stat st;
  char * buffer;

  if (stat(filename, &st) != 0)
  {
    return NULL;
  }

  fd = open(filename, O_RDONLY);
  if (fd == -1)
  {
    return NULL;
  }

  buffer = malloc(st.st_size + 1);
  if (buffer == NULL)
  {
    close(fd);
    return NULL;
  }

  if (read(fd, buffer, (size_t)st.st_size) != (ssize_t)st.st_size)
  {
    free(buffer);
    buffer = NULL;
  }
  else
  {
    buffer[st.st_size] = 0;
  }

  close(fd);

  return buffer;
}
