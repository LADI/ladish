/*
 *   LASH
 *    
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

#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>

#include <lash/lash.h>
#include <lash/internal_headers.h>

#include "store.h"
#include "globals.h"

#define STORE_INFO_FILE ".store_info"

void
store_free(store_t * store)
{
	lash_list_t *list;
	lash_config_t *config;

	store_set_dir(store, NULL);

	list = store->keys;
	while (list) {
		free(list->data);
		list = list->next;
	}
	lash_list_free(store->keys);

	list = store->unstored_configs;
	while (list) {
		config = (lash_config_t *) list->data;
		lash_config_destroy(config);
		list = list->next;
	}
	lash_list_free(store->unstored_configs);
}

store_t *
store_new()
{
	store_t *store;

	store = lash_malloc0(sizeof(store_t));
	return store;
}

void
store_destroy(store_t * store)
{
	store_free(store);
	free(store);
}

void
store_set_dir(store_t * store, const char *dir)
{
	set_string_property(store->dir, dir);
}

const char *
store_get_info_filename(store_t * store)
{
	get_store_and_return_fqn(store->dir, STORE_INFO_FILE);
}

const char *
store_get_config_filename(store_t * store, const char *key)
{
	get_store_and_return_fqn(store->dir, key);
}

int
store_open(store_t * store)
{
	unsigned long i;
	FILE *info_file;
	char *line = NULL;
	size_t line_size = 0;
	ssize_t err;
	char *ptr;

	LASH_DEBUGARGS("reading store in dir '%s'", store->dir);

	if (!lash_dir_exists(store->dir))
		return 0;

	if (!lash_file_exists(store_get_info_filename(store)))
		return 0;

	/* open the info file */
	info_file = fopen(store_get_info_filename(store), "r");
	if (!info_file) {
		fprintf(stderr, "%s: could not open info file for store '%s': %s\n",
				__FUNCTION__, store->dir, strerror(errno));
		return 1;
	}

	/* read the number of keys */
	err = getline(&line, &line_size, info_file);
	if (err == -1) {
		fprintf(stderr,
				"%s: error reading from info file for store '%s': %s\n",
				__FUNCTION__, store->dir, strerror(errno));
		if (line)
			free(line);
		return 1;
	}

	store->key_count = strtol(line, NULL, 10);

	for (i = 0; i < store->key_count; i++) {
		err = getline(&line, &line_size, info_file);
		if (err == -1) {
			lash_list_t *list;

			fprintf(stderr,
					"%s: error reading from info file for store '%s': %s\n",
					__FUNCTION__, store->dir, strerror(errno));
			free(line);

			list = store->keys;
			while (list) {
				free(list->data);
				list = list->next;
			}
			lash_list_free(store->keys);
			store->keys = 0;
			store->key_count = 0;

			return 1;
		}

		ptr = strchr(line, '\n');
		if (ptr)
			*ptr = '\0';

		store->keys = lash_list_append(store->keys, lash_strdup(line));
	}

	err = fclose(info_file);
	if (err) {
		fprintf(stderr, "%s: error closing info file for store '%s': %s\n",
				__FUNCTION__, store->dir, strerror(errno));
	}
#ifdef LASH_DEBUG
	{
		lash_list_t *list;

		list = store->keys;

		if (list) {
			LASH_DEBUGARGS("opened store in '%s' with keys: ", store->dir);
			for (; list; list = lash_list_next(list)) {
				LASH_DEBUGARGS("  '%s'", (char *)list->data);
			}
		} else {
			LASH_DEBUGARGS("opened store in '%s' with no keys", store->dir);
		}

	}
#endif

	return 0;
}

void
store_remove_config(store_t * store, const char *key)
{
	lash_list_t *list;
	char *old_key;
	lash_config_t *config;

	/* remove the key */
	for (list = store->keys; list; list = lash_list_next(list)) {
		old_key = (char *)list->data;

		if (strcmp(key, old_key) == 0) {
			store->keys = lash_list_remove(store->keys, old_key);
			store->key_count--;
			free(old_key);

			break;
		}
	}

	/* check if there's one in the unstored configs */
	for (list = store->unstored_configs; list; list = lash_list_next(list)) {
		config = (lash_config_t *) list->data;

		if (strcmp(lash_config_get_key(config), key) == 0) {
			store->unstored_configs =
				lash_list_remove(store->unstored_configs, config);
			lash_config_destroy(config);
			break;
		}
	}

	/* add it to the list of removed configs */
	store->removed_configs =
		lash_list_append(store->removed_configs, lash_strdup(key));
}

void
store_set_config(store_t * store, const lash_config_t * config)
{
	lash_list_t *list;
	char *key;
	lash_config_t *exconfig;

	/* possibly add the new key to the list */
	for (list = store->keys; list; list = lash_list_next(list)) {
		key = (char *)list->data;
		if (strcmp(key, lash_config_get_key(config)) == 0)
			break;
	}

	if (!list) {
		store->keys =
			lash_list_append(store->keys,
							 lash_strdup(lash_config_get_key(config)));
		store->key_count++;
	}

	/* check whether we're overwriting a previous config, and remove it if we are */
	for (list = store->unstored_configs; list; list = lash_list_next(list)) {
		exconfig = (lash_config_t *) list->data;

		if (strcmp(lash_config_get_key(config), lash_config_get_key(exconfig))
			== 0) {
			store->unstored_configs =
				lash_list_remove(store->unstored_configs, exconfig);
			lash_config_destroy(exconfig);
			break;
		}
	}

	/* add it to the unstored list */
	store->unstored_configs =
		lash_list_append(store->unstored_configs, lash_config_dup(config));
}

int
store_write_config(store_t * store, const lash_config_t * config)
{
	int config_file;
	ssize_t written;
	uint32_t size;
	int err;

	config_file =
		creat(store_get_config_filename(store, lash_config_get_key(config)),
			  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	if (config_file == -1) {
		fprintf(stderr,
				"%s: error opening config file '%s' for writing in store '%s'!: %s\n",
				__FUNCTION__, lash_config_get_key(config), store->dir,
				strerror(errno));
		return 1;
	}

	size = htonl(lash_config_get_value_size(config));

	written = write(config_file, &size, sizeof(size));
	if (written == -1 || written < sizeof(size)) {
		fprintf(stderr,
				"%s: error writing to config file '%s' in store '%s'!: %s\n",
				__FUNCTION__, lash_config_get_key(config), store->dir,
				strerror(errno));
		close(config_file);
		return 1;
	}

	if (lash_config_get_value_size(config) > 0) {
		written =
			write(config_file, lash_config_get_value(config),
				  lash_config_get_value_size(config));
		if (written == -1 || written < lash_config_get_value_size(config)) {
			fprintf(stderr,
					"%s: error writing to config file '%s' in store '%s'!: %s\n",
					__FUNCTION__, lash_config_get_key(config), store->dir,
					strerror(errno));
			close(config_file);
			return 1;
		}
	}

	err = close(config_file);
	if (err == -1) {
		fprintf(stderr,
				"%s: error close config file '%s' in store '%s'!: %s\n",
				__FUNCTION__, lash_config_get_key(config), store->dir,
				strerror(errno));
		return 1;
	}

	return 0;
}

int
store_remove_written_config(store_t * store, const char *key)
{
	int err;
	const char *filename;

	filename = store_get_config_filename(store, key);

	err = unlink(filename);
	if (err == -1) {
		fprintf(stderr, "%s: could not unlink config data file '%s': %s\n",
				__FUNCTION__, filename, strerror(errno));
		return 1;
	}

	return 0;
}

int
store_write_configs(store_t * store)
{
	lash_list_t *list, *old_list;
	lash_config_t *config;
	char *key;
	int err;
	int ret_err = 0;

	for (list = store->unstored_configs; list;) {
		config = (lash_config_t *) list->data;

		old_list = list;
		list = lash_list_next(list);

		err = store_write_config(store, config);
		if (err)
			ret_err = 1;
		else
			store->unstored_configs =
				lash_list_remove(store->unstored_configs, old_list);
	}

	/* remove the removed configs */
	list = store->removed_configs;
	for (list = store->removed_configs; list;) {
		key = (list->data);

		old_list = list;
		list = lash_list_next(list);

		err = store_remove_written_config(store, key);

		free(key);

		if (err)
			ret_err = 1;
		else
			store->removed_configs =
				lash_list_remove(store->removed_configs, old_list);
	}

	return ret_err;
}

int
store_write_info_file(store_t * store)
{
	lash_list_t *list;
	char *key;
	FILE *info_file;
	int err;

	info_file = fopen(store_get_info_filename(store), "w");
	if (!info_file) {
		fprintf(stderr, "%s: error opening info file for store '%s'!: %s\n",
				__FUNCTION__, store->dir, strerror(errno));
		return 1;
	}

	err = fprintf(info_file, "%ld\n", store->key_count);
	if (err < 0) {
		fprintf(stderr,
				"%s: error writing to info file for store '%s'!: %s\n",
				__FUNCTION__, store->dir, strerror(errno));
		fclose(info_file);
		return 1;
	}

	list = store->keys;
	while (list) {
		key = (char *)list->data;

		err = fprintf(info_file, "%s\n", key);
		if (err < 0) {
			fprintf(stderr,
					"%s: error writing to info file for store '%s'!: %s\n",
					__FUNCTION__, store->dir, strerror(errno));
			fclose(info_file);
			return 1;
		}

		list = list->next;
	}

	err = fclose(info_file);
	if (err == EOF) {
		fprintf(stderr, "%s: error closing info file for store '%s'!: %s\n",
				__FUNCTION__, store->dir, strerror(errno));
		return 1;
	}

	return 0;
}

void
store_remove_configs(store_t * store)
{
	lash_list_t *list;
	char *key;
	const char *filename;
	int err;

	list = store->removed_configs;
	while (list) {
		key = (char *)list->data;

		filename = store_get_config_filename(store, key);

		LASH_DEBUGARGS("removing config with key '%s', filename '%s'", key,
					   filename);

		if (lash_file_exists(filename)) {
			err = unlink(filename);
			if (err == -1)
				fprintf(stderr, "%s: could not remove file '%s': %s\n",
						__FUNCTION__, filename, strerror(errno));
		}

		free(key);

		list = list->next;
	}

	lash_list_free(store->removed_configs);
	store->removed_configs = NULL;
}

int
store_write(store_t * store)
{
	lash_list_t *list;
	int err;

	if (!store->unstored_configs)
		return 0;

	if (!lash_dir_exists(store->dir))
		lash_create_dir(store->dir);

	/* write the config files */
	err = store_write_configs(store);
	if (err) {
		fprintf(stderr, "%s: error writing configs; returning\n",
				__FUNCTION__);
		return 1;
	}

	/* write the info file */
	err = store_write_info_file(store);
	if (err) {
		fprintf(stderr, "%s: error writing info file; returning\n",
				__FUNCTION__);
		return 1;
	}

	/* only delete the unstored data now we're sure it's all been written
	 * (or at least given to the OS) */
	for (list = store->unstored_configs; list; list = lash_list_next(list)) {
		lash_config_destroy((lash_config_t *) list->data);
	}
	lash_list_free(store->unstored_configs);
	store->unstored_configs = NULL;

	store_remove_configs(store);

	return 0;
}

unsigned long
store_get_key_count(const store_t * store)
{
	return store->key_count;
}

lash_list_t *
store_get_keys(store_t * store)
{
	return store->keys;
}

lash_config_t *
store_get_unstored_config(store_t * store, const char *key)
{
	lash_list_t *list;
	lash_config_t *config;

	for (list = store->unstored_configs; list; list = lash_list_next(list)) {
		config = (lash_config_t *) list->data;

		if (strcmp(lash_config_get_key(config), key) == 0)
			return lash_config_dup(config);

	}

	return NULL;
}

lash_config_t *
store_get_config(store_t * store, const char *key)
{
	uint32_t size;
	void *value = NULL;
	size_t value_size;
	int config_file;
	lash_config_t *config = NULL;
	ssize_t err;

	config = store_get_unstored_config(store, key);
	if (config)
		return config;

	config_file = open(store_get_config_filename(store, key), O_RDONLY);
	if (config_file == -1) {
		fprintf(stderr,
				"%s: could not open config file '%s' for reading!: %s\n",
				__FUNCTION__, store_get_config_filename(store, key),
				strerror(errno));
		return NULL;
	}

	err = read(config_file, &size, sizeof(size));
	if (err == -1 || err < sizeof(size)) {
		fprintf(stderr,
				"%s: error reading value size in config file '%s': %s\n",
				__FUNCTION__, store_get_config_filename(store, key),
				err == -1 ? strerror(errno) : "not enough data read");
		close(config_file);
		return NULL;
	}

	value_size = ntohl(size);
	if (value_size > 0) {
		value = lash_malloc(value_size);

		err = read(config_file, value, value_size);
		if (err == -1 || err < value_size) {
			fprintf(stderr,
					"%s: error reading value size in config file '%s': %s\n",
					__FUNCTION__, store_get_config_filename(store, key),
					err == -1 ? strerror(errno) : "not enough data read");
			close(config_file);
			free(value);
			return NULL;
		}
	}

	err = close(config_file);
	if (err == -1) {
		fprintf(stderr, "%s: error closing config file '%s': %s\n",
				__FUNCTION__, store_get_config_filename(store, key),
				strerror(errno));
	}

	config = lash_config_new();
	lash_config_set_key(config, key);
	if (value_size > 0) {
		lash_config_set_value(config, value, value_size);
		free(value);
	}

	return config;
}

/* EOF */
