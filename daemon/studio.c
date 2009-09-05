/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the studio singleton object
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <expat.h>

#include "../jack_proxy.h"
#include "../graph_proxy.h"
#include "graph_iface.h"
#include "../dbus_constants.h"
#include "control.h"
#include "../catdup.h"
#include "../dbus/error.h"
#include "dirhelpers.h"

#define STUDIOS_DIR "/studios/"
char * g_studios_dir;

#define STUDIO_HEADER_TEXT BASE_NAME " Studio configuration.\n"

extern const struct dbus_interface_descriptor g_interface_studio;

struct studio
{
  struct graph_implementator graph_impl;

  struct list_head all_connections;        /* All connections (studio guts and all rooms). Including superconnections. */
  struct list_head all_ports;              /* All ports (studio guts and all rooms) */
  struct list_head all_clients;            /* All clients (studio guts and all rooms) */
  struct list_head jack_connections;       /* JACK connections (studio guts and all rooms). Including superconnections, excluding virtual connections. */
  struct list_head jack_ports;             /* JACK ports (studio guts and all rooms). Excluding virtual ports. */
  struct list_head jack_clients;           /* JACK clients (studio guts and all rooms). Excluding virtual clients. */
  struct list_head rooms;                  /* Rooms connected to the studio */
  struct list_head clients;                /* studio clients (studio guts and room links) */
  struct list_head ports;                  /* studio ports (studio guts and room links) */

  bool automatic:1;             /* Studio was automatically created because of external JACK start */
  bool persisted:1;             /* Studio has on-disk representation, i.e. can be reloaded from disk */
  bool modified:1;              /* Studio needs saving */
  bool jack_conf_valid:1;       /* JACK server configuration obtained successfully */
  bool jack_running:1;          /* JACK server is running */

  struct list_head jack_conf;   /* root of the conf tree */
  struct list_head jack_params; /* list of conf tree leaves */

  dbus_object_path dbus_object;

  struct list_head event_queue;

  char * name;
  char * filename;

  graph_proxy_handle jack_graph_proxy;
} g_studio;

#define EVENT_JACK_START   0
#define EVENT_JACK_STOP    1

struct event
{
  struct list_head siblings;
  unsigned int type;
};

#define JACK_CONF_MAX_ADDRESS_SIZE 1024

struct jack_conf_parameter
{
  struct list_head siblings;    /* siblings in container children list */
  struct list_head leaves;      /* studio::jack_param siblings */
  char * name;
  struct jack_conf_container * parent_ptr;
  char address[JACK_CONF_MAX_ADDRESS_SIZE];
  struct jack_parameter_variant parameter;
};

struct jack_conf_container
{
  struct list_head siblings;
  char * name;
  struct jack_conf_container * parent_ptr;
  bool children_leafs;          /* if true, children are "jack_conf_parameter"s, if false, children are "jack_conf_container"s */
  struct list_head children;
};

struct conf_callback_context
{
  char address[JACK_CONF_MAX_ADDRESS_SIZE];
  struct list_head * container_ptr;
  struct jack_conf_container * parent_ptr;
};

#define PARSE_CONTEXT_ROOT        0
#define PARSE_CONTEXT_STUDIO      1
#define PARSE_CONTEXT_JACK        2
#define PARSE_CONTEXT_CONF        3
#define PARSE_CONTEXT_PARAMETER   4

#define MAX_STACK_DEPTH       10
#define MAX_DATA_SIZE         1024

struct parse_context
{
  void * call_ptr;
  XML_Bool error;
  unsigned int element[MAX_STACK_DEPTH];
  signed int depth;
  char data[MAX_DATA_SIZE];
  int data_used;
  char * path;
};

bool
jack_conf_container_create(
  struct jack_conf_container ** container_ptr_ptr,
  const char * name)
{
  struct jack_conf_container * container_ptr;

  container_ptr = malloc(sizeof(struct jack_conf_container));
  if (container_ptr == NULL)
  {
    lash_error("malloc() failed to allocate struct jack_conf_container");
    goto fail;
  }

  container_ptr->name = strdup(name);
  if (container_ptr->name == NULL)
  {
    lash_error("strdup() failed to duplicate \"%s\"", name);
    goto fail_free;
  }

  INIT_LIST_HEAD(&container_ptr->children);
  container_ptr->children_leafs = false;

  *container_ptr_ptr = container_ptr;
  return true;

fail_free:
  free(container_ptr);

fail:
  return false;
}

bool
jack_conf_parameter_create(
  struct jack_conf_parameter ** parameter_ptr_ptr,
  const char * name)
{
  struct jack_conf_parameter * parameter_ptr;

  parameter_ptr = malloc(sizeof(struct jack_conf_parameter));
  if (parameter_ptr == NULL)
  {
    lash_error("malloc() failed to allocate struct jack_conf_parameter");
    goto fail;
  }

  parameter_ptr->name = strdup(name);
  if (parameter_ptr->name == NULL)
  {
    lash_error("strdup() failed to duplicate \"%s\"", name);
    goto fail_free;
  }

  *parameter_ptr_ptr = parameter_ptr;
  return true;

fail_free:
  free(parameter_ptr);

fail:
  return false;
}

void
jack_conf_parameter_destroy(
  struct jack_conf_parameter * parameter_ptr)
{
#if 0
  lash_info("jack_conf_parameter destroy");

  switch (parameter_ptr->parameter.type)
  {
  case jack_boolean:
    lash_info("%s value is %s (boolean)", parameter_ptr->name, parameter_ptr->parameter.value.boolean ? "true" : "false");
    break;
  case jack_string:
    lash_info("%s value is %s (string)", parameter_ptr->name, parameter_ptr->parameter.value.string);
    break;
  case jack_byte:
    lash_info("%s value is %u/%c (byte/char)", parameter_ptr->name, parameter_ptr->parameter.value.byte, (char)parameter_ptr->parameter.value.byte);
    break;
  case jack_uint32:
    lash_info("%s value is %u (uint32)", parameter_ptr->name, (unsigned int)parameter_ptr->parameter.value.uint32);
    break;
  case jack_int32:
    lash_info("%s value is %u (int32)", parameter_ptr->name, (signed int)parameter_ptr->parameter.value.int32);
    break;
  default:
    lash_error("unknown jack parameter_ptr->parameter type %d (%s)", (int)parameter_ptr->parameter.type, parameter_ptr->name);
    break;
  }
#endif

  if (parameter_ptr->parameter.type == jack_string)
  {
    free(parameter_ptr->parameter.value.string);
  }

  free(parameter_ptr->name);
  free(parameter_ptr);
}

void
jack_conf_container_destroy(
  struct jack_conf_container * container_ptr)
{
  struct list_head * node_ptr;

  //lash_info("\"%s\" jack_conf_parameter destroy", container_ptr->name);

  if (!container_ptr->children_leafs)
  {
    while (!list_empty(&container_ptr->children))
    {
      node_ptr = container_ptr->children.next;
      list_del(node_ptr);
      jack_conf_container_destroy(list_entry(node_ptr, struct jack_conf_container, siblings));
    }
  }
  else
  {
    while (!list_empty(&container_ptr->children))
    {
      node_ptr = container_ptr->children.next;
      list_del(node_ptr);
      jack_conf_parameter_destroy(list_entry(node_ptr, struct jack_conf_parameter, siblings));
    }
  }

  free(container_ptr->name);
  free(container_ptr);
}

#define context_ptr ((struct conf_callback_context *)context)

static
bool
conf_callback(
  void * context,
  bool leaf,
  const char * address,
  char * child)
{
  char path[JACK_CONF_MAX_ADDRESS_SIZE];
  const char * component;
  char * dst;
  size_t len;
  bool is_set;
  struct jack_conf_container * parent_ptr;
  struct jack_conf_container * container_ptr;
  struct jack_conf_parameter * parameter_ptr;

  parent_ptr = context_ptr->parent_ptr;

  if (parent_ptr == NULL && strcmp(child, "drivers") == 0)
  {
    lash_debug("ignoring drivers branch");
    return true;
  }

  dst = path;
  component = address;
  while (*component != 0)
  {
    len = strlen(component);
    memcpy(dst, component, len);
    dst[len] = ':';
    component += len + 1;
    dst += len + 1;
  }

  strcpy(dst, child);

  /* address always is same buffer as the one supplied through context pointer */
  assert(context_ptr->address == address);
  dst = (char *)component;

  len = strlen(child) + 1;
  memcpy(dst, child, len);
  dst[len] = 0;

  if (leaf)
  {
    lash_debug("%s (leaf)", path);

    if (parent_ptr == NULL)
    {
      lash_error("jack conf parameters can't appear in root container");
      return false;
    }

    if (!parent_ptr->children_leafs)
    {
      if (!list_empty(&parent_ptr->children))
      {
        lash_error("jack conf parameters cant be mixed with containers at same hierarchy level");
        return false;
      }

      parent_ptr->children_leafs = true;
    }

    if (!jack_conf_parameter_create(&parameter_ptr, child))
    {
      lash_error("jack_conf_parameter_create() failed");
      return false;
    }

    if (!jack_proxy_get_parameter_value(context_ptr->address, &is_set, &parameter_ptr->parameter))
    {
      lash_error("cannot get value of %s", path);
      return false;
    }

    if (is_set)
    {
#if 0
      switch (parameter_ptr->parameter.type)
      {
      case jack_boolean:
        lash_info("%s value is %s (boolean)", path, parameter_ptr->parameter.value.boolean ? "true" : "false");
        break;
      case jack_string:
        lash_info("%s value is %s (string)", path, parameter_ptr->parameter.value.string);
        break;
      case jack_byte:
        lash_info("%s value is %u/%c (byte/char)", path, parameter_ptr->parameter.value.byte, (char)parameter_ptr->parameter.value.byte);
        break;
      case jack_uint32:
        lash_info("%s value is %u (uint32)", path, (unsigned int)parameter_ptr->parameter.value.uint32);
        break;
      case jack_int32:
        lash_info("%s value is %u (int32)", path, (signed int)parameter_ptr->parameter.value.int32);
        break;
      default:
        lash_error("unknown jack parameter_ptr->parameter type %d (%s)", (int)parameter_ptr->parameter.type, path);
        jack_conf_parameter_destroy(parameter_ptr);
        return false;
      }
#endif

      parameter_ptr->parent_ptr = parent_ptr;
      memcpy(parameter_ptr->address, context_ptr->address, JACK_CONF_MAX_ADDRESS_SIZE);
      list_add_tail(&parameter_ptr->siblings, &parent_ptr->children);
      list_add_tail(&parameter_ptr->leaves, &g_studio.jack_params);
    }
    else
    {
      jack_conf_parameter_destroy(parameter_ptr);
    }
  }
  else
  {
    lash_debug("%s (container)", path);

    if (parent_ptr != NULL && parent_ptr->children_leafs)
    {
      lash_error("jack conf containers cant be mixed with parameters at same hierarchy level");
      return false;
    }

    if (!jack_conf_container_create(&container_ptr, child))
    {
      lash_error("jack_conf_container_create() failed");
      return false;
    }

    container_ptr->parent_ptr = parent_ptr;

    if (parent_ptr == NULL)
    {
      list_add_tail(&container_ptr->siblings, &g_studio.jack_conf);
    }
    else
    {
      list_add_tail(&container_ptr->siblings, &parent_ptr->children);
    }

    context_ptr->parent_ptr = container_ptr;

    if (!jack_proxy_read_conf_container(context_ptr->address, context, conf_callback))
    {
      lash_error("cannot read container %s", path);
      return false;
    }

    context_ptr->parent_ptr = parent_ptr;
  }

  *dst = 0;

  return true;
}

#undef context_ptr

static void jack_conf_clear(void)
{
  struct list_head * node_ptr;

  INIT_LIST_HEAD(&g_studio.jack_params); /* we will destroy the leaves as part of tree destroy traversal */
  while (!list_empty(&g_studio.jack_conf))
  {
    node_ptr = g_studio.jack_conf.next;
    list_del(node_ptr);
    jack_conf_container_destroy(list_entry(node_ptr, struct jack_conf_container, siblings));
  }

  g_studio.jack_conf_valid = false;
}

bool studio_fetch_jack_settings()
{
  struct conf_callback_context context;

  jack_conf_clear();

  context.address[0] = 0;
  context.container_ptr = &g_studio.jack_conf;
  context.parent_ptr = NULL;

  if (!jack_proxy_read_conf_container(context.address, &context, conf_callback))
  {
    lash_error("jack_proxy_read_conf_container() failed.");
    return false;
  }

  return true;
}

bool studio_name_generate(char ** name_ptr)
{
  time_t now;
  char timestamp_str[26];
  char * name;

  time(&now);
  //ctime_r(&now, timestamp_str);
  //timestamp_str[24] = 0;
  snprintf(timestamp_str, sizeof(timestamp_str), "%llu", (unsigned long long)now);

  name = catdup("Studio ", timestamp_str);
  if (name == NULL)
  {
    lash_error("catdup failed to create studio name");
    return false;
  }

  *name_ptr = name;
  return true;
}

bool
studio_publish(void)
{
  dbus_object_path object;

  assert(g_studio.name != NULL);

  object = dbus_object_path_new(STUDIO_OBJECT_PATH, &g_interface_studio, &g_studio, &g_interface_patchbay, &g_studio.graph_impl, NULL);
  if (object == NULL)
  {
    lash_error("dbus_object_path_new() failed");
    return false;
  }

  if (!dbus_object_path_register(g_dbus_connection, object))
  {
    lash_error("object_path_register() failed");
    dbus_object_path_destroy(g_dbus_connection, object);
    return false;
  }

  lash_info("Studio D-Bus object created. \"%s\"", g_studio.name);

  g_studio.dbus_object = object;

  emit_studio_appeared();

  return true;
}

static void emit_studio_started()
{
  signal_new_valist(g_dbus_connection, STUDIO_OBJECT_PATH, IFACE_STUDIO, "StudioStarted", DBUS_TYPE_INVALID);
}

static void emit_studio_stopped()
{
  signal_new_valist(g_dbus_connection, STUDIO_OBJECT_PATH, IFACE_STUDIO, "StudioStopped", DBUS_TYPE_INVALID);
}

static bool studio_start(void)
{
  if (!g_studio.jack_running)
  {
    lash_info("Starting JACK server.");

    if (!jack_proxy_start_server())
    {
      lash_error("jack_proxy_start_server() failed.");
      return false;
    }
  }

  emit_studio_started();

  return true;
}

static bool studio_stop(void)
{
  if (g_studio.jack_running)
  {
    lash_info("Stopping JACK server...");

    g_studio.automatic = false;   /* even if it was automatic, it is not anymore because user knows about it */

    if (jack_proxy_stop_server())
    {
      g_studio.jack_running = false;
      emit_studio_stopped();
    }
    else
    {
      lash_error("Stopping JACK server failed.");
      return false;
    }
  }

  return true;
}

void
studio_clear(void)
{
  g_studio.modified = false;
  g_studio.persisted = false;
  g_studio.automatic = false;

  jack_conf_clear();

  studio_stop();

  if (g_studio.dbus_object != NULL)
  {
    dbus_object_path_destroy(g_dbus_connection, g_studio.dbus_object);
    g_studio.dbus_object = NULL;
    emit_studio_disappeared();
  }

  if (g_studio.name != NULL)
  {
    free(g_studio.name);
    g_studio.name = NULL;
  }

  if (g_studio.filename != NULL)
  {
    free(g_studio.filename);
    g_studio.filename = NULL;
  }
}

void
studio_clear_if_automatic(void)
{
  if (g_studio.automatic)
  {
    lash_info("Unloading automatic studio.");
    studio_clear();
    return;
  }
}

void on_event_jack_started(void)
{
  if (g_studio.dbus_object == NULL)
  {
    assert(g_studio.name == NULL);
    if (!studio_name_generate(&g_studio.name))
    {
      lash_error("studio_name_generate() failed.");
      return;
    }

    g_studio.automatic = true;

    studio_publish();
  }

  if (!studio_fetch_jack_settings(g_studio))
  {
    lash_error("studio_fetch_jack_settings() failed.");

    studio_clear_if_automatic();
    return;
  }

  lash_info("jack conf successfully retrieved");
  g_studio.jack_conf_valid = true;
  g_studio.jack_running = true;

  if (!graph_proxy_create(JACKDBUS_SERVICE_NAME, JACKDBUS_OBJECT_PATH, &g_studio.jack_graph_proxy))
  {
    lash_error("graph_create() failed for jackdbus");
  }
}

void on_event_jack_stopped(void)
{
  studio_clear_if_automatic();

  g_studio.jack_running = false;

  graph_proxy_destroy(g_studio.jack_graph_proxy);

  /* TODO: if user wants, restart jack server and reconnect all jack apps to it */
}

void studio_run(void)
{
  struct event * event_ptr;

  while (!list_empty(&g_studio.event_queue))
  {
    event_ptr = list_entry(g_studio.event_queue.next, struct event, siblings);
    list_del(g_studio.event_queue.next);

    switch (event_ptr->type)
    {
    case EVENT_JACK_START:
      on_event_jack_started();
      break;
    case EVENT_JACK_STOP:
      on_event_jack_stopped();
      break;
    }

    free(event_ptr);
  }
}

static void on_jack_server_started(void)
{
  struct event * event_ptr;

  lash_info("JACK server start detected.");

  event_ptr = malloc(sizeof(struct event));
  if (event_ptr == NULL)
  {
    lash_error("malloc() failed to allocate struct event. Ignoring JACK start.");
    return;
  }

  event_ptr->type = EVENT_JACK_START;
  list_add_tail(&event_ptr->siblings, &g_studio.event_queue);
}

static void on_jack_server_stopped(void)
{
  struct event * event_ptr;

  lash_info("JACK server stop detected.");

  event_ptr = malloc(sizeof(struct event));
  if (event_ptr == NULL)
  {
    lash_error("malloc() failed to allocate struct event. Ignoring JACK stop.");
    return;
  }

  event_ptr->type = EVENT_JACK_STOP;
  list_add_tail(&event_ptr->siblings, &g_studio.event_queue);
}

static void on_jack_server_appeared(void)
{
  lash_info("JACK controller appeared.");
}

static void on_jack_server_disappeared(void)
{
  lash_info("JACK controller disappeared.");
}

#define studio_ptr ((struct studio *)this)

uint64_t
studio_get_graph_version(
  void * this)
{
  //lash_info("studio_get_graph_version() called");
  return 1;
}

#undef studio_ptr

bool studio_init(void)
{
  lash_info("studio object construct");

  g_studios_dir = catdup(g_base_dir, STUDIOS_DIR);
  if (g_studios_dir == NULL)
  {
    lash_error("catdup failed for '%s' and '%s'", g_base_dir, STUDIOS_DIR);
    return false;
  }

  if (!ensure_dir_exist(g_studios_dir, 0700))
  {
    free(g_studios_dir);
    return false;
  }

  g_studio.graph_impl.this = &g_studio;
  g_studio.graph_impl.get_graph_version = studio_get_graph_version;

  INIT_LIST_HEAD(&g_studio.all_connections);
  INIT_LIST_HEAD(&g_studio.all_ports);
  INIT_LIST_HEAD(&g_studio.all_clients);
  INIT_LIST_HEAD(&g_studio.jack_connections);
  INIT_LIST_HEAD(&g_studio.jack_ports);
  INIT_LIST_HEAD(&g_studio.jack_clients);
  INIT_LIST_HEAD(&g_studio.rooms);
  INIT_LIST_HEAD(&g_studio.clients);
  INIT_LIST_HEAD(&g_studio.ports);

  INIT_LIST_HEAD(&g_studio.jack_conf);
  INIT_LIST_HEAD(&g_studio.jack_params);

  INIT_LIST_HEAD(&g_studio.event_queue);

  g_studio.dbus_object = NULL;
  g_studio.name = NULL;
  g_studio.filename = NULL;
  g_studio.jack_running = false;
  studio_clear();

  if (!jack_proxy_init(
        on_jack_server_started,
        on_jack_server_stopped,
        on_jack_server_appeared,
        on_jack_server_disappeared))
  {
    lash_error("jack_proxy_init() failed.");
    studio_clear();
    return false;
  }

  return true;
}

void studio_uninit(void)
{
  jack_proxy_uninit();

  studio_clear();

  free(g_studios_dir);

  lash_info("studio object destroy");
}

bool studio_is_loaded(void)
{
  return g_studio.dbus_object != NULL;
}

void escape(const char ** src_ptr, char ** dst_ptr)
{
  const char * src;
  char * dst;
  static char hex_digits[] = "0123456789ABCDEF";

  src = *src_ptr;
  dst = *dst_ptr;

  while (*src != 0)
  {
    switch (*src)
    {
    case '/':               /* used as separator for address components */
    case '<':               /* invalid attribute value char (XML spec) */
    case '&':               /* invalid attribute value char (XML spec) */
    case '"':               /* we store attribute values in double quotes - invalid attribute value char (XML spec) */
    case '%':
      dst[0] = '%';
      dst[1] = hex_digits[*src >> 4];
      dst[2] = hex_digits[*src & 0x0F];
      dst += 3;
      src++;
      break;
    default:
      *dst++ = *src++;
    }
  }

  *src_ptr = src;
  *dst_ptr = dst;
}

#define HEX_TO_INT(hexchar) ((hexchar) <= '9' ? hexchar - '0' : 10 + (hexchar - 'A'))

static size_t unescape(const char * src, size_t src_len, char * dst)
{
  size_t dst_len;

  dst_len = 0;

  while (src_len)
  {
    if (src_len >= 3 &&
        src[0] == '%' &&
        ((src[1] >= '0' && src[1] <= '9') ||
         (src[1] >= 'A' && src[1] <= 'F')) &&
        ((src[2] >= '0' && src[2] <= '9') ||
         (src[2] >= 'A' && src[2] <= 'F')))
    {
      *dst = (HEX_TO_INT(src[1]) << 4) | HEX_TO_INT(src[2]);
      //lash_info("unescaping %c%c%c to '%c'", src[0], src[1], src[2], *dst);
      src_len -= 3;
      src += 3;
    }
    else
    {
      *dst = *src;
      src_len--;
      src++;
    }
    dst++;
    dst_len++;
  }

  return dst_len;
}

static bool compose_filename(const char * name, char ** filename_ptr_ptr, char ** backup_filename_ptr_ptr)
{
  size_t len_dir;
  char * p;
  const char * src;
  char * filename_ptr;
  char * backup_filename_ptr;

  len_dir = strlen(g_studios_dir);

  filename_ptr = malloc(len_dir + 1 + strlen(name) * 3 + 4 + 1);
  if (filename_ptr == NULL)
  {
    lash_error("malloc failed to allocate memory for studio file path");
    return false;
  }

  if (backup_filename_ptr_ptr != NULL)
  {
    backup_filename_ptr = malloc(len_dir + 1 + strlen(name) * 3 + 4 + 4 + 1);
    if (backup_filename_ptr == NULL)
    {
      lash_error("malloc failed to allocate memory for studio backup file path");
      free(filename_ptr);
      return false;
    }
  }

  p = filename_ptr;
  memcpy(p, g_studios_dir, len_dir);
  p += len_dir;

  *p++ = '/';

  src = name;
  escape(&src, &p);
  strcpy(p, ".xml");

  *filename_ptr_ptr = filename_ptr;

  if (backup_filename_ptr_ptr != NULL)
  {
    strcpy(backup_filename_ptr, filename_ptr);
    strcat(backup_filename_ptr, ".bak");
    *backup_filename_ptr_ptr = backup_filename_ptr;
  }

  return true;
}

bool
write_string(int fd, const char * string, void * call_ptr)
{
  size_t len;

  len = strlen(string);

  if (write(fd, string, len) != len)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "write() failed to write config file.");
    return false;
  }

  return true;
}

bool
write_jack_parameter(
  int fd,
  const char * indent,
  struct jack_conf_parameter * parameter_ptr,
  void * call_ptr)
{
  const char * src;
  char * dst;
  char path[JACK_CONF_MAX_ADDRESS_SIZE * 3]; /* encode each char in three bytes (percent encoding) */
  const char * content;
  char valbuf[100];

  /* compose the parameter path, percent-encode "bad" chars */
  src = parameter_ptr->address;
  dst = path;
  do
  {
    *dst++ = '/';
    escape(&src, &dst);
    src++;
  }
  while (*src != 0);
  *dst = 0;

  if (!write_string(fd, indent, call_ptr))
  {
    return false;
  }

  if (!write_string(fd, "<parameter path=\"", call_ptr))
  {
    return false;
  }

  if (!write_string(fd, path, call_ptr))
  {
    return false;
  }

  if (!write_string(fd, "\">", call_ptr))
  {
    return false;
  }

  switch (parameter_ptr->parameter.type)
  {
  case jack_boolean:
    content = parameter_ptr->parameter.value.boolean ? "true" : "false";
    lash_debug("%s value is %s (boolean)", path, content);
    break;
  case jack_string:
    content = parameter_ptr->parameter.value.string;
    lash_debug("%s value is %s (string)", path, content);
    break;
  case jack_byte:
    valbuf[0] = (char)parameter_ptr->parameter.value.byte;
    valbuf[1] = 0;
    content = valbuf;
    lash_debug("%s value is %u/%c (byte/char)", path, parameter_ptr->parameter.value.byte, (char)parameter_ptr->parameter.value.byte);
    break;
  case jack_uint32:
    snprintf(valbuf, sizeof(valbuf), "%" PRIu32, parameter_ptr->parameter.value.uint32);
    content = valbuf;
    lash_debug("%s value is %s (uint32)", path, content);
    break;
  case jack_int32:
    snprintf(valbuf, sizeof(valbuf), "%" PRIi32, parameter_ptr->parameter.value.int32);
    content = valbuf;
    lash_debug("%s value is %s (int32)", path, content);
    break;
  default:
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "unknown jack parameter_ptr->parameter type %d (%s)", (int)parameter_ptr->parameter.type, path);
    return false;
  }

  if (!write_string(fd, content, call_ptr))
  {
    return false;
  }

  if (!write_string(fd, "</parameter>\n", call_ptr))
  {
    return false;
  }

  return true;
}

bool studio_save(void * call_ptr)
{
  struct list_head * node_ptr;
  struct jack_conf_parameter * parameter_ptr;
  int fd;
  time_t timestamp;
  char timestamp_str[26];
  bool ret;
  char * filename;              /* filename */
  char * bak_filename;          /* filename of the backup file */
  char * old_filename;          /* filename where studio was persisted before save */
  struct stat st;

  time(&timestamp);
  ctime_r(&timestamp, timestamp_str);
  timestamp_str[24] = 0;

  ret = false;

  if (!compose_filename(g_studio.name, &filename, &bak_filename))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "failed to compose studio filename");
    goto exit;
  }

  if (g_studio.filename == NULL)
  {
    /* saving studio for first time */
    g_studio.filename = filename;
    free(bak_filename);
    bak_filename = NULL;
    old_filename = NULL;
  }
  else if (strcmp(g_studio.filename, filename) == 0)
  {
    /* saving already persisted studio that was not renamed */
    old_filename = filename;
  }
  else
  {
    /* saving renamed studio */
    old_filename = g_studio.filename;
    g_studio.filename = filename;
  }

  filename = NULL;
  assert(g_studio.filename != NULL);
  assert(g_studio.filename != old_filename);
  assert(g_studio.filename != bak_filename);

  if (bak_filename != NULL)
  {
    assert(old_filename != NULL);

    if (stat(old_filename, &st) == 0) /* if old filename does not exist, rename with fail */
    {
      if (rename(old_filename, bak_filename) != 0)
      {
        lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "rename(%s, %s) failed: %d (%s)", old_filename, bak_filename, errno, strerror(errno));
        goto free_filenames;
      }
    }
    else
    {
      /* mark that there is no backup file */
      free(bak_filename);
      bak_filename = NULL;
    }
  }

  lash_info("saving studio... (%s)", g_studio.filename);

  fd = open(g_studio.filename, O_WRONLY | O_TRUNC | O_CREAT, 0700);
  if (fd == -1)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "open(%s) failed: %d (%s)", g_studio.filename, errno, strerror(errno));
    goto rename_back;
  }

  if (!write_string(fd, "<?xml version=\"1.0\"?>\n", call_ptr))
  {
    goto close;
  }

  if (!write_string(fd, "<!--\n", call_ptr))
  {
    goto close;
  }

  if (!write_string(fd, STUDIO_HEADER_TEXT, call_ptr))
  {
    goto close;
  }

  if (!write_string(fd, "-->\n", call_ptr))
  {
    goto close;
  }

  if (!write_string(fd, "<!-- ", call_ptr))
  {
    goto close;
  }

  if (!write_string(fd, timestamp_str, call_ptr))
  {
    goto close;
  }

  if (!write_string(fd, " -->\n", call_ptr))
  {
    goto close;
  }

  if (!write_string(fd, "<studio>\n", call_ptr))
  {
    goto close;
  }

  if (!write_string(fd, "  <jack>\n", call_ptr))
  {
    goto close;
  }

  if (!write_string(fd, "    <conf>\n", call_ptr))
  {
    goto close;
  }

  list_for_each(node_ptr, &g_studio.jack_params)
  {
    parameter_ptr = list_entry(node_ptr, struct jack_conf_parameter, leaves);

    if (!write_jack_parameter(fd, "      ", parameter_ptr, call_ptr))
    {
      goto close;
    }
  }

  if (!write_string(fd, "    </conf>\n", call_ptr))
  {
    goto close;
  }

  if (!write_string(fd, "  </jack>\n", call_ptr))
  {
    goto close;
  }

  if (!write_string(fd, "</studio>\n", call_ptr))
  {
    goto close;
  }

  lash_info("studio saved. (%s)", g_studio.filename);
  g_studio.persisted = true;
  g_studio.automatic = false;   /* even if it was automatic, it is not anymore because it is saved */
  ret = true;

close:
  close(fd);

rename_back:
  if (!ret && bak_filename != NULL)
  {
    /* save failed - try to rename the backup file back */
    if (rename(bak_filename, g_studio.filename) != 0)
    {
      lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "rename(%s, %s) failed: %d (%s)", bak_filename, g_studio.filename, errno, strerror(errno));
    }
  }

free_filenames:
  if (bak_filename != NULL)
  {
    free(bak_filename);
  }

  if (old_filename != NULL)
  {
    free(old_filename);
  }

  assert(filename == NULL);
  assert(g_studio.filename != NULL);

exit:
  return ret;
}

bool studios_iterate(void * call_ptr, void * context, bool (* callback)(void * call_ptr, void * context, const char * studio, uint32_t modtime))
{
  DIR * dir;
  struct dirent * dentry;
  size_t len;
  struct stat st;
  char * path;
  char * name;

  dir = opendir(g_studios_dir);
  if (dir == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "Cannot open directory '%s': %d (%s)", g_studios_dir, errno, strerror(errno));
    return false;
  }

  while ((dentry = readdir(dir)) != NULL)
  {
    if (dentry->d_type != DT_REG)
      continue;

    len = strlen(dentry->d_name);
    if (len <= 4 || strcmp(dentry->d_name + (len - 4), ".xml") != 0)
      continue;

    path = catdup(g_studios_dir, dentry->d_name);
    if (path == NULL)
    {
      lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "catdup() failed");
      return false;
    }

    if (stat(path, &st) != 0)
    {
      lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "failed to stat '%s': %d (%s)", path, errno, strerror(errno));
      free(path);
      return false;
    }

    free(path);

    name = malloc(len - 4 + 1);
    name[unescape(dentry->d_name, len - 4, name)] = 0;
    //lash_info("name = '%s'", name);

    if (!callback(call_ptr, context, name, st.st_mtime))
    {
      free(name);
      closedir(dir);
      return false;
    }

    free(name);
  }

  closedir(dir);
  return true;
}

#define context_ptr ((struct parse_context *)data)

static void callback_chrdata(void * data, const XML_Char * s, int len)
{
  if (context_ptr->error)
  {
    return;
  }

  if (context_ptr->element[context_ptr->depth] == PARSE_CONTEXT_PARAMETER)
  {
    if (context_ptr->data_used + len >= sizeof(context_ptr->data))
    {
      lash_dbus_error(context_ptr->call_ptr, LASH_DBUS_ERROR_GENERIC, "xml parse max char data length reached");
      context_ptr->error = XML_TRUE;
      return;
    }

    memcpy(context_ptr->data + context_ptr->data_used, s, len);
    context_ptr->data_used += len;
  }
}

static void callback_elstart(void * data, const char * el, const char ** attr)
{
  if (context_ptr->error)
  {
    return;
  }

  if (context_ptr->depth + 1 >= MAX_STACK_DEPTH)
  {
    lash_dbus_error(context_ptr->call_ptr, LASH_DBUS_ERROR_GENERIC, "xml parse max stack depth reached");
    context_ptr->error = XML_TRUE;
    return;
  }

  if (strcmp(el, "studio") == 0)
  {
    //lash_info("<studio>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_STUDIO;
    return;
  }

  if (strcmp(el, "jack") == 0)
  {
    //lash_info("<jack>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_JACK;
    return;
  }

  if (strcmp(el, "conf") == 0)
  {
    //lash_info("<conf>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_CONF;
    return;
  }

  if (strcmp(el, "parameter") == 0)
  {
    //lash_info("<parameter>");
    if ((attr[0] == NULL || attr[2] != NULL) || strcmp(attr[0], "path") != 0)
    {
      lash_dbus_error(context_ptr->call_ptr, LASH_DBUS_ERROR_GENERIC, "<parameter> XML element must contain exactly one attribute, named \"path\"");
      context_ptr->error = XML_TRUE;
      return;
    }

    context_ptr->path = strdup(attr[1]);
    if (context_ptr->path == NULL)
    {
      lash_dbus_error(context_ptr->call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup() failed");
      context_ptr->error = XML_TRUE;
      return;
    }

    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_PARAMETER;
    context_ptr->data_used = 0;
    return;
  }

  lash_dbus_error(context_ptr->call_ptr, LASH_DBUS_ERROR_GENERIC, "unknown element \"%s\"", el);
  context_ptr->error = XML_TRUE;
}

static void callback_elend(void * data, const char * el)
{
  char * src;
  char * dst;
  char * sep;
  size_t src_len;
  size_t dst_len;
  char * address;
  struct jack_parameter_variant parameter;
  bool is_set;

  if (context_ptr->error)
  {
    return;
  }

  //lash_info("element end (depth = %d, element = %u)", context_ptr->depth, context_ptr->element[context_ptr->depth]);

  if (context_ptr->element[context_ptr->depth] == PARSE_CONTEXT_PARAMETER &&
      context_ptr->depth == 3 &&
      context_ptr->element[0] == PARSE_CONTEXT_STUDIO &&
      context_ptr->element[1] == PARSE_CONTEXT_JACK &&
      context_ptr->element[2] == PARSE_CONTEXT_CONF)
  {
    context_ptr->data[context_ptr->data_used] = 0;

    //lash_info("'%s' with value '%s'", context_ptr->path, context_ptr->data);

    dst = address = strdup(context_ptr->path);
    src = context_ptr->path + 1;
    while (*src != 0)
    {
      sep = strchr(src, '/');
      if (sep == NULL)
      {
        src_len = strlen(src);
      }
      else
      {
        src_len = sep - src;
      }

      dst_len = unescape(src, src_len, dst);
      dst[dst_len] = 0;
      dst += dst_len + 1;

      src += src_len;
      assert(*src == '/' || *src == 0);
      if (sep != NULL)
      {
        assert(*src == '/');
        src++;                  /* skip separator */
      }
      else
      {
        assert(*src == 0);
      }
    }
    *dst = 0;                   /* ASCIZZ */

    if (!jack_proxy_get_parameter_value(address, &is_set, &parameter))
    {
      lash_dbus_error(context_ptr->call_ptr, LASH_DBUS_ERROR_GENERIC, "jack_proxy_get_parameter_value() failed");
      goto fail_free_address;
    }

    if (parameter.type == jack_string)
    {
      free(parameter.value.string);
    }

    switch (parameter.type)
    {
    case jack_boolean:
      lash_info("%s value is %s (boolean)", context_ptr->path, context_ptr->data);
      if (strcmp(context_ptr->data, "true") == 0)
      {
        parameter.value.boolean = true;
      }
      else if (strcmp(context_ptr->data, "false") == 0)
      {
        parameter.value.boolean = false;
      }
      else
      {
        lash_dbus_error(context_ptr->call_ptr, LASH_DBUS_ERROR_GENERIC, "bad value for a bool jack param");
        goto fail_free_address;
      }
      break;
    case jack_string:
      lash_info("%s value is %s (string)", context_ptr->path, context_ptr->data);
      parameter.value.string = context_ptr->data;
      break;
    case jack_byte:
      lash_debug("%s value is %u/%c (byte/char)", context_ptr->path, *context_ptr->data, *context_ptr->data);
      if (context_ptr->data[0] == 0 ||
          context_ptr->data[1] != 0)
      {
        lash_dbus_error(context_ptr->call_ptr, LASH_DBUS_ERROR_GENERIC, "bad value for a char jack param");
        goto fail_free_address;
      }
      parameter.value.byte = context_ptr->data[0];
      break;
    case jack_uint32:
      lash_info("%s value is %s (uint32)", context_ptr->path, context_ptr->data);
      if (sscanf(context_ptr->data, "%" PRIu32, &parameter.value.uint32) != 1)
      {
        lash_dbus_error(context_ptr->call_ptr, LASH_DBUS_ERROR_GENERIC, "bad value for an uint32 jack param");
        goto fail_free_address;
      }
      break;
    case jack_int32:
      lash_info("%s value is %s (int32)", context_ptr->path, context_ptr->data);
      if (sscanf(context_ptr->data, "%" PRIi32, &parameter.value.int32) != 1)
      {
        lash_dbus_error(context_ptr->call_ptr, LASH_DBUS_ERROR_GENERIC, "bad value for an int32 jack param");
        goto fail_free_address;
      }
      break;
    default:
      lash_dbus_error(context_ptr->call_ptr, LASH_DBUS_ERROR_GENERIC, "unknown jack parameter type %d of %s", (int)parameter.type, context_ptr->path);
      goto fail_free_address;
    }

    if (!jack_proxy_set_parameter_value(address, &parameter))
    {
      lash_dbus_error(context_ptr->call_ptr, LASH_DBUS_ERROR_GENERIC, "jack_proxy_set_parameter_value() failed");
      goto fail_free_address;
    }

    free(address);
  }

  context_ptr->depth--;

  if (context_ptr->path != NULL)
  {
    free(context_ptr->path);
    context_ptr->path = NULL;
  }

  return;

fail_free_address:
  free(address);
  context_ptr->error = XML_TRUE;
  return;
}

#undef context_ptr

bool studio_delete(void * call_ptr, const char * studio_name)
{
  char * filename;
  char * bak_filename;
  struct stat st;
  bool ret;

  ret = false;

  if (!compose_filename(studio_name, &filename, &bak_filename))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "failed to compose studio filename");
    goto exit;
  }

  lash_info("Deleting studio ('%s')", filename);

  if (unlink(filename) != 0)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "unlink(%s) failed: %d (%s)", filename, errno, strerror(errno));
    goto free;
  }

  /* try to delete the backup file */
  if (stat(bak_filename, &st) == 0)
  {
    if (unlink(bak_filename) != 0)
    {
      /* failing to delete backup file will not case delete command failure */
      lash_error("unlink(%s) failed: %d (%s)", bak_filename, errno, strerror(errno));
    }
  }

  ret = true;

free:
  free(filename);
  free(bak_filename);
exit:
  return ret;
}

bool studio_load(void * call_ptr, const char * studio_name)
{
  char * path;
  struct stat st;
  XML_Parser parser;
  int bytes_read;
  void * buffer;
  int fd;
  enum XML_Status xmls;
  struct parse_context context;

  if (!compose_filename(studio_name, &path, NULL))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "catdup3() failed to compose path of studio \%s\" file", studio_name);
    return false;
  }

  lash_info("Loading studio... ('%s')", path);

  if (stat(path, &st) != 0)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "failed to stat '%s': %d (%s)", path, errno, strerror(errno));
    free(path);
    return false;
 }

  studio_clear();

  g_studio.name = strdup(studio_name);
  if (g_studio.name == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup(\"%s\") failed", studio_name);
    free(path);
    return false;
  }

  g_studio.filename = path;

  if (!jack_reset_all_params())
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "jack_reset_all_params() failed");
    return false;
  }

  fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "failed to open '%s': %d (%s)", path, errno, strerror(errno));
    return false;
  }

  parser = XML_ParserCreate(NULL);
  if (parser == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "XML_ParserCreate() failed to create parser object.");
    close(fd);
    return false;
  }

  //lash_info("conf file size is %llu bytes", (unsigned long long)st.st_size);

  /* we are expecting that conf file has small enough size to fit in memory */

  buffer = XML_GetBuffer(parser, st.st_size);
  if (buffer == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "XML_GetBuffer() failed.");
    XML_ParserFree(parser);
    close(fd);
    return false;
  }

  bytes_read = read(fd, buffer, st.st_size);
  if (bytes_read != st.st_size)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "read() returned unexpected result.");
    XML_ParserFree(parser);
    close(fd);
    return false;
  }

  context.error = XML_FALSE;
  context.depth = -1;
  context.path = NULL;
  context.call_ptr = call_ptr;

  XML_SetElementHandler(parser, callback_elstart, callback_elend);
  XML_SetCharacterDataHandler(parser, callback_chrdata);
  XML_SetUserData(parser, &context);

  xmls = XML_ParseBuffer(parser, bytes_read, XML_TRUE);
  if (xmls == XML_STATUS_ERROR)
  {
    if (!context.error)         /* if we have initiated the fail, dbus error is already set to better message */
    {
      lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "XML_ParseBuffer() failed.");
    }
    XML_ParserFree(parser);
    close(fd);
    return false;
  }

  XML_ParserFree(parser);
  close(fd);

  g_studio.persisted = true;
  lash_info("Studio loaded. ('%s')", path);

  if (!studio_publish())
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "studio_publish() failed.");
    studio_clear();
    return false;
  }

  if (!studio_start())
  {
      lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "Failed to start JACK server.");
      studio_clear();
      return false;
  }

  return true;
}

void emit_studio_renamed()
{
  signal_new_valist(g_dbus_connection, STUDIO_OBJECT_PATH, IFACE_STUDIO, "StudioRenamed", DBUS_TYPE_STRING, &g_studio.name, DBUS_TYPE_INVALID);
}

static void ladish_get_studio_name(struct dbus_method_call * call_ptr)
{
  method_return_new_single(call_ptr, DBUS_TYPE_STRING, &g_studio.name);
}

static void ladish_rename_studio(struct dbus_method_call * call_ptr)
{
  const char * new_name;
  char * new_name_dup;
  
  if (!dbus_message_get_args(call_ptr->message, &g_dbus_error, DBUS_TYPE_STRING, &new_name, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

  lash_info("Rename studio request (%s)", new_name);

  new_name_dup = strdup(new_name);
  if (new_name_dup == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup() failed to allocate new name.");
    return;
  }

  free(g_studio.name);
  g_studio.name = new_name_dup;

  method_return_new_void(call_ptr);
  emit_studio_renamed();
}

static void ladish_save_studio(struct dbus_method_call * call_ptr)
{
  if (studio_save(call_ptr))
  {
    method_return_new_void(call_ptr);
  }
}

static void ladish_unload_studio(struct dbus_method_call * call_ptr)
{
  lash_info("Unload studio request");
  studio_clear();
  method_return_new_void(call_ptr);
}

bool studio_new(void * call_ptr, const char * studio_name)
{
  lash_info("New studio request (%s)", studio_name);
  studio_clear();

  assert(g_studio.name == NULL);
  if (*studio_name != 0)
  {
    g_studio.name = strdup(studio_name);
    if (g_studio.name == NULL)
    {
      lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup() failed to allocate studio name.");
      return false;
    }
  }
  else if (!studio_name_generate(&g_studio.name))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "studio_name_generate() failed.");
    return false;
  }

  if (!studio_publish())
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "studio_publish() failed.");
    studio_clear();
    return false;
  }

  return true;
}

static void ladish_stop_studio(struct dbus_method_call * call_ptr)
{
  lash_info("Studio stop requested");

  if (!studio_stop())
  {
      lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "Failed to stop studio.");
  }
  else
  {
    method_return_new_void(call_ptr);
  }
}

static void ladish_start_studio(struct dbus_method_call * call_ptr)
{
  lash_info("Studio start requested");

  if (!studio_start())
  {
      lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "Failed to start studio.");
  }
  else
  {
    method_return_new_void(call_ptr);
  }
}

METHOD_ARGS_BEGIN(GetName, "Get studio name")
  METHOD_ARG_DESCRIBE_OUT("studio_name", "s", "Name of studio")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Rename, "Rename studio")
  METHOD_ARG_DESCRIBE_IN("studio_name", "s", "New name")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Save, "Save studio")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Unload, "Unload studio")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Start, "Start studio")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Stop, "Stop studio")
METHOD_ARGS_END

METHODS_BEGIN
  METHOD_DESCRIBE(GetName, ladish_get_studio_name)
  METHOD_DESCRIBE(Rename, ladish_rename_studio)
  METHOD_DESCRIBE(Save, ladish_save_studio)
  METHOD_DESCRIBE(Unload, ladish_unload_studio)
  METHOD_DESCRIBE(Start, ladish_start_studio)
  METHOD_DESCRIBE(Stop, ladish_stop_studio)
METHODS_END

SIGNAL_ARGS_BEGIN(StudioRenamed, "Studio name changed")
  SIGNAL_ARG_DESCRIBE("studio_name", "s", "New studio name")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(StudioStarted, "Studio started")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(StudioStopped, "Studio stopped")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(RoomAppeared, "Room D-Bus object appeared")
  SIGNAL_ARG_DESCRIBE("room_path", "s", "room object path")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(RoomDisappeared, "Room D-Bus object disappeared")
  SIGNAL_ARG_DESCRIBE("room_path", "s", "room object path")
SIGNAL_ARGS_END

SIGNALS_BEGIN
  SIGNAL_DESCRIBE(StudioRenamed)
  SIGNAL_DESCRIBE(StudioStarted)
  SIGNAL_DESCRIBE(StudioStopped)
  SIGNAL_DESCRIBE(RoomAppeared)
  SIGNAL_DESCRIBE(RoomDisappeared)
SIGNALS_END

INTERFACE_BEGIN(g_interface_studio, IFACE_STUDIO)
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
  INTERFACE_EXPOSE_SIGNALS
INTERFACE_END
