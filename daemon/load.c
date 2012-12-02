/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009,2010,2011,2012 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation for the load helper functions
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

#include "load.h"
#include "limits.h"
#include "studio.h"
#include "../proxies/jmcore_proxy.h"

void ladish_dump_element_stack(struct ladish_parse_context * context_ptr)
{
  signed int depth;
  const char * descr;

  log_info("depth=%d", context_ptr->depth);

  for (depth = context_ptr->depth; depth >= 0; depth--)
  {
    switch (context_ptr->element[depth])
    {
    case PARSE_CONTEXT_ROOT:
      descr = "root";
      break;
    case PARSE_CONTEXT_STUDIO:
      descr = "studio";
      break;
    case PARSE_CONTEXT_JACK:
      descr = "jack";
      break;
    case PARSE_CONTEXT_CONF:
      descr = "conf";
      break;
    case PARSE_CONTEXT_PARAMETER:
      descr = "parameter";
      break;
    case PARSE_CONTEXT_CLIENTS:
      descr = "clients";
      break;
    case PARSE_CONTEXT_CLIENT:
      descr = "client";
      break;
    case PARSE_CONTEXT_PORTS:
      descr = "ports";
      break;
    case PARSE_CONTEXT_PORT:
      descr = "port";
      break;
    case PARSE_CONTEXT_DICT:
      descr = "dict";
      break;
    case PARSE_CONTEXT_KEY:
      descr = "key";
      break;
    case PARSE_CONTEXT_CONNECTIONS:
      descr = "connections";
      break;
    case PARSE_CONTEXT_CONNECTION:
      descr = "connection";
      break;
    case PARSE_CONTEXT_APPLICATIONS:
      descr = "applications";
      break;
    case PARSE_CONTEXT_APPLICATION:
      descr = "application";
      break;
    case PARSE_CONTEXT_ROOMS:
      descr = "rooms";
      break;
    case PARSE_CONTEXT_ROOM:
      descr = "room";
      break;
    case PARSE_CONTEXT_PROJECT:
      descr = "project";
      break;
    default:
      descr = "?";
    }

    log_info("%d - %u (%s)", depth, context_ptr->element[depth], descr);
  }
}

static const char * get_string_attribute_internal(const char * const * attr, const char * key, bool optional)
{
  while (attr[0] != NULL)
  {
    ASSERT(attr[1] != NULL);
    if (strcmp(attr[0], key) == 0)
    {
      return attr[1];
    }
    attr += 2;
  }

  if (!optional)
  {
    log_error("attribute \"%s\" is missing", key);
  }

  return NULL;
}

const char * ladish_get_string_attribute(const char * const * attr, const char * key)
{
  return get_string_attribute_internal(attr, key, false);
}

const char * ladish_get_uuid_attribute(const char * const * attr, const char * key, uuid_t uuid, bool optional)
{
  const char * value;

  value = get_string_attribute_internal(attr, key, optional);
  if (value == NULL)
  {
    return NULL;
  }

  if (uuid_parse(value, uuid) != 0)
  {
    log_error("cannot parse uuid \"%s\"", value);
    return NULL;
  }

  return value;
}

const char * ladish_get_bool_attribute(const char * const * attr, const char * key, bool * bool_value_ptr)
{
  const char * value_str;

  value_str = ladish_get_string_attribute(attr, key);
  if (value_str == NULL)
  {
    return NULL;
  }

  if (strcmp(value_str, "true") == 0)
  {
    *bool_value_ptr = true;
    return value_str;
  }

  if (strcmp(value_str, "false") == 0)
  {
    *bool_value_ptr = false;
    return value_str;
  }

  log_error("boolean XML attribute has value of \"%s\" but only \"true\" and \"false\" are valid", value_str);
  return NULL;
}

const char * ladish_get_byte_attribute(const char * const * attr, const char * key, uint8_t * byte_value_ptr)
{
  const char * value_str;
  long int li_value;
  char * end_ptr;

  value_str = ladish_get_string_attribute(attr, key);
  if (value_str == NULL)
  {
    return NULL;
  }

  errno = 0;    /* To distinguish success/failure after call */
  li_value = strtol(value_str, &end_ptr, 10);
  if ((errno == ERANGE && (li_value == LONG_MAX || li_value == LONG_MIN)) || (errno != 0 && li_value == 0) || end_ptr == value_str)
  {
    log_error("value '%s' of attribute '%s' is not valid integer.", value_str, key);
    return NULL;
  }

  if (li_value < 0 || li_value > 255)
  {
    log_error("value '%s' of attribute '%s' is not valid uint8.", value_str, key);
    return NULL;
  }

  *byte_value_ptr = (uint8_t)li_value;
  return value_str;
}

bool
ladish_get_name_and_uuid_attributes(
  const char * element_description,
  const char * const * attr,
  const char ** name_str_ptr,
  const char ** uuid_str_ptr,
  uuid_t uuid)
{
  const char * name_str;
  const char * uuid_str;

  name_str = ladish_get_string_attribute(attr, "name");
  if (name_str == NULL)
  {
    log_error("%s \"name\" attribute is not available", element_description);
    return false;
  }

  uuid_str = ladish_get_uuid_attribute(attr, "uuid", uuid, false);
  if (uuid_str == NULL)
  {
    log_error("%s \"uuid\" attribute is not available. name=\"%s\"", element_description, name_str);
    return false;
  }

  *name_str_ptr = name_str;
  *uuid_str_ptr = uuid_str;
  return true;
}

bool
ladish_parse_port_type_and_direction_attributes(
  const char * element_description,
  const char * const * attr,
  uint32_t * type_ptr,
  uint32_t * flags_ptr)
{
  const char * type_str;
  const char * direction_str;

  type_str = ladish_get_string_attribute(attr, "type");
  if (type_str == NULL)
  {
    log_error("%s \"type\" attribute is not available", element_description);
    return false;
  }

  direction_str = ladish_get_string_attribute(attr, "direction");
  if (direction_str == NULL)
  {
    log_error("%s \"direction\" attribute is not available", element_description);
    return false;
  }

  if (strcmp(type_str, "midi") == 0)
  {
    *type_ptr = JACKDBUS_PORT_TYPE_MIDI;
  }
  else if (strcmp(type_str, "audio") == 0)
  {
    *type_ptr = JACKDBUS_PORT_TYPE_AUDIO;
  }
  else
  {
    log_error("%s \"type\" attribute contains unknown value \"%s\"", element_description, type_str);
    return false;
  }

  if (strcmp(direction_str, "playback") == 0)
  {
    *flags_ptr = 0;
    JACKDBUS_PORT_SET_INPUT(*flags_ptr);
  }
  else if (strcmp(direction_str, "capture") == 0)
  {
    *flags_ptr = 0;
    JACKDBUS_PORT_SET_OUTPUT(*flags_ptr);
  }
  else
  {
    log_error("%s \"direction\" attribute contains unknown value \"%s\"", element_description, direction_str);
    return false;
  }

  return true;
}

struct interlink_context
{
  ladish_graph_handle vgraph;
  ladish_app_supervisor_handle app_supervisor;
};

#define ctx_ptr ((struct interlink_context *)context)

static
bool
interlink_client(
  void * context,
  ladish_graph_handle UNUSED(graph_handle),
  bool UNUSED(hidden),
  ladish_client_handle jclient,
  const char * name,
  void ** UNUSED(client_iteration_context_ptr_ptr))
{
  uuid_t app_uuid;
  uuid_t vclient_app_uuid;
  uuid_t vclient_uuid;
  ladish_client_handle vclient;
  pid_t pid;
  ladish_app_handle app;
  bool interlinked;
  bool jmcore;
  ladish_graph_handle vgraph;

  ASSERT(ctx_ptr->vgraph != NULL);
  vgraph = ladish_client_get_vgraph(jclient);
  if (vgraph != NULL && ctx_ptr->vgraph != vgraph)
  {
    /* skip clients of different vgraphs */
    return true;
  }

  if (strcmp(name, "system") == 0)
  {
    return true;
  }

  interlinked = ladish_client_get_interlink(jclient, vclient_uuid);

  if (ladish_client_has_app(jclient))
  {
    ASSERT(interlinked); /* jclient has app associated but is not interlinked */
    return true;
  }
  else if (interlinked)
  {
    /* jclient has no app associated but is interlinked */
    /* this can happen if there is an external app (presumably in a different vgraph) */
    ASSERT_NO_PASS;             /* if vgraph is different, then we should have skipped the client earlier */
    return true;
  }
  ASSERT(!interlinked);

  /* XXX: why this is is here is a mystery. interlink is supposed to be called just after load so pid will always be 0 */
  pid = ladish_client_get_pid(jclient);
  jmcore = pid != 0 && pid == jmcore_proxy_get_pid_cached();
  if (jmcore)
  {
    return true;
  }

  if (ladish_virtualizer_is_a2j_client(jclient))
  {
    return true;
  }

  app = ladish_app_supervisor_find_app_by_name(ctx_ptr->app_supervisor, name);
  if (app == NULL)
  {
    log_info("JACK client \"%s\" not found in app supervisor", name);
    return true;
  }

  vclient = ladish_graph_find_client_by_name(ctx_ptr->vgraph, name, false);
  if (vclient == NULL)
  {
    log_error("JACK client '%s' has no vclient associated", name);
    return true;
  }

  log_info("Interlinking clients of app '%s'", name);
  ladish_client_interlink(jclient, vclient);

  ladish_app_get_uuid(app, app_uuid);
  if (ladish_client_get_app(vclient, vclient_app_uuid))
  {
    if (uuid_compare(app_uuid, vclient_app_uuid) != 0)
    {
      log_error("vclient of app '%s' already has a different app uuid", name);
    }
  }
  else
  {
    log_info("associating vclient with app '%s'", name);
    ladish_client_set_app(vclient, app_uuid);
  }

  ladish_client_set_app(jclient, app_uuid);
  ladish_client_set_vgraph(jclient, ctx_ptr->vgraph);

  return true;
}

bool
interlink_port(
  void * UNUSED(context),
  ladish_graph_handle UNUSED(graph_handle),
  bool UNUSED(hidden),
  void * UNUSED(client_iteration_context_ptr),
  ladish_client_handle client_handle,
  const char * UNUSED(client_name),
  ladish_port_handle port_handle,
  const char * UNUSED(port_name),
  uint32_t UNUSED(port_type),
  uint32_t UNUSED(port_flags))
{
  uuid_t app_uuid;

  if (ladish_client_get_app(client_handle, app_uuid))
  {
    ladish_port_set_app(port_handle, app_uuid);
  }

  return true;
}

void ladish_interlink(ladish_graph_handle vgraph, ladish_app_supervisor_handle app_supervisor)
{
  struct interlink_context ctx;

  ctx.vgraph = vgraph;
  ctx.app_supervisor = app_supervisor;

  ladish_graph_iterate_nodes(ladish_studio_get_jack_graph(), &ctx, interlink_client, NULL, NULL);
  ladish_graph_iterate_nodes(vgraph, &ctx, NULL, interlink_port, NULL);
}
