/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010,2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the settings storage
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

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "dbus/helpers.h"
#include "dbus_constants.h"
#include "common/catdup.h"
#include "common/dirhelpers.h"

#define STORAGE_BASE_DIR "/.ladish/conf/"

extern const struct cdbus_interface_descriptor g_interface;

static const char * g_dbus_unique_name;
static cdbus_object_path g_object;
static bool g_quit;

struct pair
{
  struct list_head siblings;
  uint64_t version;
  char * key;
  char * value;
  bool stored;
};

struct list_head g_pairs;

static bool connect_dbus(void)
{
  int ret;

  dbus_error_init(&cdbus_g_dbus_error);

  cdbus_g_dbus_connection = dbus_bus_get(DBUS_BUS_SESSION, &cdbus_g_dbus_error);
  if (dbus_error_is_set(&cdbus_g_dbus_error))
  {
    log_error("Failed to get bus: %s", cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    goto fail;
  }

  g_dbus_unique_name = dbus_bus_get_unique_name(cdbus_g_dbus_connection);
  if (g_dbus_unique_name == NULL)
  {
    log_error("Failed to read unique bus name");
    goto unref_connection;
  }

  log_info("Connected to local session bus, unique name is \"%s\"", g_dbus_unique_name);

  ret = dbus_bus_request_name(cdbus_g_dbus_connection, CONF_SERVICE_NAME, DBUS_NAME_FLAG_DO_NOT_QUEUE, &cdbus_g_dbus_error);
  if (ret == -1)
  {
    log_error("Failed to acquire bus name: %s", cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    goto unref_connection;
  }

  if (ret == DBUS_REQUEST_NAME_REPLY_EXISTS)
  {
    log_error("Requested connection name already exists");
    goto unref_connection;
  }

  g_object = cdbus_object_path_new(CONF_OBJECT_PATH, &g_interface, NULL, NULL);
  if (g_object == NULL)
  {
    goto unref_connection;
  }

  if (!cdbus_object_path_register(cdbus_g_dbus_connection, g_object))
  {
    goto destroy_control_object;
  }

  return true;

destroy_control_object:
  cdbus_object_path_destroy(cdbus_g_dbus_connection, g_object);
unref_connection:
  dbus_connection_unref(cdbus_g_dbus_connection);

fail:
  return false;
}

static void disconnect_dbus(void)
{
  cdbus_object_path_destroy(cdbus_g_dbus_connection, g_object);
  dbus_connection_unref(cdbus_g_dbus_connection);
}

void term_signal_handler(int signum)
{
  log_info("Caught signal %d (%s), terminating", signum, strsignal(signum));
  g_quit = true;
}

bool install_term_signal_handler(int signum, bool ignore_if_already_ignored)
{
  sig_t sigh;

  sigh = signal(signum, term_signal_handler);
  if (sigh == SIG_ERR)
  {
    log_error("signal() failed to install handler function for signal %d.", signum);
    return false;
  }

  if (sigh == SIG_IGN && ignore_if_already_ignored)
  {
    signal(SIGTERM, SIG_IGN);
  }

  return true;
}

int main(int argc, char ** argv)
{
  if (getenv("HOME") == NULL)
  {
    log_error("Environment variable HOME not set");
    return 1;
  }

  INIT_LIST_HEAD(&g_pairs);

  install_term_signal_handler(SIGTERM, false);
  install_term_signal_handler(SIGINT, true);

  if (!connect_dbus())
  {
    log_error("Failed to connect to D-Bus");
    return 1;
  }

  while (!g_quit)
  {
    dbus_connection_read_write_dispatch(cdbus_g_dbus_connection, 50);
  }

  disconnect_dbus();
  return 0;
}

static struct pair * create_pair(const char * key, const char * value)
{
  struct pair * pair_ptr;

  pair_ptr = malloc(sizeof(struct pair));
  if (pair_ptr == NULL)
  {
    log_error("malloc() failed to allocate memory for pair struct");
    return NULL;
  }

  pair_ptr->key = strdup(key);
  if (pair_ptr->key == NULL)
  {
    log_error("strdup(\"%s\") failed for key", key);
    free(pair_ptr);
    return NULL;
  }

  if (value != NULL)
  {
    pair_ptr->value = strdup(value);
    if (pair_ptr->value == NULL)
    {
      log_error("strdup(\"%s\") failed for value", value);
      free(pair_ptr->key);
      free(pair_ptr);
      return NULL;
    }
  }
  else
  {
    /* Caller will fill this shortly */
    pair_ptr->value = NULL;
  }

  pair_ptr->version = 1;
  pair_ptr->stored = false;

  list_add_tail(&pair_ptr->siblings, &g_pairs);

  return pair_ptr;
}

static bool store_pair(struct pair * pair_ptr)
{
  char * dirpath;
  char * filepath;
  int fd;
  size_t len;
  ssize_t written;

  dirpath = catdupv(getenv("HOME"), STORAGE_BASE_DIR, pair_ptr->key, NULL);
  if (dirpath == NULL)
  {
    return false;
  }

  if (!ensure_dir_exist(dirpath, 0700))
  {
    free(dirpath);
    return false;
  }

  filepath = catdup(dirpath, "/value");
  free(dirpath);
  if (filepath == NULL)
  {
    return false;
  }

  fd = creat(filepath, 0700);
  if (fd == -1)
  {
    log_error("Failed to create \"%s\": %d (%s)", filepath, errno, strerror(errno));
    free(filepath);
    return false;
  }

  len = strlen(pair_ptr->value);

  written = write(fd, pair_ptr->value, len);
  if (written < 0)
  {
    log_error("Failed to write() to \"%s\": %d (%s)", filepath, errno, strerror(errno));
    free(filepath);
    return false;
  }

  if ((size_t)written != len)
  {
    log_error("write() to \"%s\" returned %zd instead of %zu", filepath, written, len);
    free(filepath);
    return false;
  }

  close(fd);
  free(filepath);

  pair_ptr->stored = true;
  
  return true;
}

static struct pair * load_pair(const char * key)
{
  struct pair * pair_ptr;
  char * path;
  struct stat st;
  int fd;
  char * buffer;
  ssize_t bytes_read;

  path = catdupv(getenv("HOME"), STORAGE_BASE_DIR, key, "/value", NULL);
  if (path == NULL)
  {
    return false;
  }

  if (stat(path, &st) != 0)
  {
    log_error("Failed to stat \"%s\": %d (%s)", path, errno, strerror(errno));
    free(path);
    return false;
  }

  if (!S_ISREG(st.st_mode))
  {
    log_error("\"%s\" is not a regular file.", path);
    free(path);
    return false;
  }

  fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    log_error("Failed to open \"%s\": %d (%s)", path, errno, strerror(errno));
    free(path);
    return false;
  }

  buffer = malloc((size_t)st.st_size + 1);
  if (buffer == NULL)
  {
    log_error("malloc() failed to allocate %zu bytes of memory for value", (size_t)st.st_size + 1);
    close(fd);
    free(path);
    return false;
  }

  bytes_read = read(fd, buffer, st.st_size);
  if (bytes_read < 0)
  {
    log_error("Failed to read() from \"%s\": %d (%s)", path, errno, strerror(errno));
    free(buffer);
    close(fd);
    free(path);
    return false;
  }

  if (bytes_read != st.st_size)
  {
    log_error("read() from \"%s\" returned %zd instead of %llu", path, bytes_read, (unsigned long long)st.st_size);
    free(buffer);
    close(fd);
    free(path);
    return false;
  }

  buffer[st.st_size] = 0;

  pair_ptr = create_pair(key, NULL);
  if (pair_ptr == NULL)
  {
    free(buffer);
    close(fd);
    free(path);
    return false;
  }

  pair_ptr->value = buffer;

  close(fd);
  free(path);

  return pair_ptr;
}

static struct pair * find_pair(const char * key)
{
  struct list_head * node_ptr;
  struct pair * pair_ptr;

  list_for_each(node_ptr, &g_pairs)
  {
    pair_ptr = list_entry(node_ptr, struct pair, siblings);
    if (strcmp(pair_ptr->key, key) == 0)
    {
      return pair_ptr;
    }
  }

  return NULL;
}

static void emit_changed(struct pair * pair_ptr)
{
  cdbus_signal_emit(
    cdbus_g_dbus_connection,
    CONF_OBJECT_PATH,
    CONF_IFACE,
    "changed",
    "sst",
    &pair_ptr->key,
    &pair_ptr->value,
    &pair_ptr->version);
}

/***************************************************************************/
/* D-Bus interface implementation */

static void conf_set(struct cdbus_method_call * call_ptr)
{
  const char * key;
  const char * value;
  struct pair * pair_ptr;
  char * buffer;
  bool store;

  if (!dbus_message_get_args(
        call_ptr->message,
        &cdbus_g_dbus_error,
        DBUS_TYPE_STRING, &key,
        DBUS_TYPE_STRING, &value,
        DBUS_TYPE_INVALID))
  {
    cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  log_info("set '%s' <- '%s'", key, value);

  pair_ptr = find_pair(key);
  if (pair_ptr == NULL)
  {
    pair_ptr = create_pair(key, value);
    if (pair_ptr == NULL)
    {
      cdbus_error(call_ptr, DBUS_ERROR_FAILED, "Memory allocation failed");
      return;
    }

    emit_changed(pair_ptr);

    store = true;
  }
  else
  {
    store = strcmp(pair_ptr->value, value) != 0;
    if (store)
    {
      buffer = strdup(value);
      if (buffer == NULL)
      {
        cdbus_error(call_ptr, DBUS_ERROR_FAILED, "Memory allocation failed. strdup(\"%s\") failed for value", value);
        return;
      }
      free(pair_ptr->value);
      pair_ptr->value = buffer;
      pair_ptr->version++;
      pair_ptr->stored = false; /* mark that new value was not stored on disk yet */

      emit_changed(pair_ptr);
    }
    else if (!pair_ptr->stored) /* if store to disk failed last time, retry */
    {
      store = true;
    }
  }

  if (store)
  {
    if (!store_pair(pair_ptr))
    {
      cdbus_error(call_ptr, DBUS_ERROR_FAILED, "Storing the value of key '%s' to disk failed", pair_ptr->key);
      return;
    }
  }

  cdbus_method_return_new_single(call_ptr, DBUS_TYPE_UINT64, &pair_ptr->version);
}

static void conf_get(struct cdbus_method_call * call_ptr)
{
  const char * key;
  struct pair * pair_ptr;

  if (!dbus_message_get_args(
        call_ptr->message,
        &cdbus_g_dbus_error,
        DBUS_TYPE_STRING, &key,
        DBUS_TYPE_INVALID))
  {
    cdbus_error(call_ptr, DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  pair_ptr = find_pair(key);
  if (pair_ptr == NULL)
  {
    pair_ptr = load_pair(key);
    if (pair_ptr == NULL)
    {
      cdbus_error(call_ptr, LADISH_DBUS_ERROR_KEY_NOT_FOUND, "Key '%s' not found", key);
      return;
    }
  }

  log_info("get '%s' -> '%s'", key, pair_ptr->value);

  cdbus_method_return_new_valist(
    call_ptr,
    DBUS_TYPE_STRING, &pair_ptr->value,
    DBUS_TYPE_UINT64, &pair_ptr->version,
    DBUS_TYPE_INVALID);
}

static void conf_exit(struct cdbus_method_call * call_ptr)
{
  log_info("Exit command received through D-Bus");
  g_quit = true;
  cdbus_method_return_new_void(call_ptr);
}

CDBUS_METHOD_ARGS_BEGIN(set, "Set conf value")
  CDBUS_METHOD_ARG_DESCRIBE_IN("key", DBUS_TYPE_STRING_AS_STRING, "")
  CDBUS_METHOD_ARG_DESCRIBE_IN("value", DBUS_TYPE_STRING_AS_STRING, "")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("version", DBUS_TYPE_UINT64_AS_STRING, "")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(get, "Get conf value")
  CDBUS_METHOD_ARG_DESCRIBE_IN("key", DBUS_TYPE_STRING_AS_STRING, "")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("value", DBUS_TYPE_STRING_AS_STRING, "")
  CDBUS_METHOD_ARG_DESCRIBE_OUT("version", DBUS_TYPE_UINT64_AS_STRING, "")
CDBUS_METHOD_ARGS_END

CDBUS_METHOD_ARGS_BEGIN(exit, "Tell conf D-Bus service to exit")
CDBUS_METHOD_ARGS_END

CDBUS_METHODS_BEGIN
  CDBUS_METHOD_DESCRIBE(set, conf_set)
  CDBUS_METHOD_DESCRIBE(get, conf_get)
  CDBUS_METHOD_DESCRIBE(exit, conf_exit)
CDBUS_METHODS_END

CDBUS_SIGNAL_ARGS_BEGIN(changed, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("key", DBUS_TYPE_STRING_AS_STRING, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("value", DBUS_TYPE_STRING_AS_STRING, "")
  CDBUS_SIGNAL_ARG_DESCRIBE("version", DBUS_TYPE_UINT64_AS_STRING, "")
CDBUS_SIGNAL_ARGS_END

CDBUS_SIGNALS_BEGIN
  CDBUS_SIGNAL_DESCRIBE(changed)
CDBUS_SIGNALS_END

CDBUS_INTERFACE_BEGIN(g_interface, CONF_IFACE)
  CDBUS_INTERFACE_DEFAULT_HANDLER
  CDBUS_INTERFACE_EXPOSE_METHODS
  CDBUS_INTERFACE_EXPOSE_SIGNALS
CDBUS_INTERFACE_END
