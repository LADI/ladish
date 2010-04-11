/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
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
#include <expat.h>
#include <limits.h>

#include "escape.h"
#include "cmd.h"
#include "studio_internal.h"

#define PARSE_CONTEXT_ROOT                0
#define PARSE_CONTEXT_STUDIO              1
#define PARSE_CONTEXT_JACK                2
#define PARSE_CONTEXT_CONF                3
#define PARSE_CONTEXT_PARAMETER           4
#define PARSE_CONTEXT_CLIENTS             5
#define PARSE_CONTEXT_CLIENT              6
#define PARSE_CONTEXT_PORTS               7
#define PARSE_CONTEXT_PORT                8
#define PARSE_CONTEXT_DICT                9
#define PARSE_CONTEXT_KEY                10
#define PARSE_CONTEXT_CONNECTIONS        11
#define PARSE_CONTEXT_CONNECTION         12
#define PARSE_CONTEXT_APPLICATIONS       13
#define PARSE_CONTEXT_APPLICATION        14

#define MAX_STACK_DEPTH       10
#define MAX_DATA_SIZE         10240

struct parse_context
{
  XML_Bool error;
  unsigned int element[MAX_STACK_DEPTH];
  signed int depth;
  char data[MAX_DATA_SIZE];
  int data_used;
  char * str;
  ladish_client_handle client;
  ladish_port_handle port;
  ladish_dict_handle dict;
  uint64_t connection_id;
  bool terminal;
  bool autorun;
  uint8_t level;
};

#define context_ptr ((struct parse_context *)data)

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
  uuid_t uuid;
  uuid_t uuid2;
  ladish_port_handle port1;
  ladish_port_handle port2;
  long int li_value;
  char * end_ptr;

  if (context_ptr->error)
  {
    return;
  }

  if (context_ptr->depth + 1 >= MAX_STACK_DEPTH)
  {
    log_error("xml parse max stack depth reached");
    context_ptr->error = XML_TRUE;
    return;
  }

  if (strcmp(el, "studio") == 0)
  {
    //log_info("<studio>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_STUDIO;
    return;
  }

  if (strcmp(el, "jack") == 0)
  {
    //log_info("<jack>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_JACK;
    return;
  }

  if (strcmp(el, "conf") == 0)
  {
    //log_info("<conf>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_CONF;
    return;
  }

  if (strcmp(el, "parameter") == 0)
  {
    //log_info("<parameter>");
    if ((attr[0] == NULL || attr[2] != NULL) || strcmp(attr[0], "path") != 0)
    {
      log_error("<parameter> XML element must contain exactly one attribute, named \"path\"");
      context_ptr->error = XML_TRUE;
      return;
    }

    context_ptr->str = strdup(attr[1]);
    if (context_ptr->str == NULL)
    {
      log_error("strdup() failed");
      context_ptr->error = XML_TRUE;
      return;
    }

    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_PARAMETER;
    context_ptr->data_used = 0;
    return;
  }

  if (strcmp(el, "clients") == 0)
  {
    //log_info("<clients>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_CLIENTS;
    return;
  }

  if (strcmp(el, "client") == 0)
  {
    //log_info("<client>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_CLIENT;

    if (context_ptr->client != NULL)
    {
        log_error("nested clients");
        context_ptr->error = XML_TRUE;
        return;
    }

    if (context_ptr->depth == 3 &&
        context_ptr->element[0] == PARSE_CONTEXT_STUDIO &&
        context_ptr->element[1] == PARSE_CONTEXT_JACK &&
        context_ptr->element[2] == PARSE_CONTEXT_CLIENTS)
    {
      if (attr[0] == NULL ||
          attr[1] == NULL ||
          attr[2] == NULL ||
          attr[3] == NULL ||
          attr[4] != NULL ||
          strcmp(attr[0], "name") != 0 ||
          strcmp(attr[2], "uuid") != 0)
      {
        log_error("studio/jack/clients/client XML element must contain exactly two attributes, named \"name\" and \"uuid\", in this order");
        context_ptr->error = XML_TRUE;
        return;
      }

      if (uuid_parse(attr[3], uuid) != 0)
      {
        log_error("cannot parse uuid \"%s\"", attr[3]);
        context_ptr->error = XML_TRUE;
        return;
      }

      log_info("jack client \"%s\" with uuid %s", attr[1], attr[3]);

      if (!ladish_client_create(uuid, &context_ptr->client))
      {
        log_error("ladish_client_create() failed.");
        context_ptr->error = XML_TRUE;
        ASSERT(context_ptr->client == NULL);
        return;
      }

      if (!ladish_graph_add_client(g_studio.jack_graph, context_ptr->client, attr[1], true))
      {
        log_error("ladish_graph_add_client() failed to add client '%s' to JACK graph", attr[1]);
        context_ptr->error = XML_TRUE;
        ladish_client_destroy(context_ptr->client);
        context_ptr->client = NULL;
        return;
      }
    }
    else if (context_ptr->depth == 2 &&
             context_ptr->element[0] == PARSE_CONTEXT_STUDIO &&
             context_ptr->element[1] == PARSE_CONTEXT_CLIENTS)
    {
      if (attr[0] == NULL ||
          attr[1] == NULL ||
          attr[2] == NULL ||
          attr[3] == NULL ||
          attr[4] != NULL ||
          strcmp(attr[0], "name") != 0 ||
          strcmp(attr[2], "uuid") != 0)
      {
        log_error("studio/clients/client XML element must contain exactly two attributes, named \"name\" and \"uuid\", in this order");
        context_ptr->error = XML_TRUE;
        return;
      }

      if (uuid_parse(attr[3], uuid) != 0)
      {
        log_error("cannot parse uuid \"%s\"", attr[3]);
        context_ptr->error = XML_TRUE;
        return;
      }

      log_info("studio client \"%s\" with uuid %s", attr[1], attr[3]);

      if (!ladish_client_create(uuid, &context_ptr->client))
      {
        log_error("ladish_client_create() failed.");
        context_ptr->error = XML_TRUE;
        ASSERT(context_ptr->client == NULL);
        return;
      }

      if (!ladish_graph_add_client(g_studio.studio_graph, context_ptr->client, attr[1], true))
      {
        log_error("ladish_graph_add_client() failed to add client '%s' to studio graph", attr[1]);
        context_ptr->error = XML_TRUE;
        ladish_client_destroy(context_ptr->client);
        context_ptr->client = NULL;
        return;
      }
    }

    return;
  }

  if (strcmp(el, "ports") == 0)
  {
    //log_info("<ports>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_PORTS;
    return;
  }

  if (strcmp(el, "port") == 0)
  {
    //log_info("<port>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_PORT;

    if (context_ptr->port != NULL)
    {
        log_error("nested ports");
        context_ptr->error = XML_TRUE;
        return;
    }

    if (context_ptr->client == NULL)
    {
        log_error("client-less port");
        context_ptr->error = XML_TRUE;
        return;
    }

    if (context_ptr->depth == 5 &&
        context_ptr->element[0] == PARSE_CONTEXT_STUDIO &&
        context_ptr->element[1] == PARSE_CONTEXT_JACK &&
        context_ptr->element[2] == PARSE_CONTEXT_CLIENTS &&
        context_ptr->element[3] == PARSE_CONTEXT_CLIENT &&
        context_ptr->element[4] == PARSE_CONTEXT_PORTS)
    {
      if (attr[0] == NULL ||
          attr[1] == NULL ||
          attr[2] == NULL ||
          attr[3] == NULL ||
          attr[4] != NULL ||
          strcmp(attr[0], "name") != 0 ||
          strcmp(attr[2], "uuid") != 0)
      {
        log_error("studio/jack/clients/client/ports/port XML element must contain exactly two attributes, named \"name\" and \"uuid\", in this order");
        context_ptr->error = XML_TRUE;
        return;
      }

      if (uuid_parse(attr[3], uuid) != 0)
      {
        log_error("cannot parse uuid \"%s\"", attr[3]);
        context_ptr->error = XML_TRUE;
        return;
      }

      log_info("jack port \"%s\" with uuid %s", attr[1], attr[3]);

      if (!ladish_port_create(uuid, false, &context_ptr->port))
      {
        log_error("ladish_port_create() failed.");
        return;
      }

      if (!ladish_graph_add_port(g_studio.jack_graph, context_ptr->client, context_ptr->port, attr[1], 0, 0, true))
      {
        log_error("ladish_graph_add_port() failed.");
        ladish_port_destroy(context_ptr->port);
        context_ptr->port = NULL;
        return;
      }
    }
    else if (context_ptr->depth == 4 &&
             context_ptr->element[0] == PARSE_CONTEXT_STUDIO &&
             context_ptr->element[1] == PARSE_CONTEXT_CLIENTS &&
             context_ptr->element[2] == PARSE_CONTEXT_CLIENT &&
             context_ptr->element[3] == PARSE_CONTEXT_PORTS)
    {
      if (attr[0] == NULL ||
          attr[1] == NULL ||
          attr[2] == NULL ||
          attr[3] == NULL ||
          attr[4] != NULL ||
          strcmp(attr[0], "name") != 0 ||
          strcmp(attr[2], "uuid") != 0)
      {
        log_error("studio/clients/client/ports/port XML element must contain exactly two attributes, named \"name\" and \"uuid\", in this order");
        context_ptr->error = XML_TRUE;
        return;
      }

      if (uuid_parse(attr[3], uuid) != 0)
      {
        log_error("cannot parse uuid \"%s\"", attr[3]);
        context_ptr->error = XML_TRUE;
        return;
      }

      log_info("studio port \"%s\" with uuid %s", attr[1], attr[3]);

      context_ptr->port = ladish_graph_find_port_by_uuid(g_studio.jack_graph, uuid);
      if (context_ptr->port == NULL)
      {
        log_error("studio client with non-jack port %s", attr[3]);
        context_ptr->error = XML_TRUE;
        return;
      }

      if (!ladish_graph_add_port(g_studio.studio_graph, context_ptr->client, context_ptr->port, attr[1], 0, 0, true))
      {
        log_error("ladish_graph_add_port() failed.");
        ladish_port_destroy(context_ptr->port);
        context_ptr->port = NULL;
        return;
      }
    }

    return;
  }

  if (strcmp(el, "connections") == 0)
  {
    //log_info("<connections>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_CONNECTIONS;
    return;
  }

  if (strcmp(el, "connection") == 0)
  {
    //log_info("<connection>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_CONNECTION;

    if (attr[0] == NULL ||
        attr[1] == NULL ||
        attr[2] == NULL ||
        attr[3] == NULL ||
        attr[4] != NULL ||
        strcmp(attr[0], "port1") != 0 ||
        strcmp(attr[2], "port2") != 0)
    {
      log_error("studio/connections/connection XML element must contain exactly two attributes, named \"port1\" and \"port2\", in this order");
      context_ptr->error = XML_TRUE;
      return;
    }

    if (uuid_parse(attr[1], uuid) != 0)
    {
      log_error("cannot parse uuid \"%s\"", attr[1]);
      context_ptr->error = XML_TRUE;
      return;
    }

    if (uuid_parse(attr[3], uuid2) != 0)
    {
      log_error("cannot parse uuid \"%s\"", attr[3]);
      context_ptr->error = XML_TRUE;
      return;
    }

    log_info("studio connection between port %s and port %s", attr[1], attr[3]);

    port1 = ladish_graph_find_port_by_uuid(g_studio.studio_graph, uuid);
    if (port1 == NULL)
    {
      log_error("studio client with unknown port %s", attr[1]);
      context_ptr->error = XML_TRUE;
      return;
    }

    port2 = ladish_graph_find_port_by_uuid(g_studio.studio_graph, uuid2);
    if (port2 == NULL)
    {
      log_error("studio client with unknown port %s", attr[3]);
      context_ptr->error = XML_TRUE;
      return;
    }

    context_ptr->connection_id = ladish_graph_add_connection(g_studio.studio_graph, port1, port2, true);
    if (context_ptr->connection_id == 0)
    {
      log_error("ladish_graph_add_connection() failed.");
      return;
    }

    return;
  }

  if (strcmp(el, "applications") == 0)
  {
    //log_info("<applications>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_APPLICATIONS;
    return;
  }

  if (strcmp(el, "application") == 0)
  {
    //log_info("<application>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_APPLICATION;

    if (attr[0] == NULL ||
        attr[1] == NULL ||
        attr[2] == NULL ||
        attr[3] == NULL ||
        attr[4] == NULL ||
        attr[5] == NULL ||
        attr[6] == NULL ||
        attr[7] == NULL ||
        attr[8] != NULL ||
        strcmp(attr[0], "name") != 0 ||
        strcmp(attr[2], "terminal") != 0 ||
        strcmp(attr[4], "level") != 0 ||
        strcmp(attr[6], "autorun") != 0)
    {
      log_error("'application' XML element must contain exactly four attributes, named \"name\", \"terminal\", \"level\" and \"autorun\", in this order");
      context_ptr->error = XML_TRUE;
      return;
    }

    if (strcmp(attr[3], "true") != 0 && strcmp(attr[3], "false") != 0)
    {
      log_error("'application@terminal' XML attribute is boolean and the only valid values are \"true\" and \"false\"");
      context_ptr->error = XML_TRUE;
      return;
    }

    if (strcmp(attr[7], "true") != 0 && strcmp(attr[7], "false") != 0)
    {
      log_error("'application@autorun' XML attribute is boolean and the only valid values are \"true\" and \"false\"");
      context_ptr->error = XML_TRUE;
      return;
    }

    errno = 0;    /* To distinguish success/failure after call */
    li_value = strtol(attr[5], &end_ptr, 10);
    if ((errno == ERANGE && (li_value == LONG_MAX || li_value == LONG_MIN)) || (errno != 0 && li_value == 0) || end_ptr == attr[5])
    {
      log_error("'application@level' XML attribute '%s' is not valid integer.", attr[5]);
      context_ptr->error = XML_TRUE;
      return;
    }

    if (li_value < 0 || li_value > 255)
    {
      log_error("'application@level' XML attribute '%s' is not valid uint8.", attr[5]);
      context_ptr->error = XML_TRUE;
      return;
    }

    context_ptr->str = strdup(attr[1]);
    if (context_ptr->str == NULL)
    {
      log_error("strdup() failed");
      context_ptr->error = XML_TRUE;
      return;
    }

    context_ptr->terminal = strcmp(attr[3], "true") == 0;
    context_ptr->autorun = strcmp(attr[7], "true") == 0;
    context_ptr->level = (uint8_t)li_value;

    context_ptr->data_used = 0;
    return;
  }

  if (strcmp(el, "dict") == 0)
  {
    //log_info("<dict>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_DICT;

    if (context_ptr->dict != NULL)
    {
        log_error("nested dicts");
        context_ptr->error = XML_TRUE;
        return;
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
      return;
    }

    return;
  }

  if (strcmp(el, "key") == 0)
  {
    //log_info("<key>");
    context_ptr->element[++context_ptr->depth] = PARSE_CONTEXT_KEY;

    if (context_ptr->dict == NULL)
    {
        log_error("dict-less key");
        context_ptr->error = XML_TRUE;
        return;
    }

    if (attr[0] == NULL ||
        attr[1] == NULL ||
        attr[2] != NULL ||
        strcmp(attr[0], "name") != 0)
    {
      log_error("dict/key XML element must contain exactly one attributes, named \"name\".");
      context_ptr->error = XML_TRUE;
      return;
    }

    context_ptr->str = strdup(attr[1]);
    if (context_ptr->str == NULL)
    {
      log_error("strdup() failed");
      context_ptr->error = XML_TRUE;
      return;
    }

    context_ptr->data_used = 0;

    return;
  }

  log_error("unknown element \"%s\"", el);
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
    unescape(context_ptr->str, strlen(context_ptr->str) + 1, context_ptr->str);

    log_info("application '%s' (%s, %s, level %u) with commandline '%s'", context_ptr->str, context_ptr->terminal ? "terminal" : "shell", context_ptr->autorun ? "autorun" : "stopped", (unsigned int)context_ptr->level, context_ptr->data);

    if (!ladish_app_supervisor_add(g_studio.app_supervisor, context_ptr->str, context_ptr->autorun, context_ptr->data, context_ptr->terminal, context_ptr->level))
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
  struct parse_context parse_context;

  ASSERT(cmd_ptr->command.state == LADISH_COMMAND_STATE_PENDING);

  if (!studio_compose_filename(cmd_ptr->studio_name, &path, NULL))
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

  XML_SetElementHandler(parser, callback_elstart, callback_elend);
  XML_SetCharacterDataHandler(parser, callback_chrdata);
  XML_SetUserData(parser, &parse_context);

  xmls = XML_ParseBuffer(parser, bytes_read, XML_TRUE);
  if (xmls == XML_STATUS_ERROR)
  {
    if (!parse_context.error)
    {
      log_error("XML_ParseBuffer() failed.");
    }
    XML_ParserFree(parser);
    close(fd);
    return false;
  }

  XML_ParserFree(parser);
  close(fd);

  if (parse_context.error)
  {
    return false;
  }

  g_studio.persisted = true;
  log_info("Studio loaded. ('%s')", path);

  ladish_graph_dump(g_studio.jack_graph);
  ladish_graph_dump(g_studio.studio_graph);

  if (!studio_publish())
  {
    log_error("studio_publish() failed.");
    return false;
  }

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

bool ladish_command_load_studio(void * call_ptr, struct ladish_cqueue * queue_ptr, const char * studio_name)
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

  if (!ladish_command_start_studio(call_ptr, queue_ptr))
  {
    goto fail_drop_load_command;
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
