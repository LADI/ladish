/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009,2010,2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the liblash implementaiton
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

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#include "lash/lash.h"

#define LADISH_DEBUG

#include "../../common.h"
#include "../../common/catdup.h"
#include "../../common/dirhelpers.h"
#include "../../common/file.h"
#include "../../log.h"
#include "../../dbus/helpers.h"
#include "../../dbus/error.h"
#include "../../dbus_constants.h"

#define LASH_CONFIG_SUBDIR "/.ladish_lash_dict/"

static dbus_object_path g_object;
extern const struct dbus_interface_descriptor g_interface __attribute__((visibility("hidden")));

struct _lash_client
{
  int flags;
};

static struct _lash_client g_client;

struct _lash_event
{
  enum LASH_Event_Type type;
  char * string;
};

static struct _lash_event g_event = {0, NULL};
static bool g_quit = false;
static bool g_busy = false;     /* whether the app is processing the event */

struct _lash_config
{
  struct list_head siblings;
  char * key;
  size_t size;
  void * value;
};

static LIST_HEAD(g_pending_configs);

static void clean_pending_configs(void)
{
  struct _lash_config * config_ptr;

  while (!list_empty(&g_pending_configs))
  {
    config_ptr = list_entry(g_pending_configs.next, struct _lash_config, siblings);
    list_del(g_pending_configs.next);
    lash_config_destroy(config_ptr);
  }
}

static void save_config(const char * dir,  struct _lash_config * config_ptr)
{
  char * path;
  int fd;
	ssize_t written;

  log_debug("saving dict key '%s'", config_ptr->key);

  path = catdup3(dir, LASH_CONFIG_SUBDIR, config_ptr->key);
  if (path == NULL)
  {
    goto exit;
  }

  fd = creat(path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	if (fd == -1)
  {
    log_error("error creating config file '%s' (%s)", path, strerror(errno));
    goto free;
	}

	written = write(fd, config_ptr->value, config_ptr->size);
	if (written == -1)
  {
    log_error("error writting config file '%s' (%s)", path, strerror(errno));
    goto close;
	}

  if ((size_t)written < config_ptr->size)
  {
    log_error("error writting config file '%s' (%zd instead of %zu)", path, written, config_ptr->size);
    goto close;
  }

  log_debug("saved dict key '%s'", config_ptr->key);

close:
  close(fd);
free:
  free(path);
exit:
  return;
}

static void save_pending_configs(const char * dir)
{
  struct _lash_config * config_ptr;

  if (!ensure_dir_exist_varg(S_IRWXU | S_IRWXG | S_IRWXO, dir, LASH_CONFIG_SUBDIR, NULL))
  {
    log_error("ensure_dir_exist_varg() failed for %s%s", dir, LASH_CONFIG_SUBDIR);
    clean_pending_configs();
    return;
  }

  while (!list_empty(&g_pending_configs))
  {
    config_ptr = list_entry(g_pending_configs.next, struct _lash_config, siblings);
    list_del(g_pending_configs.next);
    save_config(dir, config_ptr);
    lash_config_destroy(config_ptr);
  }
}

static void load_configs(const char * appdir)
{
  char * dirpath;
  char * filepath;
  DIR * dir;
  struct dirent * dentry;
  struct stat st;
  lash_config_t * config_ptr;

  clean_pending_configs();
  ASSERT(list_empty(&g_pending_configs));

  log_debug("Loading configs from '%s'", appdir);

  dirpath = catdup(appdir, LASH_CONFIG_SUBDIR);
  if (dirpath == NULL)
  {
    goto exit;
  }

  dir = opendir(dirpath);
  if (dir == NULL)
  {
    log_error("Cannot open directory '%s': %d (%s)", dirpath, errno, strerror(errno));
    goto free;
  }

  while ((dentry = readdir(dir)) != NULL)
  {
    if (strcmp(dentry->d_name, ".") == 0 ||
        strcmp(dentry->d_name, "..") == 0)
    {
      continue;
    }

    filepath = catdup3(dirpath, "/", dentry->d_name);
    if (filepath == NULL)
    {
      log_error("catdup() failed");
      goto close;
    }

    if (stat(filepath, &st) != 0)
    {
      log_error("failed to stat '%s': %d (%s)", filepath, errno, strerror(errno));
      goto next;
    }

    if (!S_ISREG(st.st_mode))
    {
      log_error("not regular file '%s' with mode is %07o", filepath, st.st_mode);
      goto next;
    }

    config_ptr = lash_config_new_with_key(dentry->d_name);
    if (config_ptr == NULL)
    {
      goto next;
    }

    config_ptr->value = read_file_contents(filepath);
    if (config_ptr->value == NULL)
    {
      log_error("read from '%s' failed", filepath);
      lash_config_destroy(config_ptr);
      goto next;
    }

    config_ptr->size = (size_t)st.st_size;
    list_add_tail(&config_ptr->siblings, &g_pending_configs);

    log_debug("loaded dict key '%s'", dentry->d_name);

  next:
    free(filepath);
  }

close:
  closedir(dir);
free:
  free(dirpath);
exit:
  return;
}

const char * lash_protocol_string(lash_protocol_t protocol)
{
  return "ladish";
}

lash_args_t * lash_extract_args(int * argc, char *** argv)
{
  /* nothing to do, ladish does not pass any specific arguments */
	return NULL;
}

void lash_args_destroy(lash_args_t * args)
{
  /* nothing to do, ladish does not pass any specific arguments */
}

lash_client_t * lash_init(const lash_args_t * args, const char * class, int client_flags, lash_protocol_t protocol)
{
  DBusError error;
  DBusMessage * msg_ptr;
  const char * dbus_unique_name;
  bool ret;
  dbus_uint32_t flags32;

  if ((client_flags & LASH_Server_Interface) != 0)
  {
    log_error("ladish does not implement LASH server interface.");
    goto fail;
  }

  dbus_error_init(&error);
  cdbus_g_dbus_connection = dbus_bus_get_private(DBUS_BUS_SESSION, &error);
  if (cdbus_g_dbus_connection == NULL)
  {
    log_error("Cannot connect to D-Bus session bus: %s", error.message);
    dbus_error_free(&error);
    goto fail;
  }

  dbus_connection_set_exit_on_disconnect(cdbus_g_dbus_connection, FALSE);

  dbus_unique_name = dbus_bus_get_unique_name(cdbus_g_dbus_connection);
  if (dbus_unique_name == NULL)
  {
    log_error("Failed to read unique bus name");
    goto close_connection;
  }

  log_info("Connected to session bus, unique name is \"%s\"", dbus_unique_name);


  g_object = dbus_object_path_new("/", &g_interface, NULL, NULL);
  if (g_object == NULL)
  {
    goto close_connection;
  }

  if (!dbus_object_path_register(cdbus_g_dbus_connection, g_object))
  {
    goto destroy_object;
  }

  flags32 = client_flags;
  msg_ptr = cdbus_new_method_call_message(SERVICE_NAME, LASH_SERVER_OBJECT_PATH, IFACE_LASH_SERVER, "Init", "su", &class, &flags32);
  if (msg_ptr == NULL)
  {
    goto close_connection;
  }

  ret = dbus_connection_send(cdbus_g_dbus_connection, msg_ptr, NULL);
  dbus_message_unref(msg_ptr);
  if (!ret)
  {
    log_error("Cannot send message over D-Bus due to lack of memory");
    goto close_connection;
  }

  log_debug("ladish LASH support initialized (%s %s)", (client_flags & LASH_Config_File) != 0 ? "file" : "", (client_flags & LASH_Config_Data_Set) != 0 ? "dict" : "");
  g_client.flags = client_flags;

  return &g_client;

destroy_object:
  dbus_object_path_destroy(cdbus_g_dbus_connection, g_object);
close_connection:
  dbus_connection_close(cdbus_g_dbus_connection);
  dbus_connection_unref(cdbus_g_dbus_connection);
fail:
  return NULL;
}

unsigned int lash_get_pending_event_count(lash_client_t * client_ptr)
{
  ASSERT(client_ptr == &g_client);
  return !g_busy && g_event.type != 0 ? 1 : 0;
}

static void dispatch(void)
{
	do
	{
		dbus_connection_read_write_dispatch(cdbus_g_dbus_connection, 0);
	}
	while (dbus_connection_get_dispatch_status(cdbus_g_dbus_connection) == DBUS_DISPATCH_DATA_REMAINS);
}

unsigned int lash_get_pending_config_count(lash_client_t * client_ptr)
{
  struct list_head * node_ptr;
  unsigned int count;

  ASSERT(client_ptr == &g_client);
  dispatch();

  count = 0;
  list_for_each(node_ptr, &g_pending_configs)
  {
    count++;
  }

  return count;
}

lash_event_t * lash_get_event(lash_client_t * client_ptr)
{
  ASSERT(client_ptr == &g_client);
  dispatch();

  if (g_busy)
  {
    if (g_event.type == LASH_Restore_Data_Set && list_empty(&g_pending_configs))
    {
      /* the example lash_simple_client client does not send LASH_Restore_Data_Set event back */
      lash_send_event(&g_client, &g_event);
      ASSERT(!g_busy);
    }
    else
    {
      return NULL;
    }
  }

  if (g_event.type == 0)
  {
    if (g_quit)
    {
      g_event.type = LASH_Quit;
    }
    else
    {
      return NULL;
    }
  }

  if (g_event.type == LASH_Save_Data_Set)
  {
    clean_pending_configs();
    /* TODO: change status to "saving" */
  }
  else if (g_event.type == LASH_Restore_Data_Set)
  {
    load_configs(g_event.string);
    /* TODO: change status to "restoring (data)" */
  }
  else if (g_event.type == LASH_Restore_File)
  {
    /* TODO: change status to "restoring (file)" */
  }
  else if (g_event.type == LASH_Save_File)
  {
    /* TODO: change status to "saving" */
  }
  else if (g_event.type == LASH_Quit)
  {
    /* TODO: change status to "quitting" */
  }

  g_busy = true;
  log_debug("App begins to process event %d (%s)", g_event.type, g_event.string);
  return &g_event;
}

lash_config_t * lash_get_config(lash_client_t * client_ptr)
{
  struct _lash_config * config_ptr;

  ASSERT(client_ptr == &g_client);

  if (list_empty(&g_pending_configs))
  {
    return NULL;
  }

  config_ptr = list_entry(g_pending_configs.next, struct _lash_config, siblings);
  list_del(g_pending_configs.next);

  return config_ptr;
}

void lash_send_event(lash_client_t * client_ptr, lash_event_t * event_ptr)
{
  ASSERT(client_ptr == &g_client);

  log_debug("lash_send_event() called. type=%d string=%s", event_ptr->type, event_ptr->string != NULL ? event_ptr->string : "(NULL)");

  dispatch();

  if (!g_busy)
  {
    return;
  }

  if (event_ptr->type == LASH_Save_File)
  {
    log_debug("Save to file complete (%s)", g_event.string);
    g_busy = false;

    if ((g_client.flags & LASH_Config_Data_Set) != 0)
    {
      log_debug("Client wants to save a dict too");
      g_event.type = LASH_Save_Data_Set;
      if (event_ptr == &g_event)
      {
        return;
      }
    }
    else
    {
      g_event.type = 0;
      free(g_event.string);
      g_event.string = NULL; 
      /* TODO: change status to "saved" */
   }
  }
  else if (event_ptr->type == LASH_Save_Data_Set)
  {
    log_debug("Save to dict complete (%s)", g_event.string);

    save_pending_configs(g_event.string);

    g_busy = false;

    g_event.type = 0;
    free(g_event.string);
    g_event.string = NULL;
      /* TODO: change status to "saved" */
  }
  else if (event_ptr->type == LASH_Restore_Data_Set)
  {
    log_debug("Restore from dict complete (%s)", g_event.string);
    g_busy = false;

    if ((g_client.flags & LASH_Config_File) != 0)
    {
      log_debug("Client wants to restore from file too");
      g_event.type = LASH_Restore_File;
      if (event_ptr == &g_event)
      {
        return;
      }
    }
    else
    {
      g_event.type = 0;
      free(g_event.string);
      g_event.string = NULL;
      /* TODO: change status to "restored" */
    }
  }
  else if (event_ptr->type == LASH_Restore_File)
  {
    log_debug("Restore from file complete (%s)", g_event.string);

    g_busy = false;

    g_event.type = 0;
    free(g_event.string);
    g_event.string = NULL;
    /* TODO: change status to "restored" */
  }

  lash_event_destroy(event_ptr);
}

void lash_send_config(lash_client_t * client_ptr, lash_config_t * config_ptr)
{
  ASSERT(client_ptr == &g_client);

  log_debug("lash_send_config() called. key=%s value_size=%zu", config_ptr->key, config_ptr->size);

  dispatch();

  if (g_event.type == LASH_Save_Data_Set)
  {
    list_add_tail(&config_ptr->siblings, &g_pending_configs);
  }
  else
  {
    log_error("Ignoring config with key '%s' because app is not saving data set", config_ptr->key);
    lash_config_destroy(config_ptr);
  }
}

int lash_server_connected(lash_client_t * client_ptr)
{
  ASSERT(client_ptr == &g_client);
  return 1;                     /* yes */
}

const char * lash_get_server_name(lash_client_t * client_ptr)
{
  ASSERT(client_ptr == &g_client);
	return "localhost";
}

lash_event_t * lash_event_new(void)
{
  struct _lash_event * event_ptr;

  event_ptr = malloc(sizeof(struct _lash_event));
  if (event_ptr == NULL)
  {
    log_error("malloc() failed to allocate lash event struct");
    return NULL;
  }

  event_ptr->type = 0;
  event_ptr->string = NULL;

  return event_ptr;
}

lash_event_t * lash_event_new_with_type(enum LASH_Event_Type type)
{
	lash_event_t * event_ptr;

	event_ptr = lash_event_new();
  if (event_ptr == NULL)
  {
    return NULL;
  }

	lash_event_set_type(event_ptr, type);
	return event_ptr;
}

lash_event_t * lash_event_new_with_all(enum LASH_Event_Type type, const char * string)
{
	lash_event_t * event_ptr;

	event_ptr = lash_event_new_with_type(type);
  if (event_ptr == NULL)
  {
    return NULL;
  }

  if (string != NULL)
  {
    event_ptr->string = strdup(string);
    if (event_ptr->string == NULL)
    {
      log_error("strdup() failed for event string '%s'", string);
      free(event_ptr);
      return NULL;
    }
  }

	return event_ptr;
}

void lash_event_destroy(lash_event_t * event_ptr)
{
  free(event_ptr->string);
  if (event_ptr == &g_event)
  {
    event_ptr->type = 0;
    event_ptr->string = NULL;
  }
  else
  {
    free(event_ptr);
  }
}

enum LASH_Event_Type lash_event_get_type(const lash_event_t * event_ptr)
{
  return event_ptr->type;
}

const char * lash_event_get_string(const lash_event_t * event_ptr)
{
  return event_ptr->string;
}

void lash_event_set_type(lash_event_t * event_ptr, enum LASH_Event_Type type)
{
  event_ptr->type = type;
}

void lash_event_set_string(lash_event_t * event_ptr, const char * string)
{
  char * dup;

  if (string != NULL)
  {
    dup = strdup(string);
    if (dup == NULL)
    {
      log_error("strdup() failed for event string '%s'", string);
      ASSERT_NO_PASS;
      return;
    }
  }
  else
  {
    dup = NULL;
  }

  free(event_ptr->string);
  event_ptr->string = dup;
}

const char * lash_event_get_project(const lash_event_t * event_ptr)
{
  /* Server interface - not implemented */
  ASSERT_NO_PASS;               /* lash_init() fails if LASH_Server_Interface is set */
  return NULL;
}

void lash_event_set_project(lash_event_t * event_ptr, const char * project)
{
  /* Server interface - not implemented */
  ASSERT_NO_PASS;               /* lash_init() fails if LASH_Server_Interface is set */
}

void lash_event_get_client_id(const lash_event_t * event_ptr, uuid_t id)
{
  /* Server interface - not implemented */
  ASSERT_NO_PASS;               /* lash_init() fails if LASH_Server_Interface is set */
}
 
void lash_event_set_client_id(lash_event_t * event_ptr, uuid_t id)
{
  /* Server interface - not implemented */
  ASSERT_NO_PASS;               /* lash_init() fails if LASH_Server_Interface is set */
}

unsigned char lash_event_get_alsa_client_id(const lash_event_t * event_ptr)
{
  /* Server interface - not implemented */
  ASSERT_NO_PASS;               /* lash_init() fails if LASH_Server_Interface is set */
  return 0;
}

unsigned char lash_str_get_alsa_client_id(const char * str)
{
  /* Server interface - not implemented */
  ASSERT_NO_PASS;               /* lash_init() fails if LASH_Server_Interface is set */
  /* this is an undocumented function and probably internal one that sneaked to the public API */
  return 0;
}

void lash_jack_client_name(lash_client_t * client_ptr, const char * name)
{
  /* nothing to do, ladish detects jack client name through jack server */
}

void lash_str_set_alsa_client_id(char * str, unsigned char alsa_id)
{
  /* nothing to do, ladish detects alsa id through alsapid.so, jack and a2jmidid */
  /* this is an undocumented function and probably internal one that sneaked to the public API */
}

void lash_event_set_alsa_client_id(lash_event_t * event_ptr, unsigned char alsa_id)
{
  /* set event type, so we can silently ignore the event, when sent */
	lash_event_set_type(event_ptr, LASH_Alsa_Client_ID);
}

void lash_alsa_client_id(lash_client_t * client, unsigned char id)
{
  /* nothing to do, ladish detects alsa id through alsapid.so, jack and a2jmidid */
}

lash_config_t * lash_config_new(void)
{
  struct _lash_config * config_ptr;

  config_ptr = malloc(sizeof(struct _lash_config));
  if (config_ptr == NULL)
  {
    log_error("malloc() failed to allocate lash event struct");
    return NULL;
  }

  config_ptr->key = NULL;
  config_ptr->value = NULL;
  config_ptr->size = 0;

  return config_ptr;
}

lash_config_t * lash_config_dup(const lash_config_t * src_ptr)
{
	lash_config_t * dst_ptr;

	dst_ptr = lash_config_new();
  if (dst_ptr == NULL)
  {
    return NULL;
  }

  ASSERT(src_ptr->key != NULL);
  dst_ptr->key = strdup(src_ptr->key);
  if (dst_ptr->key == NULL)
  {
    log_error("strdup() failed for config key '%s'", src_ptr->key);
    free(dst_ptr);
    return NULL;
  }

  if (dst_ptr->value != NULL)
  {
    dst_ptr->value = malloc(src_ptr->size);
    if (dst_ptr->value == NULL)
    {
      log_error("strdup() failed for config value with size %zu", src_ptr->size);
      free(dst_ptr->key);
      free(dst_ptr);
      return NULL;
    }

		memcpy(dst_ptr->value, src_ptr->value, src_ptr->size);
    dst_ptr->size = src_ptr->size;
  }

	return dst_ptr;
}

lash_config_t * lash_config_new_with_key(const char * key)
{
	lash_config_t * config_ptr;

	config_ptr = lash_config_new();
  if (config_ptr == NULL)
  {
    return NULL;
  }

  config_ptr->key = strdup(key);
  if (config_ptr->key == NULL)
  {
    log_error("strdup() failed for config key '%s'", key);
    free(config_ptr);
    return NULL;
  }

	return config_ptr;
}

void lash_config_destroy(lash_config_t * config_ptr)
{
  free(config_ptr->key);
  free(config_ptr->value);
  free(config_ptr);
}

const char * lash_config_get_key(const lash_config_t * config_ptr)
{
  return config_ptr->key;
}

const void * lash_config_get_value(const lash_config_t * config_ptr)
{
  return config_ptr->value;
}

size_t lash_config_get_value_size(const lash_config_t * config_ptr)
{
  return config_ptr->size;
}

void lash_config_set_key(lash_config_t * config_ptr, const char * key)
{
  char * dup;

  ASSERT(key != NULL);

  dup = strdup(key);
  if (dup == NULL)
  {
    log_error("strdup() failed for config key '%s'", key);
    ASSERT_NO_PASS;
    return;
  }

  free(config_ptr->key);
  config_ptr->key = dup;
}

void lash_config_set_value(lash_config_t * config_ptr, const void * value, size_t value_size)
{
  void * buf;

  if (value != NULL)
  {
		buf = malloc(value_size);
    if (buf == NULL)
    {
      log_error("malloc() failed for config value with size %zu", value_size);
      ASSERT_NO_PASS;
      return;
    }

		memcpy(buf, value, value_size);
  }
  else
  {
    buf = NULL;
    value_size = 0;
  }

  free(config_ptr->value);
  config_ptr->value = buf;
  config_ptr->size = value_size;
}

uint32_t lash_config_get_value_int(const lash_config_t * config_ptr)
{
  ASSERT(lash_config_get_value_size(config_ptr) >= sizeof(uint32_t));
	return ntohl(*(const uint32_t *)lash_config_get_value(config_ptr));
}

float lash_config_get_value_float(const lash_config_t * config_ptr)
{
  ASSERT(lash_config_get_value_size(config_ptr) >= sizeof(float));
	return *(const float *)lash_config_get_value(config_ptr);
}

double lash_config_get_value_double(const lash_config_t * config_ptr)
{
  ASSERT(lash_config_get_value_size(config_ptr) >= sizeof(double));
	return *(const double *)lash_config_get_value(config_ptr);
}

const char * lash_config_get_value_string(const lash_config_t * config_ptr)
{
  const char * string;
  size_t len;
  void * ptr;

  string = lash_config_get_value(config_ptr);
  len = lash_config_get_value_size(config_ptr);
  ptr = memchr(string, 0, len);
  ASSERT(ptr != NULL);
  return string;
}

void lash_config_set_value_int(lash_config_t * config_ptr, uint32_t value)
{
	value = htonl(value);
	lash_config_set_value(config_ptr, &value, sizeof(uint32_t));
}

void lash_config_set_value_float(lash_config_t * config_ptr, float value)
{
	lash_config_set_value(config_ptr, &value, sizeof(float));
}

void lash_config_set_value_double(lash_config_t * config_ptr, double value)
{
	lash_config_set_value(config_ptr, &value, sizeof(double));
}

void lash_config_set_value_string(lash_config_t * config_ptr, const char * value)
{
	lash_config_set_value(config_ptr, value, strlen(value) + 1);
}

const char * lash_get_fqn(const char * dir, const char * file)
{
  static char * fqn = NULL;

  if (fqn != NULL)
  {
    free(fqn);
  }

  fqn = catdup3(dir, "/", file);

  return fqn;
}

/***************************************************************************/
/* D-Bus interface implementation */

static void lash_quit(struct dbus_method_call * call_ptr)
{
  log_debug("Quit command received through D-Bus");
  g_quit = true;
}

static void lash_save(struct dbus_method_call * call_ptr)
{
  const char * dir;
  char * dup;
  int type;

  dbus_error_init(&cdbus_g_dbus_error);
  if (!dbus_message_get_args(call_ptr->message, &cdbus_g_dbus_error, DBUS_TYPE_STRING, &dir, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  log_debug("Save command received through D-Bus (%s)", dir);

  if (g_event.type != 0)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_UNFINISHED_TASK, "App is busy processing event if type %d", g_event.type);
    return;
  }

  if (g_quit != 0)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_UNFINISHED_TASK, "App is quitting", g_event.type);
    return;
  }

  if ((g_client.flags & LASH_Config_File) != 0)
  {
    type = LASH_Save_File;
  }
  else if ((g_client.flags & LASH_Config_Data_Set) != 0)
  {
    type = LASH_Save_Data_Set;
  }
  else
  {
    log_debug("App does not have internal state");
    /* TODO: change status to "saved" */
    return;
  }

  dup = strdup(dir);
  if (dup == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup() failed for event string (dir) '%s'", dup);
    return;
  }

  ASSERT(g_event.string == NULL);
  g_event.string = dup;
  g_event.type = type;
}

static void lash_restore(struct dbus_method_call * call_ptr)
{
  const char * dir;
  char * dup;
  int type;

  dbus_error_init(&cdbus_g_dbus_error);
  if (!dbus_message_get_args(call_ptr->message, &cdbus_g_dbus_error, DBUS_TYPE_STRING, &dir, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, cdbus_g_dbus_error.message);
    dbus_error_free(&cdbus_g_dbus_error);
    return;
  }

  log_debug("Restore command received through D-Bus (%s)", dir);

  if (g_event.type != 0)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_UNFINISHED_TASK, "App is busy processing event if type %d", g_event.type);
    return;
  }

  if (g_quit != 0)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_UNFINISHED_TASK, "App is quitting", g_event.type);
    return;
  }

  if ((g_client.flags & LASH_Config_File) != 0)
  {
    type = LASH_Restore_File;
  }
  else if ((g_client.flags & LASH_Config_Data_Set) != 0)
  {
    type = LASH_Restore_Data_Set;
  }
  else
  {
    log_debug("App does not have internal state");
    /* TODO: change status to "restored" */
    return;
  }

  dup = strdup(dir);
  if (dup == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup() failed for event string (dir) '%s'", dup);
    return;
  }

  ASSERT(g_event.string == NULL);
  g_event.string = dup;
  g_event.type = type;
}

METHOD_ARGS_BEGIN(Save, "Tell lash client to save")
  METHOD_ARG_DESCRIBE_IN("app_dir", "s", "Directory where app must save its internal state")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Restore, "Tell lash client to restore")
  METHOD_ARG_DESCRIBE_IN("app_dir", "s", "Directory from where app must load its internal state")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Quit, "Tell lash client to quit")
METHOD_ARGS_END

METHODS_BEGIN
  METHOD_DESCRIBE(Save, lash_save)
  METHOD_DESCRIBE(Restore, lash_restore)
  METHOD_DESCRIBE(Quit, lash_quit)
METHODS_END

INTERFACE_BEGIN(g_interface, IFACE_LASH_CLIENT)
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
INTERFACE_END
