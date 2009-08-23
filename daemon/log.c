/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009 Nedko Arnaudov <nedko@arnaudov.name>
 * Copyright (C) 2008 Marc-Olivier Barre
 *
 **************************************************************************
 * This file contains code that implements logging functionality
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

#include <errno.h>
#include <time.h>
#include <stdarg.h>

#include "../catdup.h"
#include "dirhelpers.h"

#define DEFAULT_XDG_LOG "/.log"
#define LASH_XDG_SUBDIR "/" BASE_NAME
#define LASH_XDG_LOG "/" BASE_NAME ".log"

FILE * g_logfile;

void lash_log_init() __attribute__ ((constructor));
void lash_log_init()
{
  char * log_filename;
  size_t log_len;
  char * lash_log_dir;
  size_t lash_log_dir_len; /* without terminating '\0' char */
  const char * home_dir;
  char * xdg_log_home;

  home_dir = getenv("HOME");
  if (home_dir == NULL)
  {
    lash_error("Environment variable HOME not set");
    goto exit;
  }

  xdg_log_home = catdup(home_dir, DEFAULT_XDG_LOG);
  if (xdg_log_home == NULL)
  {
    lash_error("catdup failed for '%s' and '%s'", home_dir, DEFAULT_XDG_LOG);
    goto exit;
  }

  lash_log_dir = catdup(xdg_log_home, LASH_XDG_SUBDIR);
  if (lash_log_dir == NULL)
  {
    lash_error("catdup failed for '%s' and '%s'", home_dir, LASH_XDG_SUBDIR);
    goto free_log_home;
  }

  if (!ensure_dir_exist(xdg_log_home, 0700))
  {
    goto free_log_dir;
  }

  if (!ensure_dir_exist(lash_log_dir, 0700))
  {
    goto free_log_dir;
  }

  lash_log_dir_len = strlen(lash_log_dir);

  log_len = strlen(LASH_XDG_LOG);

  log_filename = malloc(lash_log_dir_len + log_len + 1);
  if (log_filename == NULL)
  {
    lash_error("Out of memory");
    goto free_log_dir;
  }

  memcpy(log_filename, lash_log_dir, lash_log_dir_len);
  memcpy(log_filename + lash_log_dir_len, LASH_XDG_LOG, log_len);
  log_filename[lash_log_dir_len + log_len] = 0;

  g_logfile = fopen(log_filename, "a");
  if (g_logfile == NULL)
  {
    lash_error("Cannot open jackdbus log file \"%s\": %d (%s)\n", log_filename, errno, strerror(errno));
  }

  free(log_filename);

free_log_dir:
  free(lash_log_dir);

free_log_home:
  free(xdg_log_home);

exit:
  return;
}

void lash_log_uninit()  __attribute__ ((destructor));
void lash_log_uninit()
{
  if (g_logfile != NULL)
  {
    fclose(g_logfile);
  }
}

void
lash_log(
  unsigned int level,
  const char * format,
  ...)
{
  va_list ap;
  FILE * stream;
  time_t timestamp;
  char timestamp_str[26];

  if (g_logfile != NULL)
  {
    stream = g_logfile;
  }
  else
  {
    switch (level)
    {
    case LASH_LOG_LEVEL_DEBUG:
    case LASH_LOG_LEVEL_INFO:
      stream = stdout;
      break;
    case LASH_LOG_LEVEL_WARN:
    case LASH_LOG_LEVEL_ERROR:
    case LASH_LOG_LEVEL_ERROR_PLAIN:
    default:
      stream = stderr;
    }
  }

  time(&timestamp);
  ctime_r(&timestamp, timestamp_str);
  timestamp_str[24] = 0;

  fprintf(stream, "%s: ", timestamp_str);

  va_start(ap, format);
  vfprintf(stream, format, ap);
  fflush(stream);
  va_end(ap);
}
