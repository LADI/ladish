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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#include "store.h"
#include "file.h"
#include "../common/safety.h"
#include "../common/debug.h"
#include "../dbus/method.h"
#include "../lash_compat/liblash/lash/types.h"

#define STORE_INFO_FILE ".store_info"

struct _store_key
{
  struct list_head  siblings;
  char             *name;
};

struct _store_config
{
  struct list_head  siblings;
  char             *key;
  void             *value;
  size_t            value_size;
  char              type;
};

store_t *
store_new(void)
{
  store_t *store;

  store = lash_calloc(1, sizeof(store_t));

  INIT_LIST_HEAD(&store->keys);
  INIT_LIST_HEAD(&store->removed_keys);
  INIT_LIST_HEAD(&store->unstored_configs);

  return store;
}

static void
store_destroy_key_list(struct list_head *key_list);

static void
store_destroy_configs(store_t *store);

void
store_destroy(store_t *store)
{
  if (store) {
    lash_free(&store->dir);
    store_destroy_key_list(&store->keys);
    store_destroy_key_list(&store->removed_keys);
    store_destroy_configs(store);

    free(store);
  }
}

static void
store_config_destroy(struct _store_config *config)
{
  if (config) {
    list_del(&config->siblings);
    lash_free(&config->key);
    lash_free(&config->value);
    free(config);
  }
}

static void
store_destroy_configs(store_t *store)
{
  if (store) {
    struct list_head *node, *next;
    struct _store_config *config;

    list_for_each_safe (node, next, &store->unstored_configs) {
      config = list_entry(node, struct _store_config, siblings);
      store_config_destroy(config);
    }
  }
}

static void
store_key_destroy(struct _store_key *key)
{
  if (key) {
    list_del(&key->siblings);
    lash_free(&key->name);
    free(key);
  }
}

static void
store_destroy_key_list(struct list_head *key_list)
{
  if (!list_empty(key_list)) {
    struct list_head *node, *next;
    list_for_each_safe (node, next, key_list) {
      store_key_destroy(list_entry(node, struct _store_key,
                                   siblings));
    }
  }
}

static struct _store_key *
store_key_new(const char *name)
{
  struct _store_key *key;

  key = lash_malloc(1, sizeof(struct _store_key));
  key->name = lash_strdup(name);
  INIT_LIST_HEAD(&key->siblings);

  return key;
}

static __inline__ const char *
store_get_info_filename(store_t *store)
{
  get_store_and_return_fqn(store->dir, STORE_INFO_FILE);
}

static __inline__ const char *
store_get_config_filename(store_t    *store,
                          const char *key)
{
  get_store_and_return_fqn(store->dir, key);
}

bool
store_open(store_t *store)
{
  const char *filename;
  unsigned long i;
  FILE *info_file;
  char *line = NULL;
  size_t line_size = 0;
  char *ptr;
  struct _store_key *key;

  lash_debug("Reading store in directory '%s'", store->dir);

  if (!lash_dir_exists(store->dir)) {
    lash_error("Directory '%s' does not exist", store->dir);
    return false;
  }

  filename = store_get_info_filename(store);

  if (!lash_file_exists(filename)) {
    lash_error("File '%s' does not exist", filename);
    return false;
  }

  /* Open the info file */
  info_file = fopen(filename, "r");
  if (!info_file) {
    lash_error("Cannot open info file for store '%s': %s",
               store->dir, strerror(errno));
    return false;
  }

  /* Read the number of keys */
  if (getline(&line, &line_size, info_file) == -1) {
    lash_error("Error reading from info file for store '%s': %s",
               store->dir, strerror(errno));
    goto fail;
  }

  char *endptr;
  errno = 0;
  store->num_keys = strtol(line, &endptr, 10);
  if (errno) {
    lash_error("Error parsing number of keys: %s", strerror(errno));
    goto fail;
  } else if (endptr && *endptr != '\n') {
    lash_error("Error parsing number of keys: Info file is corrupt");
    goto fail;
  }

  for (i = 0; i < store->num_keys; ++i) {
    if (getline(&line, &line_size, info_file) == -1) {
      lash_error("Error reading from info file for "
                 "store '%s': %s",
                 store->dir, strerror(errno));
      store_destroy_key_list(&store->keys);
      store->num_keys = 0;
      goto fail;
    }

    ptr = strchr(line, '\n');
    if (ptr)
      *ptr = '\0';

    key = store_key_new(line);
    list_add_tail(&key->siblings, &store->keys);
  }

  lash_free(&line);

  if (fclose(info_file) != 0) {
    lash_error("Error closing info file for store '%s': %s",
               store->dir, strerror(errno));
  }

#ifdef LASH_DEBUG
  struct list_head *node;

  if (!list_empty(&store->keys)) {
    lash_debug("Opened store in '%s' with keys:", store->dir);
    list_for_each (node, &store->keys) {
      key = list_entry(node, struct _store_key, siblings);
      lash_debug("  '%s'", key->name);
    }
  } else {
    lash_debug("Opened store in '%s' with no keys",
               store->dir);
  }
#endif

  return true;

fail:
  lash_free(&line);
  fclose(info_file);
  store->num_keys = 0;

  return false;
}

static __inline__ bool
store_write_config(store_t              *store,
                   struct _store_config *config)
{
  int config_file;
  ssize_t written;
  uint32_t size;

  if (config->value_size == 0) {
    lash_error("Config '%s' has a value size of 0", config->key);
    return false;
  }

  config_file =
    creat(store_get_config_filename(store, config->key),
          S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

  if (config_file == -1) {
    lash_error("Error opening config file '%s' for writing in "
               "store '%s': %s",
               config->key, store->dir, strerror(errno));
    return false;
  }

  /* Write the value size in network byte order */
  size = ntohl(config->value_size);
  written = write(config_file, &size, sizeof(uint32_t));
  if (written == -1 || written < sizeof(uint32_t))
    goto fail_write;

  /* Write the value data and type byte */
  written = write(config_file, config->value, config->value_size);
  if (written == -1 || written < config->value_size
      || write(config_file, &config->type, 1) < 1)
    goto fail_write;

  if (close(config_file) == -1) {
    lash_error("Error closing config file '%s' in store '%s': %s",
               config->key, store->dir, strerror(errno));
    return false;
  }

  return true;

fail_write:
  lash_error("Error writing to config file '%s' in store '%s': %s",
             config->key, store->dir, strerror(errno));
  close(config_file);
  return false;
}

static __inline__ bool
store_write_configs(store_t *store)
{
  struct list_head *node, *next;
  struct _store_config *config;
  bool ret = true;

  /* Write the unstored configs to disk */
  list_for_each_safe (node, next, &store->unstored_configs) {
    config = list_entry(node, struct _store_config, siblings);

    if (store_write_config(store, config))
      store_config_destroy(config);
    else if (ret)
      ret = false;
  }

  return ret;
}

static __inline__ bool
store_write_info_file(store_t *store)
{
  FILE *info_file;
  struct list_head *node;
  struct _store_key *key;

  info_file = fopen(store_get_info_filename(store), "w");
  if (!info_file) {
    lash_error("Error opening info file for store '%s': %s",
               store->dir, strerror(errno));
    return false;
  }

  if (fprintf(info_file, "%ld\n", store->num_keys) < 0) {
    lash_error("Error writing to info file for store '%s': %s",
               store->dir, strerror(errno));
    fclose(info_file);
    return false;
  }

  list_for_each (node, &store->keys) {
    key = list_entry(node, struct _store_key, siblings);
    if (fprintf(info_file, "%s\n", key->name) < 0) {
      lash_error("Error writing to info file for store "
                 "'%s': %s",
                 store->dir, strerror(errno));
      fclose(info_file);
      return false;
    }
  }

  if (fclose(info_file) == EOF) {
    lash_error("Error closing info file for store '%s': %s",
               store->dir, strerror(errno));
    return false;
  }

  return true;
}

static __inline__ void
store_delete_removed_configs(store_t *store)
{
  struct list_head *node, *next;
  struct _store_key *key;
  const char *filename;

  list_for_each_safe (node, next, &store->removed_keys) {
    key = list_entry(node, struct _store_key, siblings);

    filename = store_get_config_filename(store, key->name);

    lash_debug("Removing config with key '%s', filename '%s'",
               key, filename);

    if (lash_file_exists(filename)) {
      if (unlink(filename) == -1) {
        lash_error("Cannot remove file '%s': %s",
                   filename, strerror(errno));
        continue;
      }
    }

    store_key_destroy(key);
  }
}

bool
store_write(store_t *store)
{
  if (list_empty(&store->unstored_configs))
    return true;

  if (!lash_dir_exists(store->dir))
    lash_create_dir(store->dir);

  /* Write the config files */
  if (!store_write_configs(store)) {
    lash_error("Error writing configs");
    return false;
  }

  /* Write the info file */
  if (!store_write_info_file(store)) {
    lash_error("Error writing info file");
    return false;
  }

  /* Only delete the unstored data now that we're sure it's
     all been written (or at least given to the OS) */
  store_destroy_configs(store);

  store_delete_removed_configs(store);

  lash_debug("Wrote data set to disk");
  return true;
}

bool
store_set_config(store_t    *store,
                 const char *key_name,
                 const void *value,
                 size_t      size,
                 int         type)
{
  if (!key_name || !key_name[0] || !value) {
    lash_error("Invalid config parameter(s)");
    return false;
  }

  /* This condition deserves its own message */
  if (size < 1) {
    lash_error("Config data size is 0");
    return false;
  }

  struct list_head *node;
  struct _store_key *key;
  struct _store_config *config;
  bool found;

  /* Check if the config's key is already in the store's key list */
  found = false;
  list_for_each (node, &store->keys) {
    key = list_entry(node, struct _store_key, siblings);
    if (strcmp(key->name, key_name) == 0) {
      found = true;
      break;
    }
  }

  /* If the key wasn't found add it to the list */
  if (!found) {
    key = store_key_new(key_name);
    list_add_tail(&key->siblings, &store->keys);
    ++store->num_keys;
  } else
    /* Needs a reset for the next check */
    found = false;

  /* Check whether we're overwriting a previous config */
  list_for_each (node, &store->unstored_configs) {
    config = list_entry(node, struct _store_config, siblings);
    if (strcmp(config->key, key_name) == 0) {
      found = true;
      break;
    }
  }

  /* If a config wasn't found allocate a new one and add it to the list */
  if (!found) {
    config = lash_calloc(1, sizeof(struct _store_config));
    config->value = lash_malloc(1, size);
    list_add_tail(&config->siblings, &store->unstored_configs);
  } else if (config->value_size < size)
    /* Enlarge existing config's buffer if necessary */
    config->value = lash_realloc(config->value, 1, size);

  lash_strset(&config->key, key_name);
  memcpy(config->value, value, size);
  config->value_size = size;
  config->type = (char) type;

  lash_debug("Added key \"%s\" of type '%c' to data set (%u bytes)",
             key_name, config->type, size);

  return true;
}

static __inline__ struct _store_config *
store_get_unstored_config(store_t    *store,
                          const char *key)
{
  struct list_head *node;
  struct _store_config *config;

  list_for_each (node, &store->unstored_configs) {
    config = list_entry(node, struct _store_config, siblings);
    if (strcmp(config->key, key) == 0)
      return config;
  }

  return NULL;
}

static __inline__ bool
store_get_config(store_t     *store,
                 const char  *key,
                 void       **value_ptr,
                 size_t      *size_ptr,
                 int         *type_ptr)
{
  struct _store_config *config;
  const char *filename;
  int config_file;
  ssize_t err;
  uint32_t u;
  size_t size;
  void *value;
  int type;

  /* If there's an unstored config return that */
  config = store_get_unstored_config(store, key);
  if (config) {
    *value_ptr = config->value;
    *size_ptr = config->value_size;
    // TODO: Can we trust the object to always contain a sane type?
    *type_ptr = config->type;
    return true;
  }

  filename = store_get_config_filename(store, key);

  config_file = open(filename, O_RDONLY);
  if (config_file == -1) {
    lash_error("Cannot open config file '%s' for reading: %s",
               filename, strerror(errno));
    return false;
  }

  /* Read the value size from the file's first 2 bytes */
  err = read(config_file, &u, sizeof(uint32_t));
  if (err == -1 || err < sizeof(uint32_t)) {
    lash_error("Cannot read value size from config file '%s': %s",
               filename,
               err == -1 ? strerror(errno) : "Not enough data read");
    goto fail;
  }
  size = ntohl(u);

  if (size == 0) {
    lash_error("Config file '%s' contains a value size of 0", filename);
    goto fail;
  }

  /* Read the config data itself */
  value = lash_malloc(1, size);
  err = read(config_file, value, size);
  if (err == -1 || err < size) {
    lash_error("Cannot read value from config file '%s': %s",
               filename,
               err == -1 ? strerror(errno) : "Not enough data read");
    goto fail_free;
  }

  /* Try to read the value type byte, if that fails assume raw data */
  if (read(config_file, &type, 1) != 1)
    type = LASH_TYPE_RAW;

  /* Only accept API-defined types */
  else if (type != LASH_TYPE_DOUBLE && type != LASH_TYPE_INTEGER
           && type != LASH_TYPE_STRING && type != LASH_TYPE_RAW) {
    lash_error("Config file '%s' contains an invalid type",
               filename);
    goto fail_free;
  }

  /* Done */
  *value_ptr = value;
  *size_ptr = size;
  *type_ptr = type;

  /* This is non-fatal */
  if (close(config_file) == -1) {
    lash_error("Error closing config file '%s': %s",
               filename, strerror(errno));
  }

  return true;

fail_free:
  free(value);
fail:
  close(config_file);
  return false;
}

/* Add an array of configs to a D-Bus message. Used to
   create a LoadDataSet message to be sent to a client. */
bool
store_create_config_array(store_t         *store,
                          DBusMessageIter *iter)
{
  struct list_head *node;
  const char *key_name;
  void *value, *ptr;
  size_t size;
  int type;

  list_for_each (node, &store->keys) {
    key_name = list_entry(node, struct _store_key, siblings)->name;

    if (!store_get_config(store, key_name, &value, &size, &type))
      continue;

    if (type == LASH_TYPE_STRING || type == LASH_TYPE_RAW)
      ptr = &value;
    else
      ptr = value;

    if (!method_iter_append_dict_entry(iter, type, key_name, ptr, size)) {
      lash_error("Failed to append dict entry");
      return false;
    }
  }

  return true;
}

/* EOF */
