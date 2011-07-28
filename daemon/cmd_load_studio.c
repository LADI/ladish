/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010, 2011 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the "load studio" command
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

#include "escape.h"
#include "cmd.h"
#include "studio_internal.h"
#include "../proxies/notify_proxy.h"
#include "load.h"

#define context_ptr ((struct ladish_parse_context *)data)

static void callback_chrdata(void * data, const XML_Char * s, int len)
{
  if (context_ptr->error)
  {
    return;
  }

  if (context_ptr->element[context_ptr->depth] == PARSE_CONTEXT_PARAMETER ||
      context_ptr->element[context_ptr->depth] == PARSE_CONTEXT_KEY ||
      context_ptr->element[context_ptr->depth] == PARSE_CONTEXT_APPLICATION)
  {
    if (context_ptr->data_used + len >= sizeof(context_ptr->data))
    {
      log_error("xml parse max char data length reached");
      context_ptr->error = XML_TRUE;
      return;
    }

    memcpy(context_ptr->data + context_ptr->data_used, s, len);
    context_ptr->data_used += len;
  }
}

static void callback_elstart(void * data, const char * el, const char ** attr)
{
  const char * name;
  const char * level;
  char * name_dup;
  const char * path;
  const char * uuid_str;
  uuid_t uuid;
  const char * uuid2_str;
  uuid_t uuid2;
  ladish_port_handle port1;
  ladish_port_handle port2;
  uint32_t port_type;
  uint32_t port_flags;
  size_t len;

  name_dup = NULL;

  if (context_ptr->error)
  {
    goto free;
  }

  if (context_ptr->depth + 1 >= MAX_STACK_DEPTH)
  {
    log_error("xml parse max stack depth reached");
    context_ptr->error = XML_TRUE;
    goto free;
  }

  if (strcmp(el, "studio") == 0)
  {
    //log_info("<studio>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_STUDIO;
    goto free;
  }

  if (strcmp(el, "jack") == 0)
  {
    //log_info("<jack>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_JACK;
    goto free;
  }

  if (strcmp(el, "conf") == 0)
  {
    //log_info("<conf>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_CONF;
    goto free;
  }

  if (strcmp(el, "parameter") == 0)
  {
    //log_info("<parameter>");
    path = ladish_get_string_attribute(attr, "path");
    if (path == NULL)
    {
      log_error("<parameter> XML element without \"path\" attribute");
      context_ptr->error = XML_TRUE;
      goto free;
    }

    context_ptr->str = strdup(path);
    if (context_ptr->str == NULL)
    {
      log_error("strdup() failed");
      context_ptr->error = XML_TRUE;
      goto free;
    }

    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_PARAMETER;
    context_ptr->data_used = 0;
    goto free;
  }

  if (strcmp(el, "clients") == 0)
  {
    //log_info("<clients>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_CLIENTS;
    goto free;
  }

  if (strcmp(el, "rooms") == 0)
  {
    //log_info("<rooms>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_ROOMS;
    goto free;
  }

  if (strcmp(el, "room") == 0)
  {
    //log_info("<room>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_ROOM;

    if (context_ptr->room != NULL)
    {
        log_error("nested rooms");
        context_ptr->error = XML_TRUE;
        goto free;
    }

    if (context_ptr->depth == 2 &&
        context_ptr->element[0] == PARSE_CONTEXT_STUDIO &&
        context_ptr->element[1] == PARSE_CONTEXT_ROOMS)
    {
      if (!ladish_get_name_and_uuid_attributes("/studio/rooms/room", attr, &name, &uuid_str, uuid))
      {
        context_ptr->error = XML_TRUE;
        goto free;
      }

      if (!ladish_room_create(uuid, name, NULL, g_studio.studio_graph, &context_ptr->room))
      {
        log_error("ladish_room_create() failed.");
        context_ptr->error = XML_TRUE;
        ASSERT(context_ptr->room == NULL);
        goto free;
      }

      goto free;
    }

    log_error("ignoring <room> element in wrong context");
    goto free;
  }

  if (strcmp(el, "client") == 0)
  {
    //log_info("<client>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_CLIENT;

    if (context_ptr->client != NULL)
    {
        log_error("nested clients");
        context_ptr->error = XML_TRUE;
        goto free;
    }

    if (context_ptr->depth == 3 &&
        context_ptr->element[0] == PARSE_CONTEXT_STUDIO &&
        context_ptr->element[1] == PARSE_CONTEXT_JACK &&
        context_ptr->element[2] == PARSE_CONTEXT_CLIENTS)
    {
      if (!ladish_get_name_and_uuid_attributes("/studio/jack/clients/client", attr, &name, &uuid_str, uuid))
      {
        context_ptr->error = XML_TRUE;
        goto free;
      }

      name_dup = unescape_dup(name);
      if (name_dup == NULL)
      {
        log_error("allocation of memory for unescaped name buffer failed. name = '%s'", name);
        context_ptr->error = XML_TRUE;
        goto free;
      }

      log_info("jack client \"%s\" with uuid %s", name_dup, uuid_str);

      if (!ladish_client_create(uuid, &context_ptr->client))
      {
        log_error("ladish_client_create() failed.");
        context_ptr->error = XML_TRUE;
        ASSERT(context_ptr->client == NULL);
        goto free;
      }

      if (!ladish_graph_add_client(g_studio.jack_graph, context_ptr->client, name_dup, true))
      {
        log_error("ladish_graph_add_client() failed to add client '%s' to JACK graph", name_dup);
        context_ptr->error = XML_TRUE;
        ladish_client_destroy(context_ptr->client);
        context_ptr->client = NULL;
        goto free;
      }
    }
    else if (context_ptr->depth == 2 &&
             context_ptr->element[0] == PARSE_CONTEXT_STUDIO &&
             context_ptr->element[1] == PARSE_CONTEXT_CLIENTS)
    {
      if (!ladish_get_name_and_uuid_attributes("/studio/clients/client", attr, &name, &uuid_str, uuid))
      {
        context_ptr->error = XML_TRUE;
        goto free;
      }

      name_dup = unescape_dup(name);
      if (name_dup == NULL)
      {
        log_error("allocation of memory for unescaped name buffer failed. name = '%s'", name);
        context_ptr->error = XML_TRUE;
        goto free;
      }

      log_info("studio client \"%s\" with uuid %s", name_dup, uuid_str);

      context_ptr->client = ladish_graph_find_client_by_uuid(g_studio.studio_graph, uuid);
      if (context_ptr->client != NULL)
      {
        log_info("Found existing client");
        goto free;
      }

      if (!ladish_client_create(uuid, &context_ptr->client))
      {
        log_error("ladish_client_create() failed.");
        context_ptr->error = XML_TRUE;
        ASSERT(context_ptr->client == NULL);
        goto free;
      }

      if (ladish_get_uuid_attribute(attr, "app", context_ptr->uuid, true))
      {
        ladish_client_set_app(context_ptr->client, context_ptr->uuid);
      }

      if (!ladish_graph_add_client(g_studio.studio_graph, context_ptr->client, name_dup, true))
      {
        log_error("ladish_graph_add_client() failed to add client '%s' to studio graph", name_dup);
        context_ptr->error = XML_TRUE;
        ladish_client_destroy(context_ptr->client);
        context_ptr->client = NULL;
        goto free;
      }
    }

    goto free;
  }

  if (strcmp(el, "ports") == 0)
  {
    //log_info("<ports>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_PORTS;
    goto free;
  }

  if (strcmp(el, "port") == 0)
  {
    //log_info("<port>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_PORT;

    if (context_ptr->port != NULL)
    {
        log_error("nested ports");
        context_ptr->error = XML_TRUE;
        goto free;
    }

    if (context_ptr->depth >= 3 &&
        context_ptr->element[context_ptr->depth - 3] == PARSE_CONTEXT_CLIENTS &&
        context_ptr->element[context_ptr->depth - 2] == PARSE_CONTEXT_CLIENT &&
        context_ptr->element[context_ptr->depth - 1] == PARSE_CONTEXT_PORTS)
    {
      //log_info("client port");
      if (context_ptr->client == NULL)
      {
        log_error("client-less port");
        context_ptr->error = XML_TRUE;
        goto free;
      }

      if (context_ptr->depth == 5 && context_ptr->element[0] == PARSE_CONTEXT_STUDIO && context_ptr->element[1] == PARSE_CONTEXT_JACK)
      {
        if (!ladish_get_name_and_uuid_attributes("/studio/jack/clients/client/ports/port", attr, &name, &uuid_str, uuid))
        {
          context_ptr->error = XML_TRUE;
          goto free;
        }

        name_dup = unescape_dup(name);
        if (name_dup == NULL)
        {
          log_error("allocation of memory for unescaped name buffer failed. name = '%s'", name);
          context_ptr->error = XML_TRUE;
          goto free;
        }

        log_info("jack port \"%s\" with uuid %s", name_dup, uuid_str);

        if (!ladish_port_create(uuid, false, &context_ptr->port))
        {
          log_error("ladish_port_create() failed.");
          goto free;
        }

        ladish_port_set_vgraph(context_ptr->port, g_studio.studio_graph);

        if (!ladish_graph_add_port(g_studio.jack_graph, context_ptr->client, context_ptr->port, name_dup, 0, 0, true))
        {
          log_error("ladish_graph_add_port() failed.");
          ladish_port_destroy(context_ptr->port);
          context_ptr->port = NULL;
        }

        goto free;
      }
      else if (context_ptr->depth == 4 && context_ptr->element[0] == PARSE_CONTEXT_STUDIO)
      {
        if (!ladish_get_name_and_uuid_attributes("/studio/clients/client/ports/port", attr, &name, &uuid_str, uuid))
        {
          context_ptr->error = XML_TRUE;
          goto free;
        }

        name_dup = unescape_dup(name);
        if (name_dup == NULL)
        {
          log_error("allocation of memory for unescaped name buffer failed. name = '%s'", name);
          context_ptr->error = XML_TRUE;
          goto free;
        }

        uuid2_str = ladish_get_uuid_attribute(attr, "link_uuid", uuid2, true);

        log_info("studio port \"%s\" with uuid %s (%s)", name_dup, uuid_str, uuid2_str == NULL ? "normal" : "room link");

        if (uuid2_str == NULL)
        { /* normal studio port */
          context_ptr->port = ladish_graph_find_port_by_uuid(g_studio.jack_graph, uuid, false, g_studio.studio_graph);
          if (context_ptr->port == NULL)
          {
            log_error("studio client with non-jack port %s", uuid_str);
            context_ptr->error = XML_TRUE;
            goto free;
          }

          if (!ladish_graph_add_port(g_studio.studio_graph, context_ptr->client, context_ptr->port, name_dup, 0, 0, true))
          {
            log_error("ladish_graph_add_port() failed.");
            ladish_port_destroy(context_ptr->port);
            context_ptr->port = NULL;
            goto free;
          }

          goto free;
        }

        /* room link port */
        context_ptr->port = ladish_graph_find_client_port_by_uuid(g_studio.studio_graph, context_ptr->client, uuid2, false);
        if (context_ptr->port == NULL)
        {
          log_error("room link port not found");
          context_ptr->error = XML_TRUE;
          goto free;
        }

        ladish_graph_set_link_port_override_uuid(g_studio.studio_graph, context_ptr->port, uuid);
        goto free;
      }
    }
    else if (context_ptr->depth == 3 &&
             context_ptr->element[0] == PARSE_CONTEXT_STUDIO &&
             context_ptr->element[1] == PARSE_CONTEXT_ROOMS &&
             context_ptr->element[2] == PARSE_CONTEXT_ROOM)
    {
      ASSERT(context_ptr->room != NULL);
      //log_info("room port");

      if (!ladish_get_name_and_uuid_attributes("/studio/rooms/room/port", attr, &name, &uuid_str, uuid))
      {
        context_ptr->error = XML_TRUE;
        goto free;
      }

      name_dup = unescape_dup(name);
      if (name_dup == NULL)
      {
        log_error("allocation of memory for unescaped name buffer failed. name = '%s'", name);
        context_ptr->error = XML_TRUE;
        goto free;
      }

      log_info("room port \"%s\" with uuid %s", name_dup, uuid_str);

      if (!ladish_parse_port_type_and_direction_attributes("/studio/rooms/room/port", attr, &port_type, &port_flags))
      {
        context_ptr->error = XML_TRUE;
        goto free;
      }

      context_ptr->port = ladish_room_add_port(context_ptr->room, uuid, name_dup, port_type, port_flags);
      if (context_ptr->port == NULL)
      {
        log_error("ladish_room_add_port() failed.");
        context_ptr->port = NULL;
      }

      goto free;
    }

    log_error("port element in wrong place");
    ladish_dump_element_stack(context_ptr);
    context_ptr->error = XML_TRUE;
    goto free;
  }

  if (strcmp(el, "connections") == 0)
  {
    //log_info("<connections>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_CONNECTIONS;
    goto free;
  }

  if (strcmp(el, "connection") == 0)
  {
    //log_info("<connection>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_CONNECTION;

    uuid_str = ladish_get_uuid_attribute(attr, "port1", uuid, false);
    if (uuid_str == NULL)
    {
      log_error("/studio/connections/connection \"port1\" attribute is not available.");
      context_ptr->error = XML_TRUE;
      goto free;
    }

    uuid2_str = ladish_get_uuid_attribute(attr, "port2", uuid2, false);
    if (uuid2_str == NULL)
    {
      log_error("/studio/connections/connection \"port2\" attribute is not available.");
      context_ptr->error = XML_TRUE;
      goto free;
    }

    log_info("studio connection between port %s and port %s", uuid_str, uuid2_str);

    port1 = ladish_graph_find_port_by_uuid(g_studio.studio_graph, uuid, true, NULL);
    if (port1 == NULL)
    {
      log_error("studio client with unknown port %s", uuid_str);
      context_ptr->error = XML_TRUE;
      goto free;
    }

    port2 = ladish_graph_find_port_by_uuid(g_studio.studio_graph, uuid2, true, NULL);
    if (port2 == NULL)
    {
      log_error("studio client with unknown port %s", uuid2_str);
      context_ptr->error = XML_TRUE;
      goto free;
    }

    context_ptr->connection_id = ladish_graph_add_connection(g_studio.studio_graph, port1, port2, true);
    if (context_ptr->connection_id == 0)
    {
      log_error("ladish_graph_add_connection() failed.");
      goto free;
    }

    goto free;
  }

  if (strcmp(el, "applications") == 0)
  {
    //log_info("<applications>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_APPLICATIONS;
    goto free;
  }

  if (strcmp(el, "application") == 0)
  {
    //log_info("<application>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_APPLICATION;

    name = ladish_get_string_attribute(attr, "name");
    if (name == NULL)
    {
      log_error("application \"name\" attribute is not available.");
      context_ptr->error = XML_TRUE;
      goto free;
    }

    if (!ladish_get_uuid_attribute(attr, "uuid", context_ptr->uuid, true))
    {
      uuid_clear(context_ptr->uuid);
    }

    if (ladish_get_bool_attribute(attr, "terminal", &context_ptr->terminal) == NULL)
    {
      log_error("application \"terminal\" attribute is not available. name=\"%s\"", name);
      context_ptr->error = XML_TRUE;
      goto free;
    }

    if (ladish_get_bool_attribute(attr, "autorun", &context_ptr->autorun) == NULL)
    {
      log_error("application \"autorun\" attribute is not available. name=\"%s\"", name);
      context_ptr->error = XML_TRUE;
      goto free;
    }

    level = ladish_get_string_attribute(attr, "level");
    if (level == NULL)
    {
      log_error("application \"level\" attribute is not available. name=\"%s\"", name);
      context_ptr->error = XML_TRUE;
      goto free;
    }

    if (!ladish_check_app_level_validity(level, &len))
    {
      log_error("application \"level\" attribute has invalid value \"%s\", name=\"%s\"", level, name);
      context_ptr->error = XML_TRUE;
      goto free;
    }

    memcpy(context_ptr->level, level, len + 1);

    context_ptr->str = strdup(name);
    if (context_ptr->str == NULL)
    {
      log_error("strdup() failed");
      context_ptr->error = XML_TRUE;
      goto free;
    }

    context_ptr->data_used = 0;
    goto free;
  }

  if (strcmp(el, "dict") == 0)
  {
    //log_info("<dict>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_DICT;

    if (context_ptr->dict != NULL)
    {
        log_error("nested dicts");
        context_ptr->error = XML_TRUE;
        goto free;
    }

    if (context_ptr->depth == 1 &&
        context_ptr->element[0] == PARSE_CONTEXT_STUDIO)
    {
      context_ptr->dict = ladish_graph_get_dict(g_studio.studio_graph);
      ASSERT(context_ptr->dict != NULL);
    }
    else if (context_ptr->depth > 0 &&
             context_ptr->element[context_ptr->depth - 1] == PARSE_CONTEXT_CLIENT)
    {
      ASSERT(context_ptr->client != NULL);
      context_ptr->dict = ladish_client_get_dict(context_ptr->client);
      ASSERT(context_ptr->dict != NULL);
    }
    else if (context_ptr->depth > 0 &&
             context_ptr->element[context_ptr->depth - 1] == PARSE_CONTEXT_PORT)
    {
      ASSERT(context_ptr->port != NULL);
      context_ptr->dict = ladish_port_get_dict(context_ptr->port);
      ASSERT(context_ptr->dict != NULL);
    }
    else if (context_ptr->depth > 0 &&
             context_ptr->element[context_ptr->depth - 1] == PARSE_CONTEXT_CONNECTION)
    {
      ASSERT(context_ptr->port != NULL);
      context_ptr->dict = ladish_graph_get_connection_dict(g_studio.studio_graph, context_ptr->connection_id);
      ASSERT(context_ptr->dict != NULL);
    }
    else
    {
      log_error("unexpected dict XML element");
      context_ptr->error = XML_TRUE;
      goto free;
    }

    goto free;
  }

  if (strcmp(el, "key") == 0)
  {
    //log_info("<key>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_KEY;

    if (context_ptr->dict == NULL)
    {
        log_error("dict-less key");
        context_ptr->error = XML_TRUE;
        goto free;
    }

    name = ladish_get_string_attribute(attr, "name");
    if (name == NULL)
    {
      log_error("dict/key \"name\" attribute is not available.");
      context_ptr->error = XML_TRUE;
      goto free;
    }

    context_ptr->str = strdup(name);
    if (context_ptr->str == NULL)
    {
      log_error("strdup() failed");
      context_ptr->error = XML_TRUE;
      goto free;
    }

    context_ptr->data_used = 0;

    goto free;
  }

  log_error("unknown element \"%s\"", el);
  context_ptr->error = XML_TRUE;

free:
  free(name_dup);
  return;
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

  //log_info("element end (depth = %d, element = %u)", context_ptr->depth, context_ptr->element[context_ptr->depth]);

  if (context_ptr->element[context_ptr->depth] == PARSE_CONTEXT_PARAMETER &&
      context_ptr->depth == 3 &&
      context_ptr->element[0] == PARSE_CONTEXT_STUDIO &&
      context_ptr->element[1] == PARSE_CONTEXT_JACK &&
      context_ptr->element[2] == PARSE_CONTEXT_CONF)
  {
    context_ptr->data[context_ptr->data_used] = 0;

    //log_info("'%s' with value '%s'", context_ptr->str, context_ptr->data);

    dst = address = strdup(context_ptr->str);
    src = context_ptr->str + 1;
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
      ASSERT(*src == '/' || *src == 0);
      if (sep != NULL)
      {
        ASSERT(*src == '/');
        src++;                  /* skip separator */
      }
      else
      {
        ASSERT(*src == 0);
      }
    }
    *dst = 0;                   /* ASCIZZ */

    if (!jack_proxy_get_parameter_value(address, &is_set, &parameter))
    {
      log_error("jack_proxy_get_parameter_value() failed");
      goto fail_free_address;
    }

    if (parameter.type == jack_string)
    {
      free(parameter.value.string);
    }

    switch (parameter.type)
    {
    case jack_boolean:
      log_info("%s value is %s (boolean)", context_ptr->str, context_ptr->data);
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
        log_error("bad value for a bool jack param");
        goto fail_free_address;
      }
      break;
    case jack_string:
      log_info("%s value is %s (string)", context_ptr->str, context_ptr->data);
      parameter.value.string = context_ptr->data;
      break;
    case jack_byte:
      log_debug("%s value is %u/%c (byte/char)", context_ptr->str, *context_ptr->data, *context_ptr->data);
      if (context_ptr->data[0] == 0 ||
          context_ptr->data[1] != 0)
      {
        log_error("bad value for a char jack param");
        goto fail_free_address;
      }
      parameter.value.byte = context_ptr->data[0];
      break;
    case jack_uint32:
      log_info("%s value is %s (uint32)", context_ptr->str, context_ptr->data);
      if (sscanf(context_ptr->data, "%" PRIu32, &parameter.value.uint32) != 1)
      {
        log_error("bad value for an uint32 jack param");
        goto fail_free_address;
      }
      break;
    case jack_int32:
      log_info("%s value is %s (int32)", context_ptr->str, context_ptr->data);
      if (sscanf(context_ptr->data, "%" PRIi32, &parameter.value.int32) != 1)
      {
        log_error("bad value for an int32 jack param");
        goto fail_free_address;
      }
      break;
    default:
      log_error("unknown jack parameter type %d of %s", (int)parameter.type, context_ptr->str);
      goto fail_free_address;
    }

    if (!jack_proxy_set_parameter_value(address, &parameter))
    {
      log_error("jack_proxy_set_parameter_value() failed");
      goto fail_free_address;
    }

    free(address);
  }
  else if (context_ptr->element[context_ptr->depth] == PARSE_CONTEXT_KEY &&
           context_ptr->depth > 0 &&
           context_ptr->element[context_ptr->depth - 1] == PARSE_CONTEXT_DICT)
  {
    ASSERT(context_ptr->dict != NULL);
    context_ptr->data[context_ptr->data_used] = 0;
    log_info("dict key '%s' with value '%s'", context_ptr->str, context_ptr->data);
    if (!ladish_dict_set(context_ptr->dict, context_ptr->str, context_ptr->data))
    {
      log_error("ladish_dict_set() failed");
      context_ptr->error = XML_TRUE;
      return;
    }
  }
  else if (context_ptr->element[context_ptr->depth] == PARSE_CONTEXT_ROOM)
  {
    //log_info("</room>");
    ASSERT(context_ptr->room != NULL);
    context_ptr->room = NULL;
  }
  else if (context_ptr->element[context_ptr->depth] == PARSE_CONTEXT_DICT)
  {
    //log_info("</dict>");
    ASSERT(context_ptr->dict != NULL);
    context_ptr->dict = NULL;
  }
  else if (context_ptr->element[context_ptr->depth] == PARSE_CONTEXT_CLIENT)
  {
    //log_info("</client>");
    ASSERT(context_ptr->client != NULL);
    context_ptr->client = NULL;
  }
  else if (context_ptr->element[context_ptr->depth] == PARSE_CONTEXT_PORT)
  {
    //log_info("</port>");
    ASSERT(context_ptr->port != NULL);
    context_ptr->port = NULL;
  }
  else if (context_ptr->element[context_ptr->depth] == PARSE_CONTEXT_APPLICATION)
  {
    context_ptr->data[unescape(context_ptr->data, context_ptr->data_used, context_ptr->data)] = 0;
    unescape_simple(context_ptr->str);

    log_info("application '%s' (%s, %s, level '%s') with commandline '%s'", context_ptr->str, context_ptr->terminal ? "terminal" : "shell", context_ptr->autorun ? "autorun" : "stopped", context_ptr->level, context_ptr->data);

    if (ladish_app_supervisor_add(
          g_studio.app_supervisor,
          context_ptr->str,
          context_ptr->uuid,
          context_ptr->autorun,
          context_ptr->data,
          context_ptr->terminal,
          context_ptr->level) == NULL)
    {
      log_error("ladish_app_supervisor_add() failed.");
      context_ptr->error = XML_TRUE;
    }
  }

  context_ptr->depth--;

  if (context_ptr->str != NULL)
  {
    free(context_ptr->str);
    context_ptr->str = NULL;
  }

  return;

fail_free_address:
  free(address);
  context_ptr->error = XML_TRUE;
  return;
}

#undef context_ptr

struct ladish_command_load_studio
{
  struct ladish_command command;
  char * studio_name;
};

#define cmd_ptr ((struct ladish_command_load_studio *)command_context)

static bool run(void * command_context)
{
  char * path;
  struct stat st;
  XML_Parser parser;
  int bytes_read;
  void * buffer;
  int fd;
  enum XML_Status xmls;
  struct ladish_parse_context parse_context;

  ASSERT(cmd_ptr->command.state == LADISH_COMMAND_STATE_PENDING);

  if (!ladish_studio_compose_filename(cmd_ptr->studio_name, &path, NULL))
  {
    log_error("failed to compose path of studio \%s\" file", cmd_ptr->studio_name);
    return false;
  }

  log_info("Loading studio... ('%s')", path);

  if (stat(path, &st) != 0)
  {
    log_error("failed to stat '%s': %d (%s)", path, errno, strerror(errno));
    free(path);
    return false;
  }

  g_studio.name = cmd_ptr->studio_name;
  cmd_ptr->studio_name = NULL;

  g_studio.filename = path;

  if (!jack_reset_all_params())
  {
    log_error("jack_reset_all_params() failed");
    return false;
  }

  fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    log_error("failed to open '%s': %d (%s)", path, errno, strerror(errno));
    return false;
  }

  parser = XML_ParserCreate(NULL);
  if (parser == NULL)
  {
    log_error("XML_ParserCreate() failed to create parser object.");
    close(fd);
    return false;
  }

  //log_info("conf file size is %llu bytes", (unsigned long long)st.st_size);

  /* we are expecting that conf file has small enough size to fit in memory */

  buffer = XML_GetBuffer(parser, st.st_size);
  if (buffer == NULL)
  {
    log_error("XML_GetBuffer() failed.");
    XML_ParserFree(parser);
    close(fd);
    return false;
  }

  bytes_read = read(fd, buffer, st.st_size);
  if (bytes_read != st.st_size)
  {
    log_error("read() returned unexpected result.");
    XML_ParserFree(parser);
    close(fd);
    return false;
  }

  parse_context.error = XML_FALSE;
  parse_context.depth = -1;
  parse_context.str = NULL;
  parse_context.client = NULL;
  parse_context.port = NULL;
  parse_context.dict = NULL;
  parse_context.room = NULL;

  XML_SetElementHandler(parser, callback_elstart, callback_elend);
  XML_SetCharacterDataHandler(parser, callback_chrdata);
  XML_SetUserData(parser, &parse_context);

  if (!ladish_studio_show())
  {
    log_error("ladish_studio_show() failed.");
    XML_ParserFree(parser);
    close(fd);
    return false;
  }

  xmls = XML_ParseBuffer(parser, bytes_read, XML_TRUE);
  if (xmls == XML_STATUS_ERROR)
  {
    if (!parse_context.error)
    {
      log_error("XML_ParseBuffer() failed.");
    }

    ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "Studio load failed", LADISH_CHECK_LOG_TEXT);
    ladish_studio_clear();
    XML_ParserFree(parser);
    close(fd);
    return false;
  }

  XML_ParserFree(parser);
  close(fd);

  if (parse_context.error)
  {
    ladish_studio_clear();
    ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "Studio load failed", LADISH_CHECK_LOG_TEXT);
    return false;
  }

  ladish_interlink(ladish_studio_get_studio_graph(), ladish_studio_get_studio_app_supervisor());

  g_studio.persisted = true;
  log_info("Studio loaded. ('%s')", path);

  ladish_graph_dump(g_studio.jack_graph);
  ladish_graph_dump(g_studio.studio_graph);
  ladish_app_supervisor_dump(g_studio.app_supervisor);

  ladish_recent_store_use_item(g_studios_recent_store, g_studio.name);

  if (!ladish_app_supervisor_set_project_name(ladish_studio_get_studio_app_supervisor(), g_studio.name))
  {
    ladish_app_supervisor_set_project_name(ladish_studio_get_studio_app_supervisor(), NULL);
  }

  ladish_studio_announce();

  cmd_ptr->command.state = LADISH_COMMAND_STATE_DONE;
  return true;
}

static void destructor(void * command_context)
{
  log_info("load studio command destructor");
  if (cmd_ptr->studio_name != NULL)
  {
    free(cmd_ptr->studio_name);
  }
}

#undef cmd_ptr

bool ladish_command_load_studio(void * call_ptr, struct ladish_cqueue * queue_ptr, const char * studio_name, bool autostart)
{
  struct ladish_command_load_studio * cmd_ptr;
  char * studio_name_dup;

  studio_name_dup = strdup(studio_name);
  if (studio_name_dup == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup('%s') failed.", studio_name);
    goto fail;
  }

  if (!ladish_command_unload_studio(call_ptr, queue_ptr))
  {
    goto fail_free_name;
  }

  cmd_ptr = ladish_command_new(sizeof(struct ladish_command_load_studio));
  if (cmd_ptr == NULL)
  {
    log_error("ladish_command_new() failed.");
    goto fail_drop_unload_command;
  }

  cmd_ptr->command.run = run;
  cmd_ptr->command.destructor = destructor;
  cmd_ptr->studio_name = studio_name_dup;

  if (!ladish_cqueue_add_command(queue_ptr, &cmd_ptr->command))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "ladish_cqueue_add_command() failed.");
    goto fail_destroy_command;
  }

  if (autostart)
  {
    if (!ladish_command_start_studio(call_ptr, queue_ptr))
    {
      goto fail_drop_load_command;
    }
  }

  return true;

fail_drop_load_command:
  ladish_cqueue_drop_command(queue_ptr);

fail_destroy_command:
  free(cmd_ptr);

fail_drop_unload_command:
  ladish_cqueue_drop_command(queue_ptr);

fail_free_name:
  free(studio_name_dup);

fail:
  return false;
}
