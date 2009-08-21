/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains studio object helpers
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
  bool jack_conf_stable:1;      /* JACK server configuration obtained successfully */

  struct list_head jack_conf;   /* root of the conf tree */
  struct list_head jack_params; /* list of conf tree leaves */

  object_path_t * dbus_object;

  struct patchbay_implementator patchbay_implementator;
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
  struct studio * studio_ptr;
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

  assert(context_ptr->studio_ptr);

  parent_ptr = context_ptr->parent_ptr;

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
      list_add_tail(&parameter_ptr->leaves, &context_ptr->studio_ptr->jack_params);
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
      list_add_tail(&container_ptr->siblings, &context_ptr->studio_ptr->jack_conf);
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

#define studio_ptr ((struct studio *)this)

uint64_t
studio_get_graph_version(
  void * this)
{
  //lash_info("studio_get_graph_version() called");
  return 1;
}

#undef studio_ptr

bool
studio_create(
  studio_handle * studio_handle_ptr)
{
  struct studio * studio_ptr;

  lash_info("studio object construct");

  studio_ptr = malloc(sizeof(struct studio));
  if (studio_ptr == NULL)
  {
    lash_error("malloc() failed to allocate struct studio");
    return false;
  }

  studio_ptr->patchbay_impl.this = studio_ptr;
  studio_ptr->patchbay_impl.get_graph_version = studio_get_graph_version;

  INIT_LIST_HEAD(&studio_ptr->all_connections);
  INIT_LIST_HEAD(&studio_ptr->all_ports);
  INIT_LIST_HEAD(&studio_ptr->all_clients);
  INIT_LIST_HEAD(&studio_ptr->jack_connections);
  INIT_LIST_HEAD(&studio_ptr->jack_ports);
  INIT_LIST_HEAD(&studio_ptr->jack_clients);
  INIT_LIST_HEAD(&studio_ptr->rooms);
  INIT_LIST_HEAD(&studio_ptr->clients);
  INIT_LIST_HEAD(&studio_ptr->ports);

  studio_ptr->modified = false;
  studio_ptr->persisted = false;
  studio_ptr->jack_conf_stable = false;

  INIT_LIST_HEAD(&studio_ptr->jack_conf);
  INIT_LIST_HEAD(&studio_ptr->jack_params);

  studio_ptr->dbus_object = NULL;

  *studio_handle_ptr = (studio_handle)studio_ptr;

  return true;
}

#define studio_ptr ((struct studio *)studio)

void
studio_destroy(
  studio_handle studio)
{
  struct list_head * node_ptr;

  if (studio_ptr->dbus_object != NULL)
  {
    object_path_destroy(g_dbus_connection, studio_ptr->dbus_object);
  }

  while (!list_empty(&studio_ptr->jack_conf))
  {
    node_ptr = studio_ptr->jack_conf.next;
    list_del(node_ptr);
    jack_conf_container_destroy(list_entry(node_ptr, struct jack_conf_container, siblings));
  }

  free(studio_ptr);
  lash_info("studio object destroy");
}

bool
studio_activate(
  studio_handle studio)
{
  object_path_t * object;

  object = object_path_new(DBUS_BASE_PATH "/Studio", studio, 2, &g_interface_studio, &g_interface_patchbay);
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

  lash_info("Studio D-Bus object created.");

  studio_ptr->dbus_object = object;

  return true;
}

bool
studio_is_persisted(
  studio_handle studio)
{
  return studio_ptr->persisted;
}

bool
studio_save(
  studio_handle studio,
  const char * file_path)
{
  struct list_head * node_ptr;
  struct jack_conf_parameter * parameter_ptr;
  char path[JACK_CONF_MAX_ADDRESS_SIZE * 3]; /* encode each char in three bytes (percent encoding) */
  const char * src;
  char * dst;
  static char hex_digits[] = "0123456789ABCDEF";

  lash_info("saving studio...");

  list_for_each(node_ptr, &studio_ptr->jack_params)
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

bool
studio_load(
  studio_handle studio,
  const char * file_path)
{
  return false;                 /* not implemented yet */
}

#undef studio_ptr

bool
studio_fetch_jack_settings(
  studio_handle studio)
{
  struct conf_callback_context context;

  context.address[0] = 0;
  context.studio_ptr = (struct studio *)studio;
  context.container_ptr = &context.studio_ptr->jack_conf;
  context.parent_ptr = NULL;

  if (!jack_proxy_read_conf_container(context.address, &context, conf_callback))
  {
    lash_error("jack_proxy_read_conf_container() failed.");
    return false;
  }

  return true;
}

METHODS_BEGIN
METHODS_END

SIGNAL_ARGS_BEGIN(RoomAppeared, "Room D-Bus object appeared")
  SIGNAL_ARG_DESCRIBE("room_path", "s", "room object path")
SIGNAL_ARGS_END

SIGNAL_ARGS_BEGIN(RoomDisappeared, "Room D-Bus object disappeared")
  SIGNAL_ARG_DESCRIBE("room_path", "s", "room object path")
SIGNAL_ARGS_END

SIGNALS_BEGIN
  SIGNAL_DESCRIBE(RoomAppeared)
  SIGNAL_DESCRIBE(RoomDisappeared)
SIGNALS_END

INTERFACE_BEGIN(g_interface_studio, DBUS_NAME_BASE ".Studio")
  INTERFACE_DEFAULT_HANDLER
  INTERFACE_EXPOSE_METHODS
  INTERFACE_EXPOSE_SIGNALS
INTERFACE_END
