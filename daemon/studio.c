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
#include "../jack_proxy.h"
#include "patchbay.h"
#include "../dbus_constants.h"
#include "control.h"
#include "../catdup.h"
#include "../dbus/error.h"

extern const interface_t g_interface_studio;

struct studio
{
  /* this must be first member of struct studio because object_path_new() assumes all interfaces have same context */
  struct patchbay_implementator patchbay_impl;

  struct list_head all_connections;        /* All connections (studio guts and all rooms). Including superconnections. */
  struct list_head all_ports;              /* All ports (studio guts and all rooms) */
  struct list_head all_clients;            /* All clients (studio guts and all rooms) */
  struct list_head jack_connections;       /* JACK connections (studio guts and all rooms). Including superconnections, excluding virtual connections. */
  struct list_head jack_ports;             /* JACK ports (studio guts and all rooms). Excluding virtual ports. */
  struct list_head jack_clients;           /* JACK clients (studio guts and all rooms). Excluding virtual clients. */
  struct list_head rooms;                  /* Rooms connected to the studio */
  struct list_head clients;                /* studio clients (studio guts and room links) */
  struct list_head ports;                  /* studio ports (studio guts and room links) */

  bool persisted:1;             /* Studio has on-disk representation, i.e. can be reloaded from disk */
  bool modified:1;              /* Studio needs saving */
  bool jack_conf_valid:1;       /* JACK server configuration obtained successfully */

  struct list_head jack_conf;   /* root of the conf tree */
  struct list_head jack_params; /* list of conf tree leaves */

  object_path_t * dbus_object;

  struct list_head event_queue;

  char * name;
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
    lash_info("ignoring drivers branch");
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

bool studio_fetch_jack_settings()
{
  struct conf_callback_context context;

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
studio_activate(void)
{
  object_path_t * object;

  assert(g_studio.name != NULL);

  object = object_path_new(STUDIO_OBJECT_PATH, &g_studio, 2, &g_interface_studio, &g_interface_patchbay);
  if (object == NULL)
  {
    lash_error("object_path_new() failed");
    return false;
  }

  if (!object_path_register(g_dbus_connection, object))
  {
    lash_error("object_path_register() failed");
    object_path_destroy(g_dbus_connection, object);
    return false;
  }

  lash_info("Studio D-Bus object created. \"%s\"", g_studio.name);

  g_studio.dbus_object = object;

  emit_studio_appeared();

  return true;
}

void
studio_clear(void)
{
  struct list_head * node_ptr;

  g_studio.modified = false;
  g_studio.persisted = false;

  while (!list_empty(&g_studio.jack_conf))
  {
    node_ptr = g_studio.jack_conf.next;
    list_del(node_ptr);
    jack_conf_container_destroy(list_entry(node_ptr, struct jack_conf_container, siblings));
  }

  g_studio.jack_conf_valid = false;

  if (g_studio.dbus_object != NULL)
  {
    object_path_destroy(g_dbus_connection, g_studio.dbus_object);
    g_studio.dbus_object = NULL;
    emit_studio_disappeared();
  }

  if (g_studio.name != NULL)
  {
    free(g_studio.name);
    g_studio.name = NULL;
  }
}

void
studio_clear_if_not_persisted(void)
{
  if (!g_studio.persisted)
  {
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

    studio_activate();
  }

  if (!studio_fetch_jack_settings(g_studio))
  {
    lash_error("studio_fetch_jack_settings() failed.");

    studio_clear_if_not_persisted();
    return;
  }

  lash_info("jack conf successfully retrieved");
  g_studio.jack_conf_valid = true;
}

void on_event_jack_stopped(void)
{
  studio_clear_if_not_persisted();

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

  g_studio.patchbay_impl.this = &g_studio;
  g_studio.patchbay_impl.get_graph_version = studio_get_graph_version;

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
  studio_clear();

  if (!jack_proxy_init(
        on_jack_server_started,
        on_jack_server_stopped,
        on_jack_server_appeared,
        on_jack_server_disappeared))
  {
    return false;
  }

  return true;
}

void studio_uninit(void)
{
  jack_proxy_uninit();

  studio_clear();

  lash_info("studio object destroy");
}

bool studio_is_loaded(void)
{
  return g_studio.dbus_object != NULL;
}

bool studio_save()
{
  struct list_head * node_ptr;
  struct jack_conf_parameter * parameter_ptr;
  char path[JACK_CONF_MAX_ADDRESS_SIZE * 3]; /* encode each char in three bytes (percent encoding) */
  const char * src;
  char * dst;
  static char hex_digits[] = "0123456789ABCDEF";

  lash_info("saving studio...");

  list_for_each(node_ptr, &g_studio.jack_params)
  {
    parameter_ptr = list_entry(node_ptr, struct jack_conf_parameter, leaves);

    /* compose the parameter path, percent-encode "bad" chars */
    src = parameter_ptr->address;
    dst = path;
    do
    {
      *dst++ = '/';
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
      src++;
    }
    while (*src != 0);
    *dst = 0;

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
      return false;
    }
  }

  return false;                 /* not implemented yet */
}

bool studio_load(const char * file_path)
{
  return false;                 /* not implemented yet */
}

void emit_studio_renamed()
{
  signal_new_valist(g_dbus_connection, STUDIO_OBJECT_PATH, IFACE_STUDIO, "StudioRenamed", DBUS_TYPE_STRING, &g_studio.name, DBUS_TYPE_INVALID);
}

static void ladish_get_studio_name(method_call_t * call_ptr)
{
  method_return_new_single(call_ptr, DBUS_TYPE_STRING, &g_studio.name);
}

static void ladish_rename_studio(method_call_t * call_ptr)
{
  const char * new_name;
  char * new_name_dup;
  
  if (!dbus_message_get_args(call_ptr->message, &g_dbus_error, DBUS_TYPE_STRING, &new_name, DBUS_TYPE_INVALID))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_INVALID_ARGS, "Invalid arguments to method \"%s\": %s",  call_ptr->method_name, g_dbus_error.message);
    dbus_error_free(&g_dbus_error);
    return;
  }

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

static void ladish_save_studio(method_call_t * call_ptr)
{
  studio_save();
  method_return_new_void(call_ptr);
}

METHOD_ARGS_BEGIN(GetName, "Get studio name")
  METHOD_ARG_DESCRIBE_OUT("studio_name", "s", "Name of studio")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Rename, "Rename studio")
  METHOD_ARG_DESCRIBE_IN("studio_name", "s", "New name")
METHOD_ARGS_END

METHOD_ARGS_BEGIN(Save, "Save studio")
METHOD_ARGS_END

METHODS_BEGIN
  METHOD_DESCRIBE(GetName, ladish_get_studio_name)
  METHOD_DESCRIBE(Rename, ladish_rename_studio)
  METHOD_DESCRIBE(Save, ladish_save_studio)
METHODS_END

SIGNAL_ARGS_BEGIN(StudioRenamed, "Studio name changed")
  SIGNAL_ARG_DESCRIBE("studio_name", "s", "New studio name")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(RoomAppeared, "Room D-Bus object appeared")
  SIGNAL_ARG_DESCRIBE("room_path", "s", "room object path")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(RoomDisappeared, "Room D-Bus object disappeared")
  SIGNAL_ARG_DESCRIBE("room_path", "s", "room object path")
SIGNAL_ARGS_END

SIGNALS_BEGIN
  SIGNAL_DESCRIBE(StudioRenamed)
  SIGNAL_DESCRIBE(RoomAppeared)
  SIGNAL_DESCRIBE(RoomDisappeared)
SIGNALS_END

INTERFACE_BEGIN(g_interface_studio, IFACE_STUDIO)
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
  INTERFACE_EXPOSE_SIGNALS
INTERFACE_END
