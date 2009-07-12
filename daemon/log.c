/*
 *   LASH
 *
 *   Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
 *   Copyright (C) 2008 Marc-Olivier Barre
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "../common/debug.h"
#include "../common/safety.h"

#define DEFAULT_XDG_LOG "/.log"
#define LASH_XDG_SUBDIR "/lash"
#define LASH_XDG_LOG "/lash.log"

FILE * g_logfile;

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
			lash_info("Directory \"%s\" does not exist. Creating...", dirname);
			if (mkdir(dirname, mode) != 0)
			{
				lash_error("Failed to create \"%s\" directory: %d (%s)", dirname, errno, strerror(errno));
				return false;
			}
		}
		else
		{
			lash_error("Failed to stat \"%s\": %d (%s)", dirname, errno, strerror(errno));
			return false;
		}
	}
	else
	{
		if (!S_ISDIR(st.st_mode))
		{
			lash_error("\"%s\" exists but is not directory.", dirname);
			return false;
		}
	}

	return true;
}

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

	xdg_log_home = lash_catdup(home_dir, DEFAULT_XDG_LOG);
	lash_log_dir = lash_catdup(xdg_log_home, LASH_XDG_SUBDIR);

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

//free_log_home:
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
