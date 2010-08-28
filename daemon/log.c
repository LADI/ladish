/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
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
#include <sys/stat.h>

#include "../catdup.h"
#include "dirhelpers.h"

#define DEFAULT_XDG_LOG "/.log"
#define LADISH_XDG_SUBDIR "/" BASE_NAME
#define LADISH_XDG_LOG "/" BASE_NAME ".log"

static ino_t g_log_file_ino;
static FILE * g_logfile;
static char * g_log_filename;

static bool ladish_log_open(void)
{
    struct stat st;
    int ret;
    int retry;

    if (g_logfile != NULL)
    {
        ret = stat(g_log_filename, &st);
        if (ret != 0 || g_log_file_ino != st.st_ino)
        {
            fclose(g_logfile);
        }
        else
        {
            return true;
        }
    }

    for (retry = 0; retry < 10; retry++)
    {
        g_logfile = fopen(g_log_filename, "a");
        if (g_logfile == NULL)
        {
            fprintf(stderr, "Cannot open ladishd log file \"%s\": %d (%s)\n", g_log_filename, errno, strerror(errno));
            return false;
        }

        ret = stat(g_log_filename, &st);
        if (ret == 0)
        {
            g_log_file_ino = st.st_ino;
            return true;
        }

        fclose(g_logfile);
        g_logfile = NULL;
    }

    fprintf(stderr, "Cannot stat just opened ladishd log file \"%s\": %d (%s). %d retries\n", g_log_filename, errno, strerror(errno), retry);
    return false;
}

void ladish_log_init() __attribute__ ((constructor));
void ladish_log_init()
{
  char * ladish_log_dir;
  const char * home_dir;
  char * xdg_log_home;

  home_dir = getenv("HOME");
  if (home_dir == NULL)
  {
    log_error("Environment variable HOME not set");
    goto exit;
  }

  xdg_log_home = catdup(home_dir, DEFAULT_XDG_LOG);
  if (xdg_log_home == NULL)
  {
    log_error("catdup failed for '%s' and '%s'", home_dir, DEFAULT_XDG_LOG);
    goto exit;
  }

  ladish_log_dir = catdup(xdg_log_home, LADISH_XDG_SUBDIR);
  if (ladish_log_dir == NULL)
  {
    log_error("catdup failed for '%s' and '%s'", home_dir, LADISH_XDG_SUBDIR);
    goto free_log_home;
  }

  if (!ensure_dir_exist(xdg_log_home, 0700))
  {
    goto free_log_dir;
  }

  if (!ensure_dir_exist(ladish_log_dir, 0700))
  {
    goto free_log_dir;
  }

  g_log_filename = catdup(ladish_log_dir, LADISH_XDG_LOG);
  if (g_log_filename == NULL)
  {
    log_error("Out of memory");
    goto free_log_dir;
  }

  ladish_log_open();

free_log_dir:
  free(ladish_log_dir);

free_log_home:
  free(xdg_log_home);

exit:
  return;
}

void ladish_log_uninit()  __attribute__ ((destructor));
void ladish_log_uninit()
{
  if (g_logfile != NULL)
  {
    fclose(g_logfile);
  }

  free(g_log_filename);
}

void
ladish_log(
  unsigned int level,
  const char * format,
  ...)
{
  va_list ap;
  FILE * stream;
  time_t timestamp;
  char timestamp_str[26];

  if (g_logfile != NULL && ladish_log_open())
  {
    stream = g_logfile;
  }
  else
  {
    switch (level)
    {
    case LADISH_LOG_LEVEL_DEBUG:
    case LADISH_LOG_LEVEL_INFO:
      stream = stdout;
      break;
    case LADISH_LOG_LEVEL_WARN:
    case LADISH_LOG_LEVEL_ERROR:
    case LADISH_LOG_LEVEL_ERROR_PLAIN:
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
