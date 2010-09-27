/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of code that interfaces ladiconfd through D-Bus
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

/* This implementation subscribes for changes of all values.
 * It is suboptimal in general because of the increased overhead.
 * A smarter implementation will subscribe using filter based on arg0 (key).
 * Such smarter implementation seems to not be worth becase of
 * the limited key set that ladish uses.
 */

#include "conf_proxy.h"

struct pair
{
  struct list_head siblings;

  uint64_t version;

  char * key;
  char * value;
  size_t value_buffer_size;

  void (* callback)(void * context, const char * key, const char * value);
  void * callback_context;
};

static struct list_head g_pairs;

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

static void on_value_changed(struct pair * pair_ptr, const char * value, uint64_t version, bool announce)
{
  size_t len;

  log_info("'%s' <- '%s' (%"PRIu64")", pair_ptr->key, value, version);

  pair_ptr->version = version;

  len = strlen(value);
  if (len < pair_ptr->value_buffer_size)
  {
    memcpy(pair_ptr->value, value, len + 1);
  }
  else
  {
    free(pair_ptr->value);
    pair_ptr->value = strdup(value);
    if (pair_ptr->value == NULL)
    {
      log_error("strdup(\"%s\") failed for key \"%s\" value", value, pair_ptr->key);
    }
  }

  if (announce && pair_ptr->callback != NULL)
  {
    pair_ptr->callback(pair_ptr->callback_context, pair_ptr->key, value);
  }
}

static void on_life_status_changed(bool appeared)
{
  struct list_head * node_ptr;
  struct pair * pair_ptr;

  if (appeared)
  {
      log_info("confd activatation detected.");
  }
  else
  {
    log_info("confd deactivatation detected.");

    list_for_each(node_ptr, &g_pairs)
    {
      pair_ptr = list_entry(node_ptr, struct pair, siblings);
      pair_ptr->version = 0;
    }
  }
}

static void on_conf_changed(void * context, DBusMessage * message_ptr)
{
  const char * key;
  const char * value;
  dbus_uint64_t version;
  struct pair * pair_ptr;

  if (!dbus_message_get_args(
        message_ptr,
        &g_dbus_error,
        DBUS_TYPE_STRING, &key,
        DBUS_TYPE_STRING, &value,
        DBUS_TYPE_UINT64, &version,
        DBUS_TYPE_INVALID))
  {
    log_error("Invalid parameters of \"changed\" signal: %s",  g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  pair_ptr = find_pair(key);
  if (pair_ptr == NULL)
  {
    /* we dont care about this key */
    return;
  }

  if (pair_ptr->version >= version)
  {
    /* signal for either already known version of the key or a older one */
    return;
  }

  if (pair_ptr->value != NULL && strcmp(value, pair_ptr->value) == 0)
  {
    /* the conf service should not send the signal when value is not changed,
       but in case that it does, ignore it. This can happen when confd is restarted */
    return;
  }

  on_value_changed(pair_ptr, value, version, true);
}

/* this must be static because it is referenced by the
 * dbus helper layer when hooks are active */
static struct dbus_signal_hook g_signal_hooks[] =
{
  {"changed", on_conf_changed},
  {NULL, NULL}
};

bool conf_proxy_init(void)
{
  INIT_LIST_HEAD(&g_pairs);

  if (!dbus_register_service_lifetime_hook(g_dbus_connection, CONF_SERVICE_NAME, on_life_status_changed))
  {
    log_error("dbus_register_service_lifetime_hook() failed for confd service");
    return false;
  }

  if (!dbus_register_object_signal_hooks(
        g_dbus_connection,
        CONF_SERVICE_NAME,
        CONF_OBJECT_PATH,
        CONF_IFACE,
        NULL,
        g_signal_hooks))
  {
    dbus_unregister_service_lifetime_hook(g_dbus_connection, CONF_SERVICE_NAME);
    log_error("dbus_register_object_signal_hooks() failed for conf interface");
    return false;
  }

  return true;
}

void conf_proxy_uninit(void)
{
  dbus_unregister_object_signal_hooks(g_dbus_connection, CONF_SERVICE_NAME, CONF_OBJECT_PATH, CONF_IFACE);
  dbus_unregister_service_lifetime_hook(g_dbus_connection, CONF_SERVICE_NAME);
}

bool
conf_register(
  const char * key,
  void (* callback)(void * context, const char * key, const char * value),
  void * callback_context)
{
  const char * value;
  uint64_t version;
  struct pair * pair_ptr;

  pair_ptr = find_pair(key);
  if (pair_ptr != NULL)
  {
    log_error("key '%s' already registered", key);
    ASSERT_NO_PASS;
    return false;
  }

  if (!dbus_call(CONF_SERVICE_NAME, CONF_OBJECT_PATH, CONF_IFACE, "get", "s", &key, "st", &value, &version))
  {
    //log_error("conf::get() failed.");
    version = 0;
    value = NULL;
  }

  pair_ptr = malloc(sizeof(struct pair));
  if (pair_ptr == NULL)
  {
    log_error("malloc() failed to allocate memory for pair struct");
    return false;
  }

  pair_ptr->key = strdup(key);
  if (pair_ptr->key == NULL)
  {
    log_error("strdup(\"%s\") failed for key", key);
    free(pair_ptr);
    return false;
  }

  if (value != NULL)
  {
    pair_ptr->value_buffer_size = strlen(value) + 1;

    pair_ptr->value = malloc(pair_ptr->value_buffer_size);
    if (pair_ptr->value == NULL)
    {
      log_error("malloc(%zu) failed for value \"%s\"", pair_ptr->value_buffer_size, value);
      free(pair_ptr->key);
      free(pair_ptr);
      return false;
    }
    memcpy(pair_ptr->value, value, pair_ptr->value_buffer_size);
  }
  else
  {
    pair_ptr->value = NULL;
    pair_ptr->value_buffer_size = 0;
  }

  pair_ptr->version = version;
  pair_ptr->callback = callback;
  pair_ptr->callback_context = callback_context;

  list_add_tail(&pair_ptr->siblings, &g_pairs);

  if (callback != NULL)
  {
    callback(callback_context, key, value);
  }

  return true;
}

bool conf_set(const char * key, const char * value)
{
  uint64_t version;
  struct pair * pair_ptr;

  pair_ptr = find_pair(key);
  if (pair_ptr != NULL && pair_ptr->value != NULL && strcmp(value, pair_ptr->value) == 0)
  {
    return true;                /* not changed */
  }

  if (!dbus_call(CONF_SERVICE_NAME, CONF_OBJECT_PATH, CONF_IFACE, "set", "ss", &key, &value, "t", &version))
  {
    log_error("conf::set() failed.");
    return false;
  }

  if (pair_ptr != NULL && pair_ptr->value != NULL && strcmp(value, pair_ptr->value) != 0)
  {
    /* record the new version and dont call the callback */
    on_value_changed(pair_ptr, value, version, false);
  }

  return true;
}

bool conf_get(const char * key, const char ** value_ptr)
{
  struct pair * pair_ptr;

  pair_ptr = find_pair(key);
  if (pair_ptr == NULL)
  {
    ASSERT_NO_PASS;             /* call conf_register() first */
    return false;
  }

  if (pair_ptr->value == NULL)
  {
    return false;               /* malloc() failed */
  }

  *value_ptr = pair_ptr->value;
  return true;
}

bool conf_string2bool(const char * value)
{
  if (value[0] == 0 ||
      strcasecmp(value, "false") == 0 ||
      strcmp(value, "0") == 0)
  {
    return false;
  }

  return true;
}

const char * conf_bool2string(bool value)
{
  return value ? "true" : "false";
}

bool conf_set_bool(const char * key, bool value)
{
  return conf_set(key, conf_bool2string(value));
}

bool conf_get_bool(const char * key, bool * value_ptr)
{
  const char * str;

  if (!conf_get(key, &str))
  {
    return false;
  }

  *value_ptr = conf_string2bool(str);

  return true;
}
