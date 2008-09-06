/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *   Copyright (C) 2002 Robert Ham <rah@bash.sh>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "file.h"
#include "common/safety.h"
#include "common/debug.h"

bool
lash_file_exists(const char *file)
{
	struct stat filestat;

	if (stat(file, &filestat) != -1)
		return true;

	return false;
}

bool
lash_dir_exists(const char *dir)
{
	if (dir) {
		DIR *dirstream = opendir(dir);
		if (dirstream) {
			closedir(dirstream);
			return true;
		}
	}

	return false;
}

const char *
lash_get_fqn(const char *param_dir,
             const char *param_file)
{
	static char *fqn = NULL;
	static size_t fqn_size = 48;
	size_t str_size;
	char *dir, *file;

	dir = lash_strdup(param_dir);
	file = lash_strdup(param_file);

	if (!fqn)
		fqn = lash_malloc(1, fqn_size);

	str_size = strlen(dir) + 1 + strlen(file) + 1;

	if (str_size > fqn_size) {
		fqn_size = str_size;
		fqn = lash_realloc(fqn, 1, fqn_size);
	}

	sprintf(fqn, "%s/%s", dir, file);

	free(dir);
	free(file);

	return fqn;
}

char *
lash_dup_fqn(
	const char * dir,
	const char * append)
{
	char * fqn;
	size_t size_dir;
	size_t size_append;

	size_dir = strlen(dir);
	size_append = strlen(append);
 
	fqn = lash_malloc(1, size_dir + 1 + size_append + 1);
 
	memcpy(fqn, dir, size_dir);
	fqn[size_dir] = '/';
	memcpy(fqn + size_dir + 1, append, size_append);
	fqn[size_dir + 1 + size_append] = 0;
 
	return fqn;
}

void
lash_create_dir(const char *dir)
{
	char *parent, *ptr;
	DIR *dirstream;
	struct stat parentstat;

	dirstream = opendir(dir);
	if (dirstream) {
		closedir(dirstream);
		return;
	}

	if (errno == EACCES) {
		lash_error("Cannot read directory %s: Permission denied", dir);
		return;
	}

	/* Try and create the parent */
	parent = lash_strdup(dir);
	ptr = strrchr(parent, '/');
	if (ptr) {
		*ptr = '\0';

		if (parent[0])
			lash_create_dir(parent);
		else {
			ptr[0] = '/';
			ptr[1] = '\0';
		}
	}

	/* Try and create the directory */
	if (stat(parent, &parentstat) == -1) {
		lash_error("Cannot stat parent %s to create directory %s: %s",
		           parent, dir, strerror(errno));
	} else if (mkdir(dir, parentstat.st_mode) == -1) {
		lash_error("Cannot create directory %s: %s",
		           dir, strerror(errno));
	}

	free(parent);
}

bool
lash_dir_empty(const char *dir)
{
	DIR *dirstream;
	struct dirent *entry;
	bool empty = true;

	dirstream = opendir(dir);
	if (!dirstream) {
		lash_error("Cannot open directory %s to check emptiness: %s",
		           dir, strerror(errno));
		return false;
	}

	while ((entry = readdir(dirstream))) {
		if (entry->d_name[0] == '.'
		    && (entry->d_name[1] == '\0'
		        || (entry->d_name[1] == '.'
		            && entry->d_name[2] == '\0')))
			continue;

		empty = false;
		break;
	}

	closedir(dirstream);

	return empty;
}

void
lash_remove_dir(const char *dirarg)
{
	DIR *dirstream;
	struct dirent *entry;
	const char *fqn;
	struct stat stat_info;
	char *dir;

	dir = lash_strdup(dirarg);

	dirstream = opendir(dir);
	if (!dirstream) {
		lash_error("Cannot open directory %s to remove it: %s",
		           dir, strerror(errno));
		free(dir);
		return;
	}

	while ((entry = readdir(dirstream))) {
		if (entry->d_name[0] == '.'
		    && (entry->d_name[1] == '\0'
		        || (entry->d_name[1] == '.'
		            && entry->d_name[2] == '\0')))
			continue;

		fqn = lash_get_fqn(dir, entry->d_name);

		if (stat(fqn, &stat_info) == -1) {
			lash_error("Cannot stat file %s: %s",
			           fqn, strerror(errno));
		} else {
			if (S_ISDIR(stat_info.st_mode)) {
				lash_remove_dir(fqn);
				continue;
			}
		}

		if (unlink(fqn) == -1) {
			lash_error("Cannot unlink file %s: %s",
			           fqn, strerror(errno));
			closedir(dirstream);
			free(dir);
			return;
		}
	}

	closedir(dirstream);

	if (rmdir(dir) == -1) {
		lash_error("Cannot remove directory %s: %s",
		           dir, strerror(errno));
	}

	free(dir);
}

bool
lash_read_text_file(const char  *file_path,
                    char       **ptr)
{
	FILE *file;
	long size;
	char *data;
	bool ret = false;

	file = fopen(file_path, "r");
	if (!file) {
		lash_error("Failed to open '%s' for reading: %s",
		           file_path, strerror(errno));
		return false;
	}

	if (fseek(file, 0, SEEK_END) == -1) {
		lash_error("Failed to set file offset for '%s': %s",
		           file_path, strerror(errno));
		goto exit_close;
	}

	size = ftell(file);
	if (size == -1) {
		lash_error("Failed to obtain file offset for '%s': %s",
		           file_path, strerror(errno));
		goto exit_close;
	}

	if (size == 0) {
		*ptr = NULL;
		ret = true;
		goto exit_close;
	}

	if (fseek(file, 0, SEEK_SET) == -1) {
		lash_error("Failed to set file offset for '%s': %s",
		           file_path, strerror(errno));
		goto exit_close;
	}

	data = lash_malloc(1, size + 1);

	if (fread(data, size, 1, file) != 1) {
		lash_error("Failed to read %ld bytes of data from file '%s'",
		           size, file_path);
		free(data);
		goto exit_close;
	}

	data[size] = 0;
	*ptr = data;

	ret = true;

exit_close:
	if (fclose(file) == EOF)
		lash_error("Failed to close file '%s': %s",
		           file_path, strerror(errno));

	return ret;
}


/* EOF */
