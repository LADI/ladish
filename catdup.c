/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the catdup() function
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
#include "catdup.h"

char * catdup(const char * s1, const char * s2)
{
  char * buffer;
  size_t s1_len, s2_len;

  ASSERT(s1 != NULL && s2 != NULL);

  s1_len = strlen(s1);
  s2_len = strlen(s2);

  buffer = malloc(s1_len + s2_len + 1);
  if (buffer == NULL)
  {
    log_error("malloc() failed.");
    return NULL;
  }

  memcpy(buffer, s1, s1_len);
  memcpy(buffer + s1_len, s2, s2_len);
  buffer[s1_len + s2_len] = 0;

  return buffer;
}

char * catdup3(const char * s1, const char * s2, const char * s3)
{
  char * buffer;
  size_t s1_len, s2_len, s3_len;

  ASSERT(s1 != NULL && s2 != NULL && s3 != NULL);

  s1_len = strlen(s1);
  s2_len = strlen(s2);
  s3_len = strlen(s3);

  buffer = malloc(s1_len + s2_len + s3_len + 1);
  if (buffer == NULL)
  {
    log_error("malloc() failed.");
    return NULL;
  }

  memcpy(buffer, s1, s1_len);
  memcpy(buffer + s1_len, s2, s2_len);
  memcpy(buffer + s1_len + s2_len, s3, s3_len);
  buffer[s1_len + s2_len + s3_len] = 0;

  return buffer;
}
