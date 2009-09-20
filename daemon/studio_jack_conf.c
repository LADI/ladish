/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the studio functionality
 * related to jack configuration
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
#include "graph.h"
#include "../dbus_constants.h"
#include "control.h"
#include "../catdup.h"
#include "../dbus/error.h"
#include "dirhelpers.h"
#include "jack_dispatch.h"
#include "graph_dict.h"
#include "escape.h"
#include "studio_internal.h"

bool
jack_conf_container_create(
  struct jack_conf_container ** container_ptr_ptr,
  const char * name)
{
  struct jack_conf_container * container_ptr;

  container_ptr = malloc(sizeof(struct jack_conf_container));
  if (container_ptr == NULL)
  {
    log_error("malloc() failed to allocate struct jack_conf_container");
    goto fail;
  }

  container_ptr->name = strdup(name);
  if (container_ptr->name == NULL)
  {
    log_error("strdup() failed to duplicate \"%s\"", name);
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
    log_error("malloc() failed to allocate struct jack_conf_parameter");
    goto fail;
  }

  parameter_ptr->name = strdup(name);
  if (parameter_ptr->name == NULL)
  {
    log_error("strdup() failed to duplicate \"%s\"", name);
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
  log_info("jack_conf_parameter destroy");

  switch (parameter_ptr->parameter.type)
  {
  case jack_boolean:
    log_info("%s value is %s (boolean)", parameter_ptr->name, parameter_ptr->parameter.value.boolean ? "true" : "false");
    break;
  case jack_string:
    log_info("%s value is %s (string)", parameter_ptr->name, parameter_ptr->parameter.value.string);
    break;
  case jack_byte:
    log_info("%s value is %u/%c (byte/char)", parameter_ptr->name, parameter_ptr->parameter.value.byte, (char)parameter_ptr->parameter.value.byte);
    break;
  case jack_uint32:
    log_info("%s value is %u (uint32)", parameter_ptr->name, (unsigned int)parameter_ptr->parameter.value.uint32);
    break;
  case jack_int32:
    log_info("%s value is %u (int32)", parameter_ptr->name, (signed int)parameter_ptr->parameter.value.int32);
    break;
  default:
    log_error("unknown jack parameter_ptr->parameter type %d (%s)", (int)parameter_ptr->parameter.type, parameter_ptr->name);
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

  //log_info("\"%s\" jack_conf_parameter destroy", container_ptr->name);

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
    log_debug("ignoring drivers branch");
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
  ASSERT(context_ptr->address == address);
  dst = (char *)component;

  len = strlen(child) + 1;
  memcpy(dst, child, len);
  dst[len] = 0;

  if (leaf)
  {
    log_debug("%s (leaf)", path);

    if (parent_ptr == NULL)
    {
      log_error("jack conf parameters can't appear in root container");
      return false;
    }

    if (!parent_ptr->children_leafs)
    {
      if (!list_empty(&parent_ptr->children))
      {
        log_error("jack conf parameters cant be mixed with containers at same hierarchy level");
        return false;
      }

      parent_ptr->children_leafs = true;
    }

    if (!jack_conf_parameter_create(&parameter_ptr, child))
    {
      log_error("jack_conf_parameter_create() failed");
      return false;
    }

    if (!jack_proxy_get_parameter_value(context_ptr->address, &is_set, &parameter_ptr->parameter))
    {
      log_error("cannot get value of %s", path);
      return false;
    }

    if (is_set)
    {
#if 0
      switch (parameter_ptr->parameter.type)
      {
      case jack_boolean:
        log_info("%s value is %s (boolean)", path, parameter_ptr->parameter.value.boolean ? "true" : "false");
        break;
      case jack_string:
        log_info("%s value is %s (string)", path, parameter_ptr->parameter.value.string);
        break;
      case jack_byte:
        log_info("%s value is %u/%c (byte/char)", path, parameter_ptr->parameter.value.byte, (char)parameter_ptr->parameter.value.byte);
        break;
      case jack_uint32:
        log_info("%s value is %u (uint32)", path, (unsigned int)parameter_ptr->parameter.value.uint32);
        break;
      case jack_int32:
        log_info("%s value is %u (int32)", path, (signed int)parameter_ptr->parameter.value.int32);
        break;
      default:
        log_error("unknown jack parameter_ptr->parameter type %d (%s)", (int)parameter_ptr->parameter.type, path);
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
    log_debug("%s (container)", path);

    if (parent_ptr != NULL && parent_ptr->children_leafs)
    {
      log_error("jack conf containers cant be mixed with parameters at same hierarchy level");
      return false;
    }

    if (!jack_conf_container_create(&container_ptr, child))
    {
      log_error("jack_conf_container_create() failed");
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
      log_error("cannot read container %s", path);
      return false;
    }

    context_ptr->parent_ptr = parent_ptr;
  }

  *dst = 0;

  return true;
}

#undef context_ptr

void jack_conf_clear(void)
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

bool studio_fetch_jack_settings(void)
{
  struct conf_callback_context context;

  jack_conf_clear();

  context.address[0] = 0;
  context.container_ptr = &g_studio.jack_conf;
  context.parent_ptr = NULL;

  if (!jack_proxy_read_conf_container(context.address, &context, conf_callback))
  {
    log_error("jack_proxy_read_conf_container() failed.");
    return false;
  }

  return true;
}
