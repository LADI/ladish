/* -*- Mode: C ; indent-tabs-mode: t ; tab-width: 8 ; c-basic-offset: 8 -*- */
/*
 *   LASH
 *
 *   Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "procfs.h"
#include "common/debug.h"

static char g_buffer[4096];

static
char *
procfs_get_process_file(
	unsigned long long pid,
	const char * filename)
{
	int fd;
	ssize_t ret;
	size_t max;
	char * buffer_ptr;

	sprintf(g_buffer, "/proc/%llu/%s", pid, filename);

	fd = open(g_buffer, O_RDONLY);
	if (fd == -1)
	{
		return NULL;
	}

	buffer_ptr = g_buffer;
	do
	{
		max = sizeof(g_buffer) - (buffer_ptr - g_buffer);
		ret = read(fd, buffer_ptr, max);
		buffer_ptr += ret;
	}
	while (ret > 0);

	if (ret == 0)
	{
		buffer_ptr = strdup(g_buffer);
		lash_debug("process %llu %s is \"%s\"", pid, filename, buffer_ptr);
	}
	else
	{
		assert(ret == -1);
		buffer_ptr = NULL;
	}

	close(fd);

	return buffer_ptr;
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
		lash_debug("process %llu %s symlink points to \"%s\"", pid, filename, buffer_ptr);
	}
	else
	{
		assert(ret == -1);
		buffer_ptr = NULL;
	}

	close(fd);

	return buffer_ptr;
}

char *
procfs_get_process_cmdline(
	unsigned long long pid)
{
	return procfs_get_process_file(pid, "cmdline");
}

char *
procfs_get_process_cwd(
	unsigned long long pid)
{
	return procfs_get_process_link(pid, "cwd");
}
