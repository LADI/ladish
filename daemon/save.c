/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation save releated helper functions
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

#include <unistd.h>

#include "save.h"
#include "escape.h"
#include "studio.h"

struct ladish_write_vgraph_context
{
  int fd;
  int indent;
  ladish_app_supervisor_handle app_supervisor;
  bool client_visible;
};

static bool is_system_client(ladish_client_handle client)
{
  uuid_t uuid;
  ladish_client_get_uuid(client, uuid);
  return ladish_virtualizer_is_system_client(uuid);
}

static bool is_hidden_port_interesting(ladish_app_supervisor_handle app_supervisor, ladish_client_handle client, ladish_port_handle port)
{
  uuid_t app_uuid;
  ladish_app_handle app;

  if (is_system_client(client) || ladish_port_is_link(port))
  {
    return true;
  }

  /* hidden ports of external apps should not be saved */
  /* hidden ports of stopped managed apps should be saved */
  /* hidden ports of started managed apps should not be saved */

  if (!ladish_port_get_app(port, app_uuid))
  {
    /* port of external app, don't save */
    return false;
  }

  app = ladish_app_supervisor_find_app_by_uuid(app_supervisor, app_uuid);
  if (app == NULL)
  {
    ASSERT_NO_PASS;             /* this should not happen because app ports are removed before app is removed */
    return false;
  }

  return !ladish_app_is_running(app);
}

bool ladish_write_string(int fd, const char * string)
{
  size_t len;
  ssize_t ret;

  len = strlen(string);

  ret = write(fd, string, len);
  if (ret == -1)
  {
    log_error("write(%d, \"%s\", %zu) failed to write file: %d (%s)", fd, string, len, errno, strerror(errno));
    return false;
  }
  if ((size_t)ret != len)
  {
    log_error("write() wrote wrong byte count to file (%zd != %zu).", ret, len);
    return false;
  }

  return true;
}

bool ladish_write_indented_string(int fd, int indent, const char * string)
{
  ASSERT(indent >= 0);
  while (indent--)
  {
    if (!ladish_write_string(fd, LADISH_XML_BASE_INDENT))
    {
      return false;
    }
  }

  if (!ladish_write_string(fd, string))
  {
    return false;
  }

  return true;
}

bool ladish_write_string_escape_ex(int fd, const char * string, unsigned int flags)
{
  bool ret;
  char * escaped_buffer;

  escaped_buffer = malloc(max_escaped_length(strlen(string)));
  if (escaped_buffer == NULL)
  {
    log_error("malloc() failed to allocate buffer for escaped string");
    return false;
  }

  escape_simple(string, escaped_buffer, flags);

  ret = ladish_write_string(fd, escaped_buffer);

  free(escaped_buffer);

  return ret;
}

bool ladish_write_string_escape(int fd, const char * string)
{
  return ladish_write_string_escape_ex(fd, string, LADISH_ESCAPE_FLAG_ALL);
}

#define fd (((struct ladish_write_context *)context)->fd)
#define indent (((struct ladish_write_context *)context)->indent)

static
bool
write_dict_entry(
  void * context,
  const char * key,
  const char * value)
{
  if (!ladish_write_indented_string(fd, indent, "<key name=\""))
  {
    return false;
  }

  if (!ladish_write_string(fd, key))
  {
    return false;
  }

  if (!ladish_write_string(fd, "\">"))
  {
    return false;
  }

  if (!ladish_write_string(fd, value))
  {
    return false;
  }

  if (!ladish_write_string(fd, "</key>\n"))
  {
    return false;
  }

  return true;
}

static
bool
ladish_write_room_port(
  void * context,
  ladish_port_handle port,
  const char * name,
  uint32_t type,
  uint32_t flags)
{
  uuid_t uuid;
  char str[37];
  bool midi;
  const char * type_str;
  bool playback;
  const char * direction_str;
  ladish_dict_handle dict;

  ladish_port_get_uuid(port, uuid);
  uuid_unparse(uuid, str);

  playback = (flags & JACKDBUS_PORT_FLAG_INPUT) != 0;
  ASSERT(playback || (flags & JACKDBUS_PORT_FLAG_OUTPUT) != 0); /* playback or capture */
  ASSERT(!(playback && (flags & JACKDBUS_PORT_FLAG_OUTPUT) != 0)); /* but not both */
  direction_str = playback ? "playback" : "capture";

  midi = type == JACKDBUS_PORT_TYPE_MIDI;
  ASSERT(midi || type == JACKDBUS_PORT_TYPE_AUDIO); /* midi or audio */
  ASSERT(!(midi && type == JACKDBUS_PORT_TYPE_AUDIO)); /* but not both */
  type_str = midi ? "midi" : "audio";

  log_info("saving room %s %s port '%s' (%s)", direction_str, type_str, name, str);

  if (!ladish_write_indented_string(fd, indent, "<port name=\""))
  {
    return false;
  }

  if (!ladish_write_string_escape_ex(fd, name, LADISH_ESCAPE_FLAG_XML_ATTR))
  {
    return false;
  }

  if (!ladish_write_string(fd, "\" uuid=\""))
  {
    return false;
  }

  if (!ladish_write_string(fd, str))
  {
    return false;
  }

  if (!ladish_write_string(fd, "\" type=\""))
  {
    return false;
  }

  if (!ladish_write_string(fd, type_str))
  {
    return false;
  }

  if (!ladish_write_string(fd, "\" direction=\""))
  {
    return false;
  }

  if (!ladish_write_string(fd, direction_str))
  {
    return false;
  }

  dict = ladish_port_get_dict(port);
  if (ladish_dict_is_empty(dict))
  {
    if (!ladish_write_string(fd, "\" />\n"))
    {
      return false;
    }
  }
  else
  {
    if (!ladish_write_string(fd, "\">\n"))
    {
      return false;
    }

    if (!ladish_write_dict(fd, indent + 1, dict))
    {
      return false;
    }

    if (!ladish_write_indented_string(fd, indent, "</port>\n"))
    {
      return false;
    }
  }

  return true;
}

#undef indent
#undef fd

bool ladish_write_dict(int fd, int indent, ladish_dict_handle dict)
{
  struct ladish_write_context context;

  if (ladish_dict_is_empty(dict))
  {
    return true;
  }

  context.fd = fd;
  context.indent = indent + 1;

  if (!ladish_write_indented_string(fd, indent, "<dict>\n"))
  {
    return false;
  }

  if (!ladish_dict_iterate(dict, &context, write_dict_entry))
  {
    return false;
  }

  if (!ladish_write_indented_string(fd, indent, "</dict>\n"))
  {
    return false;
  }

  return true;
}

bool ladish_write_room_link_ports(int fd, int indent, ladish_room_handle room)
{
  struct ladish_write_context context;

  ladish_check_integrity();

  context.fd = fd;
  context.indent = indent;

  if (!ladish_room_iterate_link_ports(room, &context, ladish_write_room_port))
  {
    log_error("ladish_room_iterate_link_ports() failed");
    return false;
  }

  return true;
}

/****************/
/* write vgraph */
/****************/

#define fd (((struct ladish_write_vgraph_context *)context)->fd)
#define indent (((struct ladish_write_vgraph_context *)context)->indent)
#define ctx_ptr ((struct ladish_write_vgraph_context *)context)

static
bool
ladish_save_vgraph_client_begin(
  void * context,
  ladish_graph_handle graph,
  bool hidden,
  ladish_client_handle client_handle,
  const char * client_name,
  void ** client_iteration_context_ptr_ptr)
{
  uuid_t uuid;
  char str[37];

  ctx_ptr->client_visible = !hidden || ladish_client_has_app(client_handle);
  if (!ctx_ptr->client_visible)
  {
    return true;
  }

  ladish_client_get_uuid(client_handle, uuid);
  uuid_unparse(uuid, str);

  log_info("saving vgraph client '%s' (%s)", client_name, str);

  if (!ladish_write_indented_string(fd, indent, "<client name=\""))
  {
    return false;
  }

  if (!ladish_write_string_escape_ex(fd, client_name, LADISH_ESCAPE_FLAG_XML_ATTR))
  {
    return false;
  }

  if (!ladish_write_string(fd, "\" uuid=\""))
  {
    return false;
  }

  if (!ladish_write_string(fd, str))
  {
    return false;
  }

  if (!ladish_write_string(fd, "\" naming=\""))
  {
    return false;
  }

  if (!ladish_write_string(fd, "app"))
  {
    return false;
  }

  if (!ladish_write_string(fd, "\">\n"))
  {
    return false;
  }

  if (!ladish_write_indented_string(fd, indent + 1, "<ports>\n"))
  {
    return false;
  }

  return true;
}

static
bool
ladish_save_vgraph_client_end(
  void * context,
  ladish_graph_handle graph,
  bool hidden,
  ladish_client_handle client_handle,
  const char * client_name,
  void * client_iteration_context_ptr)
{
  if (!ctx_ptr->client_visible)
  {
    return true;
  }

  if (!ladish_write_indented_string(fd, indent + 1, "</ports>\n"))
  {
    return false;
  }

  if (!ladish_write_dict(fd, indent + 1, ladish_client_get_dict(client_handle)))
  {
    return false;
  }

  if (!ladish_write_indented_string(fd, indent, "</client>\n"))
  {
    return false;
  }

  return true;
}

static bool ladish_get_vgraph_port_uuids(ladish_graph_handle vgraph, ladish_port_handle port, uuid_t uuid, uuid_t link_uuid)
{
  bool link;

  if (vgraph != ladish_studio_get_studio_graph())
  {
    link = false;               /* room ports are saved using their fixed uuids */
  }
  else
  {
    link = ladish_port_is_link(port);
    if (link)
    {
      /* get the generated port uuid that is used for identification in the virtual graph */
      ladish_graph_get_port_uuid(vgraph, port, uuid);
    }
  }

  if (!link || link_uuid != NULL)
  {
    /* get the real port uuid that is same in both room and studio graphs */
    ladish_port_get_uuid(port, link ? link_uuid : uuid);
  }

  return link;
}

static
bool
ladish_save_vgraph_port(
  void * context,
  ladish_graph_handle graph,
  bool hidden,
  void * client_iteration_context_ptr,
  ladish_client_handle client_handle,
  const char * client_name,
  ladish_port_handle port_handle,
  const char * port_name,
  uint32_t port_type,
  uint32_t port_flags)
{
  uuid_t uuid;
  bool link;
  uuid_t link_uuid;
  char str[37];
  char link_str[37];
  ladish_dict_handle dict;

  if (!ctx_ptr->client_visible)
  {
    return true;
  }

  /* skip hidden ports of running apps */
  if (hidden && !is_hidden_port_interesting(ctx_ptr->app_supervisor, client_handle, port_handle))
  {
    return true;
  }

  link = ladish_get_vgraph_port_uuids(graph, port_handle, uuid, link_uuid);
  uuid_unparse(uuid, str);
  if (link)
  {
    uuid_unparse(link_uuid, link_str);
    log_info("saving vgraph link port '%s':'%s' (%s link=%s)", client_name, port_name, str, link_str);
  }
  else
  {
    log_info("saving vgraph port '%s':'%s' (%s)", client_name, port_name, str);
  }

  if (!ladish_write_indented_string(fd, indent + 2, "<port name=\""))
  {
    return false;
  }

  if (!ladish_write_string_escape_ex(fd, port_name, LADISH_ESCAPE_FLAG_XML_ATTR))
  {
    return false;
  }

  if (!ladish_write_string(fd, "\" uuid=\""))
  {
    return false;
  }

  if (!ladish_write_string(fd, str))
  {
    return false;
  }

  if (!ladish_write_string(fd, "\" type=\""))
  {
    return false;
  }

  if (!ladish_write_string(fd, port_type == JACKDBUS_PORT_TYPE_AUDIO ? "audio" : "midi"))
  {
    return false;
  }

  if (!ladish_write_string(fd, "\" direction=\""))
  {
    return false;
  }

  if (!ladish_write_string(fd, JACKDBUS_PORT_IS_INPUT(port_flags) ? "input" : "output"))
  {
    return false;
  }

  if (link)
  {
    if (!ladish_write_string(fd, "\" link_uuid=\""))
    {
      return false;
    }

    if (!ladish_write_string(fd, link_str))
    {
      return false;
    }
  }

  dict = ladish_port_get_dict(port_handle);
  if (ladish_dict_is_empty(dict))
  {
    if (!ladish_write_string(fd, "\" />\n"))
    {
      return false;
    }
  }
  else
  {
    if (!ladish_write_string(fd, "\">\n"))
    {
      return false;
    }

    if (!ladish_write_dict(fd, indent + 3, dict))
    {
      return false;
    }

    if (!ladish_write_indented_string(fd, indent + 2, "</port>\n"))
    {
      return false;
    }
  }

  return true;
}

static
bool
ladish_save_vgraph_connection(
  void * context,
  ladish_graph_handle graph,
  bool hidden,
  ladish_client_handle client1,
  ladish_port_handle port1,
  ladish_client_handle client2,
  ladish_port_handle port2,
  ladish_dict_handle dict)
{
  uuid_t uuid;
  char str[37];

  if (hidden &&
      (!is_hidden_port_interesting(ctx_ptr->app_supervisor, client1, port1) ||
       !is_hidden_port_interesting(ctx_ptr->app_supervisor, client2, port2)))
  {
    return true;
  }

  log_info("saving vgraph connection");

  if (!ladish_write_indented_string(fd, indent, "<connection port1=\""))
  {
    return false;
  }

  ladish_get_vgraph_port_uuids(graph, port1, uuid, NULL);
  uuid_unparse(uuid, str);

  if (!ladish_write_string(fd, str))
  {
    return false;
  }

  if (!ladish_write_string(fd, "\" port2=\""))
  {
    return false;
  }

  ladish_get_vgraph_port_uuids(graph, port2, uuid, NULL);
  uuid_unparse(uuid, str);

  if (!ladish_write_string(fd, str))
  {
    return false;
  }

  if (ladish_dict_is_empty(dict))
  {
    if (!ladish_write_string(fd, "\" />\n"))
    {
      return false;
    }
  }
  else
  {
    if (!ladish_write_string(fd, "\">\n"))
    {
      return false;
    }

    if (!ladish_write_dict(fd, indent + 1, dict))
    {
      return false;
    }

    if (!ladish_write_indented_string(fd, indent, "</connection>\n"))
    {
      return false;
    }
  }

  return true;
}

static
bool
ladish_save_app(
  void * context,
  const char * name,
  bool running,
  const char * command,
  bool terminal,
  uint8_t level,
  pid_t pid,
  const uuid_t uuid)
{
  char buf[100];
  const char * unescaped_string;
  char * escaped_string;
  char * escaped_buffer;
  bool ret;

  log_info("saving app: name='%s', %srunning, %s, level %u, commandline='%s'", name, running ? "" : "not ", terminal ? "terminal" : "shell", (unsigned int)level, command);

  ret = false;

  escaped_buffer = malloc(max_escaped_length(ladish_max(strlen(name), strlen(command))) + 1);
  if (escaped_buffer == NULL)
  {
    log_error("malloc() failed.");
    goto exit;
  }

  if (!ladish_write_indented_string(fd, indent, "<application name=\""))
  {
    goto free_buffer;
  }

  unescaped_string = name;
  escaped_string = escaped_buffer;
  escape(&unescaped_string, &escaped_string, LADISH_ESCAPE_FLAG_ALL);
  *escaped_string = 0;
  if (!ladish_write_string(fd, escaped_buffer))
  {
    goto free_buffer;
  }

  if (!ladish_write_string(fd, "\" terminal=\""))
  {
    goto free_buffer;
  }

  if (!ladish_write_string(fd, terminal ? "true" : "false"))
  {
    goto free_buffer;
  }

  if (!ladish_write_string(fd, "\" level=\""))
  {
    goto free_buffer;
  }

  sprintf(buf, "%u", (unsigned int)level);

  if (!ladish_write_string(fd, buf))
  {
    goto free_buffer;
  }

  if (!ladish_write_string(fd, "\" autorun=\""))
  {
    goto free_buffer;
  }

  if (!ladish_write_string(fd, running ? "true" : "false"))
  {
    goto free_buffer;
  }

  if (!ladish_write_string(fd, "\">"))
  {
    goto free_buffer;
  }

  unescaped_string = command;
  escaped_string = escaped_buffer;
  escape(&unescaped_string, &escaped_string, LADISH_ESCAPE_FLAG_ALL);
  *escaped_string = 0;
  if (!ladish_write_string(fd, escaped_buffer))
  {
    goto free_buffer;
  }

  if (!ladish_write_string(fd, "</application>\n"))
  {
    goto free_buffer;
  }

  ret = true;

free_buffer:
  free(escaped_buffer);

exit:
  return ret;
}

#undef ctx_ptr
#undef indent
#undef fd

bool ladish_write_vgraph(int fd, int indent, ladish_graph_handle vgraph, ladish_app_supervisor_handle app_supervisor)
{
  struct ladish_write_vgraph_context context;

  ladish_check_integrity();

  context.fd = fd;
  context.indent = indent + 1;
  context.app_supervisor = app_supervisor;

  if (!ladish_write_indented_string(fd, indent, "<clients>\n"))
  {
    return false;
  }

  if (!ladish_graph_iterate_nodes(
        vgraph,
        &context,
        ladish_save_vgraph_client_begin,
        ladish_save_vgraph_port,
        ladish_save_vgraph_client_end))
  {
    log_error("ladish_graph_iterate_nodes() failed");
    return false;
  }

  if (!ladish_write_indented_string(fd, indent, "</clients>\n"))
  {
    return false;
  }

  if (!ladish_write_indented_string(fd, indent, "<connections>\n"))
  {
    return false;
  }

  if (!ladish_graph_iterate_connections(vgraph, &context, ladish_save_vgraph_connection))
  {
    log_error("ladish_graph_iterate_connections() failed");
    return false;
  }

  if (!ladish_write_indented_string(fd, indent, "</connections>\n"))
  {
    return false;
  }

  if (!ladish_write_indented_string(fd, indent, "<applications>\n"))
  {
    return false;
  }

  if (!ladish_app_supervisor_enum(app_supervisor, &context, ladish_save_app))
  {
    return false;
  }

  if (!ladish_write_indented_string(fd, indent, "</applications>\n"))
  {
    return false;
  }

  return true;
}

/********************/
/* write jack graph */
/********************/

struct ladish_write_jack_context
{
  int fd;
  int indent;
  ladish_graph_handle vgraph_filter;
  ladish_app_supervisor_handle app_supervisor;
  bool client_vgraph_match;
  bool a2j;
  bool client_visible;
};

static bool ladish_save_jack_client_write_prolog(int fd, int indent, ladish_client_handle client_handle, const char * client_name)
{
  uuid_t uuid;
  char str[37];

  ladish_client_get_uuid(client_handle, uuid);
  uuid_unparse(uuid, str);

  log_info("saving jack client '%s' (%s)", client_name, str);

  if (!ladish_write_indented_string(fd, indent, "<client name=\""))
  {
    return false;
  }

  if (!ladish_write_string_escape_ex(fd, client_name, LADISH_ESCAPE_FLAG_XML_ATTR))
  {
    return false;
  }

  if (!ladish_write_string(fd, "\" uuid=\""))
  {
    return false;
  }

  if (!ladish_write_string(fd, str))
  {
    return false;
  }

  if (!ladish_write_string(fd, "\">\n"))
  {
    return false;
  }

  if (!ladish_write_indented_string(fd, indent + 1, "<ports>\n"))
  {
    return false;
  }

  return true;
}

#define fd (((struct ladish_write_jack_context *)context)->fd)
#define indent (((struct ladish_write_jack_context *)context)->indent)
#define ctx_ptr ((struct ladish_write_jack_context *)context)

static
bool
ladish_save_jack_client_begin(
  void * context,
  ladish_graph_handle graph_handle,
  bool hidden,
  ladish_client_handle client_handle,
  const char * client_name,
  void ** client_iteration_context_ptr_ptr)
{
  void * vgraph;

  vgraph = ladish_client_get_vgraph(client_handle);
  ctx_ptr->client_vgraph_match = vgraph == ctx_ptr->vgraph_filter;
  ctx_ptr->a2j = ladish_virtualizer_is_a2j_client(client_handle);

  /* for the a2j client vgraph is always the studio graph.
     However if studio has no a2j ports, lets not write a2j client.
     If there is a a2j port that matched the vgraph, the prolog will get written anyway */
  ctx_ptr->client_visible =
    (!hidden ||
     ladish_client_has_app(client_handle)) &&
     ctx_ptr->client_vgraph_match &&
     !ctx_ptr->a2j;
  if (!ctx_ptr->client_visible)
  {
    return true;
  }

  return ladish_save_jack_client_write_prolog(fd, indent, client_handle, client_name);
}

static
bool
ladish_save_jack_client_end(
  void * context,
  ladish_graph_handle graph_handle,
  bool hidden,
  ladish_client_handle client_handle,
  const char * client_name,
  void * client_iteration_context_ptr)
{
  if (!ctx_ptr->client_visible)
  {
    return true;
  }

  if (!ladish_write_indented_string(fd, indent + 1, "</ports>\n"))
  {
    return false;
  }

  if (!ladish_write_indented_string(fd, indent, "</client>\n"))
  {
    return false;
  }

  return true;
}

static
bool
ladish_save_jack_port(
  void * context,
  ladish_graph_handle graph_handle,
  bool hidden,
  void * client_iteration_context_ptr,
  ladish_client_handle client_handle,
  const char * client_name,
  ladish_port_handle port_handle,
  const char * port_name,
  uint32_t port_type,
  uint32_t port_flags)
{
  uuid_t uuid;
  char str[37];

  if (hidden && !ladish_port_has_app(port_handle))
  {
    return true;
  }

  /* check vgraph for a2j ports */
  if (ctx_ptr->a2j && ctx_ptr->vgraph_filter == ladish_port_get_vgraph(port_handle))
  {
    if (!ctx_ptr->client_visible)
    {
      if (!ladish_save_jack_client_write_prolog(fd, indent, client_handle, client_name))
      {
        return false;
      }

      ctx_ptr->client_visible = true;
    }
  }

  if (!ctx_ptr->client_visible)
  {
    return true;
  }

  /* skip hidden ports of running apps */
  if (hidden && !is_hidden_port_interesting(ctx_ptr->app_supervisor, client_handle, port_handle))
  {
    return true;
  }

  ladish_port_get_uuid(port_handle, uuid);
  uuid_unparse(uuid, str);

  log_info("saving jack port '%s':'%s' (%s)", client_name, port_name, str);

  if (!ladish_write_indented_string(fd, indent + 2, "<port name=\""))
  {
    return false;
  }

  if (!ladish_write_string_escape_ex(fd, port_name, LADISH_ESCAPE_FLAG_XML_ATTR))
  {
    return false;
  }

  if (!ladish_write_string(fd, "\" uuid=\""))
  {
    return false;
  }

  if (!ladish_write_string(fd, str))
  {
    return false;
  }

  if (!ladish_write_string(fd, "\" />\n"))
  {
    return false;
  }

  return true;
}

#undef ctx_ptr
#undef indent
#undef fd

bool ladish_write_jgraph(int fd, int indent, ladish_graph_handle vgraph, ladish_app_supervisor_handle app_supervisor)
{
  struct ladish_write_jack_context context;

  ladish_check_integrity();

  if (!ladish_write_indented_string(fd, indent, "<clients>\n"))
  {
    return false;
  }

  context.fd = fd;
  context.indent = indent + 1;
  context.vgraph_filter = vgraph;
  context.app_supervisor = app_supervisor;

  if (!ladish_graph_iterate_nodes(
        ladish_studio_get_jack_graph(),
        &context,
        ladish_save_jack_client_begin,
        ladish_save_jack_port,
        ladish_save_jack_client_end))
  {
    log_error("ladish_graph_iterate_nodes() failed");
    return false;
  }

  if (!ladish_write_indented_string(fd, indent, "</clients>\n"))
  {
    return false;
  }

  return true;
}
