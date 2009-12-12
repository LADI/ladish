/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the "save studio" command
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
#include "studio_internal.h"
#include "cmd.h"

struct save_context
{
  int fd;
  const char * indent;
};

#define STUDIO_HEADER_TEXT BASE_NAME " Studio configuration.\n"

bool
write_string(int fd, const char * string)
{
  size_t len;

  len = strlen(string);

  if (write(fd, string, len) != len)
  {
    log_error("write() failed to write config file.");
    return false;
  }

  return true;
}

bool
write_jack_parameter(
  int fd,
  const char * indent,
  struct jack_conf_parameter * parameter_ptr)
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

  if (!write_string(fd, indent))
  {
    return false;
  }

  if (!write_string(fd, "<parameter path=\""))
  {
    return false;
  }

  if (!write_string(fd, path))
  {
    return false;
  }

  if (!write_string(fd, "\">"))
  {
    return false;
  }

  switch (parameter_ptr->parameter.type)
  {
  case jack_boolean:
    content = parameter_ptr->parameter.value.boolean ? "true" : "false";
    log_debug("%s value is %s (boolean)", path, content);
    break;
  case jack_string:
    content = parameter_ptr->parameter.value.string;
    log_debug("%s value is %s (string)", path, content);
    break;
  case jack_byte:
    valbuf[0] = (char)parameter_ptr->parameter.value.byte;
    valbuf[1] = 0;
    content = valbuf;
    log_debug("%s value is %u/%c (byte/char)", path, parameter_ptr->parameter.value.byte, (char)parameter_ptr->parameter.value.byte);
    break;
  case jack_uint32:
    snprintf(valbuf, sizeof(valbuf), "%" PRIu32, parameter_ptr->parameter.value.uint32);
    content = valbuf;
    log_debug("%s value is %s (uint32)", path, content);
    break;
  case jack_int32:
    snprintf(valbuf, sizeof(valbuf), "%" PRIi32, parameter_ptr->parameter.value.int32);
    content = valbuf;
    log_debug("%s value is %s (int32)", path, content);
    break;
  default:
    log_error("unknown jack parameter_ptr->parameter type %d (%s)", (int)parameter_ptr->parameter.type, path);
    return false;
  }

  if (!write_string(fd, content))
  {
    return false;
  }

  if (!write_string(fd, "</parameter>\n"))
  {
    return false;
  }

  return true;
}

static
bool
write_dict_entry(
  void * context,
  const char * key,
  const char * value);

bool
write_dict(
  int fd,
  const char * indent,
  ladish_dict_handle dict)
{
  struct save_context context;

  if (ladish_dict_is_empty(dict))
  {
    return true;
  }

  if (!write_string(fd, indent))
  {
    return false;
  }

  context.fd = fd;
  context.indent = indent;

  if (!write_string(fd, "<dict>\n"))
  {
    return false;
  }

  if (!ladish_dict_iterate(dict, &context, write_dict_entry))
  {
    return false;
  }

  if (!write_string(fd, indent))
  {
    return false;
  }

  if (!write_string(fd, "</dict>\n"))
  {
    return false;
  }

  return true;
}

#define fd (((struct save_context *)context)->fd)
#define indent (((struct save_context *)context)->indent)

static
bool
write_dict_entry(
  void * context,
  const char * key,
  const char * value)
{
  if (!write_string(fd, indent))
  {
    return false;
  }

  if (!write_string(fd, "  <key name=\""))
  {
    return false;
  }

  if (!write_string(fd, key))
  {
    return false;
  }

  if (!write_string(fd, "\">"))
  {
    return false;
  }

  if (!write_string(fd, value))
  {
    return false;
  }

  if (!write_string(fd, "</key>\n"))
  {
    return false;
  }

  return true;
}

bool
save_jack_client_begin(
  void * context,
  ladish_client_handle client_handle,
  const char * client_name,
  void ** client_iteration_context_ptr_ptr)
{
  uuid_t uuid;
  char str[37];

  ladish_client_get_uuid(client_handle, uuid);
  uuid_unparse(uuid, str);

  log_info("saving jack client '%s' (%s)", client_name, str);

  if (!write_string(fd, "      <client name=\""))
  {
    return false;
  }

  if (!write_string(fd, client_name))
  {
    return false;
  }

  if (!write_string(fd, "\" uuid=\""))
  {
    return false;
  }

  if (!write_string(fd, str))
  {
    return false;
  }

  if (!write_string(fd, "\">\n"))
  {
    return false;
  }

  if (!write_string(fd, "        <ports>\n"))
  {
    return false;
  }

  return true;
}

bool
save_jack_client_end(
  void * context,
  ladish_client_handle client_handle,
  const char * client_name,
  void * client_iteration_context_ptr)
{
  if (!write_string(fd, "        </ports>\n"))
  {
    return false;
  }

  if (!write_string(fd, "      </client>\n"))
  {
    return false;
  }

  return true;
}

bool
save_jack_port(
  void * context,
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

  ladish_port_get_uuid(port_handle, uuid);
  uuid_unparse(uuid, str);

  log_info("saving jack port '%s':'%s' (%s)", client_name, port_name, str);

  if (!write_string(fd, "          <port name=\""))
  {
    return false;
  }

  if (!write_string(fd, port_name))
  {
    return false;
  }

  if (!write_string(fd, "\" uuid=\""))
  {
    return false;
  }

  if (!write_string(fd, str))
  {
    return false;
  }

  if (!write_string(fd, "\" />\n"))
  {
    return false;
  }

  return true;
}

bool
save_studio_client_begin(
  void * context,
  ladish_client_handle client_handle,
  const char * client_name,
  void ** client_iteration_context_ptr_ptr)
{
  uuid_t uuid;
  char str[37];

  ladish_client_get_uuid(client_handle, uuid);
  uuid_unparse(uuid, str);

  log_info("saving studio client '%s' (%s)", client_name, str);

  if (!write_string(fd, "    <client name=\""))
  {
    return false;
  }

  if (!write_string(fd, client_name))
  {
    return false;
  }

  if (!write_string(fd, "\" uuid=\""))
  {
    return false;
  }

  if (!write_string(fd, str))
  {
    return false;
  }

  if (!write_string(fd, "\">\n"))
  {
    return false;
  }

  if (!write_string(fd, "      <ports>\n"))
  {
    return false;
  }

  return true;
}

bool
save_studio_client_end(
  void * context,
  ladish_client_handle client_handle,
  const char * client_name,
  void * client_iteration_context_ptr)
{
  if (!write_string(fd, "      </ports>\n"))
  {
    return false;
  }

  if (!write_dict(fd, "      ", ladish_client_get_dict(client_handle)))
  {
    return false;
  }

  if (!write_string(fd, "    </client>\n"))
  {
    return false;
  }

  return true;
}

bool
save_studio_port(
  void * context,
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
  ladish_dict_handle dict;

  ladish_port_get_uuid(port_handle, uuid);
  uuid_unparse(uuid, str);

  log_info("saving studio port '%s':'%s' (%s)", client_name, port_name, str);

  if (!write_string(fd, "        <port name=\""))
  {
    return false;
  }

  if (!write_string(fd, port_name))
  {
    return false;
  }

  if (!write_string(fd, "\" uuid=\""))
  {
    return false;
  }

  if (!write_string(fd, str))
  {
    return false;
  }

  dict = ladish_port_get_dict(port_handle);
  if (ladish_dict_is_empty(dict))
  {
    if (!write_string(fd, "\" />\n"))
    {
      return false;
    }
  }
  else
  {
    if (!write_string(fd, "\">\n"))
    {
      return false;
    }

    if (!write_dict(fd, "          ", dict))
    {
      return false;
    }

    if (!write_string(fd, "        </port>\n"))
    {
      return false;
    }
  }

  return true;
}

bool save_studio_connection(void * context, ladish_port_handle port1_handle, ladish_port_handle port2_handle, ladish_dict_handle dict)
{
  uuid_t uuid;
  char str[37];

  log_info("saving studio connection");

  if (!write_string(fd, "    <connection port1=\""))
  {
    return false;
  }

  ladish_port_get_uuid(port1_handle, uuid);
  uuid_unparse(uuid, str);

  if (!write_string(fd, str))
  {
    return false;
  }

  if (!write_string(fd, "\" port2=\""))
  {
    return false;
  }

  ladish_port_get_uuid(port2_handle, uuid);
  uuid_unparse(uuid, str);

  if (!write_string(fd, str))
  {
    return false;
  }

  if (ladish_dict_is_empty(dict))
  {
    if (!write_string(fd, "\" />\n"))
    {
      return false;
    }
  }
  else
  {
    if (!write_string(fd, "\">\n"))
    {
      return false;
    }

    if (!write_dict(fd, "      ", dict))
    {
      return false;
    }

    if (!write_string(fd, "    </connection>\n"))
    {
      return false;
    }
  }

  return true;
}

bool save_studio_app(void * context, const char * name, bool running, const char * command, bool terminal, uint8_t level)
{
  char buf[100];
  const char * unescaped_string;
  char * escaped_string;
  char * escaped_buffer;
  bool ret;

  log_info("saving app: name='%s', %srunning, %s, level %u, commandline='%s'", name, running ? "" : "not ", terminal ? "terminal" : "shell", (unsigned int)level, command);

  ret = false;

  escaped_buffer = malloc(ladish_max(strlen(name), strlen(command)) * 3 + 1); /* encode each char in three bytes (percent encoding) */
  if (escaped_buffer == NULL)
  {
    log_error("malloc() failed.");
    goto exit;
  }

  if (!write_string(fd, "    <application name=\""))
  {
    goto free_buffer;
  }

  unescaped_string = name;
  escaped_string = escaped_buffer;
  escape(&unescaped_string, &escaped_string);
  *escaped_string = 0;
  if (!write_string(fd, escaped_buffer))
  {
    goto free_buffer;
  }

  if (!write_string(fd, "\" terminal=\""))
  {
    goto free_buffer;
  }

  if (!write_string(fd, terminal ? "true" : "false"))
  {
    goto free_buffer;
  }

  if (!write_string(fd, "\" level=\""))
  {
    goto free_buffer;
  }

  sprintf(buf, "%u", (unsigned int)level);

  if (!write_string(fd, buf))
  {
    goto free_buffer;
  }

  if (!write_string(fd, "\" autorun=\""))
  {
    goto free_buffer;
  }

  if (!write_string(fd, running ? "true" : "false"))
  {
    goto free_buffer;
  }

  if (!write_string(fd, "\">"))
  {
    goto free_buffer;
  }

  unescaped_string = command;
  escaped_string = escaped_buffer;
  escape(&unescaped_string, &escaped_string);
  *escaped_string = 0;
  if (!write_string(fd, escaped_buffer))
  {
    goto free_buffer;
  }

  if (!write_string(fd, "</application>\n"))
  {
    goto free_buffer;
  }

  ret = true;

free_buffer:
  free(escaped_buffer);

exit:
  return ret;
}

#undef indent
#undef fd

#define cmd_ptr ((struct ladish_command *)command_context)

static bool run(void * command_context)
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
  struct save_context save_context;

  ASSERT(cmd_ptr->state == LADISH_COMMAND_STATE_PENDING);

  time(&timestamp);
  ctime_r(&timestamp, timestamp_str);
  timestamp_str[24] = 0;

  ret = false;

  if (!studio_is_started())
  {
    log_error("Cannot save not-started studio");
    goto exit;
  }

  if (!studio_compose_filename(g_studio.name, &filename, &bak_filename))
  {
    log_error("failed to compose studio filename");
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
  ASSERT(g_studio.filename != NULL);
  ASSERT(g_studio.filename != old_filename);
  ASSERT(g_studio.filename != bak_filename);

  if (bak_filename != NULL)
  {
    ASSERT(old_filename != NULL);

    if (stat(old_filename, &st) == 0) /* if old filename does not exist, rename with fail */
    {
      if (rename(old_filename, bak_filename) != 0)
      {
        log_error("rename(%s, %s) failed: %d (%s)", old_filename, bak_filename, errno, strerror(errno));
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

  log_info("saving studio... (%s)", g_studio.filename);

  fd = open(g_studio.filename, O_WRONLY | O_TRUNC | O_CREAT, 0700);
  if (fd == -1)
  {
    log_error("open(%s) failed: %d (%s)", g_studio.filename, errno, strerror(errno));
    goto rename_back;
  }

  if (!write_string(fd, "<?xml version=\"1.0\"?>\n"))
  {
    goto close;
  }

  if (!write_string(fd, "<!--\n"))
  {
    goto close;
  }

  if (!write_string(fd, STUDIO_HEADER_TEXT))
  {
    goto close;
  }

  if (!write_string(fd, "-->\n"))
  {
    goto close;
  }

  if (!write_string(fd, "<!-- "))
  {
    goto close;
  }

  if (!write_string(fd, timestamp_str))
  {
    goto close;
  }

  if (!write_string(fd, " -->\n"))
  {
    goto close;
  }

  if (!write_string(fd, "<studio>\n"))
  {
    goto close;
  }

  if (!write_string(fd, "  <jack>\n"))
  {
    goto close;
  }

  if (!write_string(fd, "    <conf>\n"))
  {
    goto close;
  }

  list_for_each(node_ptr, &g_studio.jack_params)
  {
    parameter_ptr = list_entry(node_ptr, struct jack_conf_parameter, leaves);

    if (!write_jack_parameter(fd, "      ", parameter_ptr))
    {
      goto close;
    }
  }

  if (!write_string(fd, "    </conf>\n"))
  {
    goto close;
  }

  if (!write_string(fd, "    <clients>\n"))
  {
    goto close;
  }

  save_context.fd = fd;

  if (!ladish_graph_iterate_nodes(g_studio.jack_graph, &save_context, save_jack_client_begin, save_jack_port, save_jack_client_end))
  {
    log_error("ladish_graph_iterate_nodes() failed");
    goto close;
  }

  if (!write_string(fd, "    </clients>\n"))
  {
    goto close;
  }

  if (!write_string(fd, "  </jack>\n"))
  {
    goto close;
  }

  if (!write_string(fd, "  <clients>\n"))
  {
    goto close;
  }

  if (!ladish_graph_iterate_nodes(g_studio.studio_graph, &save_context, save_studio_client_begin, save_studio_port, save_studio_client_end))
  {
    log_error("ladish_graph_iterate_nodes() failed");
    goto close;
  }

  if (!write_string(fd, "  </clients>\n"))
  {
    goto close;
  }

  if (!write_string(fd, "  <connections>\n"))
  {
    goto close;
  }

  if (!ladish_graph_iterate_connections(g_studio.studio_graph, &save_context, save_studio_connection))
  {
    log_error("ladish_graph_iterate_connections() failed");
    goto close;
  }

  if (!write_string(fd, "  </connections>\n"))
  {
    goto close;
  }

  if (!write_string(fd, "  <applications>\n"))
  {
    goto close;
  }

  if (!ladish_app_supervisor_enum(g_studio.app_supervisor, &save_context, save_studio_app))
  {
    goto close;
  }

  if (!write_string(fd, "  </applications>\n"))
  {
    goto close;
  }

  if (!write_dict(fd, "  ", ladish_graph_get_dict(g_studio.studio_graph)))
  {
    goto close;
  }

  if (!write_string(fd, "</studio>\n"))
  {
    goto close;
  }

  log_info("studio saved. (%s)", g_studio.filename);
  g_studio.persisted = true;
  g_studio.automatic = false;   /* even if it was automatic, it is not anymore because it is saved */

  cmd_ptr->state = LADISH_COMMAND_STATE_DONE;

  ret = true;

close:
  close(fd);

rename_back:
  if (!ret && bak_filename != NULL)
  {
    /* save failed - try to rename the backup file back */
    if (rename(bak_filename, g_studio.filename) != 0)
    {
      log_error("rename(%s, %s) failed: %d (%s)", bak_filename, g_studio.filename, errno, strerror(errno));
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

  ASSERT(filename == NULL);
  ASSERT(g_studio.filename != NULL);

exit:
  return ret;
}

#undef cmd_ptr

bool ladish_command_save_studio(void * call_ptr, struct ladish_cqueue * queue_ptr)
{
  struct ladish_command * cmd_ptr;

  cmd_ptr = ladish_command_new(sizeof(struct ladish_command));
  if (cmd_ptr == NULL)
  {
    log_error("ladish_command_new() failed.");
    goto fail;
  }

  cmd_ptr->run = run;

  if (!ladish_cqueue_add_command(queue_ptr, cmd_ptr))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "ladish_cqueue_add_command() failed.");
    goto fail_destroy_command;
  }

  return true;

fail_destroy_command:
  free(cmd_ptr);

fail:
  return false;
}
