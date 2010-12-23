/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009,2010 Nedko Arnaudov <nedko@arnaudov.name>
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

#include <sys/types.h>
#include <signal.h>

#include "common.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "escape.h"
#include "studio_internal.h"
#include "cmd.h"
#include "../proxies/notify_proxy.h"
#include "save.h"

#define STUDIO_HEADER_TEXT BASE_NAME " Studio configuration.\n"

bool
write_jack_parameter(
  int fd,
  int indent,
  struct jack_conf_parameter * parameter_ptr)
{
  const char * src;
  char * dst;
  char path[max_escaped_length(JACK_CONF_MAX_ADDRESS_SIZE)];
  const char * content;
  char valbuf[100];

  /* compose the parameter path, percent-encode "bad" chars */
  src = parameter_ptr->address;
  dst = path;
  do
  {
    *dst++ = '/';
    escape(&src, &dst, LADISH_ESCAPE_FLAG_ALL);
    src++;
  }
  while (*src != 0);
  *dst = 0;

  if (!ladish_write_indented_string(fd, indent, "<parameter path=\""))
  {
    return false;
  }

  if (!ladish_write_string(fd, path))
  {
    return false;
  }

  if (!ladish_write_string(fd, "\">"))
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

  if (!ladish_write_string(fd, content))
  {
    return false;
  }

  if (!ladish_write_string(fd, "</parameter>\n"))
  {
    return false;
  }

  return true;
}

#define fd (((struct ladish_write_context *)context)->fd)
#define indent (((struct ladish_write_context *)context)->indent)

static bool save_studio_room(void * context, ladish_room_handle room)
{
  uuid_t uuid;
  char str[37];

  log_info("saving room '%s'", ladish_room_get_name(room));

  if (!ladish_write_indented_string(fd, indent, "<room name=\""))
  {
    return false;
  }

  if (!ladish_write_string(fd, ladish_room_get_name(room)))
  {
    return false;
  }

  if (!ladish_write_string(fd, "\" uuid=\""))
  {
    return false;
  }

  ladish_room_get_uuid(room, uuid);
  uuid_unparse(uuid, str);

  if (!ladish_write_string(fd, str))
  {
    return false;
  }

  if (!ladish_write_string(fd, "\">\n"))
  {
    return false;
  }

  if (!ladish_write_room_link_ports(fd, indent + 1, room))
  {
    log_error("ladish_write_room_link_ports() failed");
    return false;
  }

  if (!ladish_write_indented_string(fd, indent, " </room>\n"))
  {
    return false;
  }

  return true;
}

#undef indent
#undef fd

struct ladish_command_save_studio
{
  struct ladish_command command;
  char * studio_name;
};

#define cmd_ptr ((struct ladish_command_save_studio *)command_context)

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
  struct ladish_write_context save_context;
  bool renaming;

  ASSERT(cmd_ptr->command.state == LADISH_COMMAND_STATE_PENDING);

  time(&timestamp);
  ctime_r(&timestamp, timestamp_str);
  timestamp_str[24] = 0;

  ret = false;

  ladish_app_supervisor_save_L1(g_studio.app_supervisor);

  if (!ladish_studio_is_started())
  {
    log_error("Cannot save not-started studio");
    ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "Cannot save not-started studio", NULL);
    goto exit;
  }

  ladish_check_integrity();

  if (!ladish_studio_compose_filename(cmd_ptr->studio_name, &filename, &bak_filename))
  {
    log_error("failed to compose studio filename");
    goto exit;
  }

  /* whether save will initiate a rename */
  renaming = strcmp(cmd_ptr->studio_name, g_studio.name) != 0;

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
  else if (!renaming)
  {
    /* saving already renamed studio */
    old_filename = g_studio.filename;
    g_studio.filename = filename;
  }
  else
  {
    /* saving studio copy (save as) */
    old_filename = filename;
    g_studio.filename = filename;
  }

  filename = NULL;
  ASSERT(g_studio.filename != NULL);
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

  fd = open(g_studio.filename, O_WRONLY | O_TRUNC | O_CREAT, 0666);
  if (fd == -1)
  {
    log_error("open(%s) failed: %d (%s)", g_studio.filename, errno, strerror(errno));
    goto rename_back;
  }

  if (!ladish_write_string(fd, "<?xml version=\"1.0\"?>\n"))
  {
    goto close;
  }

  if (!ladish_write_string(fd, "<!--\n"))
  {
    goto close;
  }

  if (!ladish_write_string(fd, STUDIO_HEADER_TEXT))
  {
    goto close;
  }

  if (!ladish_write_string(fd, "-->\n"))
  {
    goto close;
  }

  if (!ladish_write_string(fd, "<!-- "))
  {
    goto close;
  }

  if (!ladish_write_string(fd, timestamp_str))
  {
    goto close;
  }

  if (!ladish_write_string(fd, " -->\n"))
  {
    goto close;
  }

  if (!ladish_write_string(fd, "<studio>\n"))
  {
    goto close;
  }

  if (!ladish_write_indented_string(fd, 1, "<jack>\n"))
  {
    goto close;
  }

  if (!ladish_write_indented_string(fd, 2, "<conf>\n"))
  {
    goto close;
  }

  list_for_each(node_ptr, &g_studio.jack_params)
  {
    parameter_ptr = list_entry(node_ptr, struct jack_conf_parameter, leaves);

    if (!write_jack_parameter(fd, 3, parameter_ptr))
    {
      goto close;
    }
  }

  if (!ladish_write_indented_string(fd, 2, "</conf>\n"))
  {
    goto close;
  }

  if (!ladish_write_jgraph(fd, 2, ladish_studio_get_studio_graph()))
  {
    log_error("ladish_write_jgraph() failed for studio graph");
    goto close;
  }

  if (!ladish_write_indented_string(fd, 1, "</jack>\n"))
  {
    goto close;
  }

  if (ladish_studio_has_rooms())
  {
    if (!ladish_write_indented_string(fd, 1, "<rooms>\n"))
    {
      goto close;
    }

    save_context.indent = 2;
    save_context.fd = fd;

    if (!ladish_studio_iterate_rooms(&save_context, save_studio_room))
    {
      log_error("ladish_studio_iterate_rooms() failed");
      goto close;
    }

    if (!ladish_write_indented_string(fd, 1, "</rooms>\n"))
    {
      goto close;
    }
  }

  if (!ladish_write_vgraph(fd, 1, g_studio.studio_graph, g_studio.app_supervisor))
  {
    log_error("ladish_write_vgraph() failed for studio");
    goto close;
  }

  if (!ladish_write_dict(fd, 1, ladish_graph_get_dict(g_studio.studio_graph)))
  {
    goto close;
  }

  if (!ladish_write_string(fd, "</studio>\n"))
  {
    goto close;
  }

  log_info("studio saved. (%s)", g_studio.filename);
  g_studio.persisted = true;
  g_studio.automatic = false;   /* even if it was automatic, it is not anymore because it is saved */

  cmd_ptr->command.state = LADISH_COMMAND_STATE_DONE;

  ret = true;

  if (renaming)
  {
    free(g_studio.name);
    g_studio.name = cmd_ptr->studio_name;
    cmd_ptr->studio_name = NULL; /* mark that descructor does not need to free the new name buffer */
    ladish_studio_emit_renamed(); /* uses g_studio.name */
  }

close:
  close(fd);

rename_back:
  if (!ret && bak_filename != NULL)
  {
    /* save failed - try to rename the backup file back */
    ASSERT(old_filename != NULL);
    if (rename(bak_filename, old_filename) != 0)
    {
      log_error("rename(%s, %s) failed: %d (%s)", bak_filename, g_studio.filename, errno, strerror(errno));
    }
  }

free_filenames:
  if (bak_filename != NULL)
  {
    free(bak_filename);
  }

  if (old_filename != NULL && old_filename != g_studio.filename)
  {
    free(old_filename);
  }

  ASSERT(filename == NULL);
  ASSERT(g_studio.filename != NULL);

exit:
  if (!ret)
  {
    ladish_notify_simple(LADISH_NOTIFY_URGENCY_HIGH, "Studio save failed", LADISH_CHECK_LOG_TEXT);
  }

  return ret;
}

static void destructor(void * command_context)
{
  log_info("save studio command destructor");
  if (cmd_ptr->studio_name != NULL)
  {
    free(cmd_ptr->studio_name);
  }
}

#undef cmd_ptr

bool ladish_command_save_studio(void * call_ptr, struct ladish_cqueue * queue_ptr, const char * new_studio_name)
{
  struct ladish_command_save_studio * cmd_ptr;
  char * studio_name_dup;

  studio_name_dup = strdup(new_studio_name);
  if (studio_name_dup == NULL)
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "strdup('%s') failed.", new_studio_name);
    goto fail;
  }

  cmd_ptr = ladish_command_new(sizeof(struct ladish_command_save_studio));
  if (cmd_ptr == NULL)
  {
    log_error("ladish_command_new() failed.");
    goto fail_free_name;
  }

  cmd_ptr->command.run = run;
  cmd_ptr->command.destructor = destructor;
  cmd_ptr->studio_name = studio_name_dup;

  if (!ladish_cqueue_add_command(queue_ptr, &cmd_ptr->command))
  {
    lash_dbus_error(call_ptr, LASH_DBUS_ERROR_GENERIC, "ladish_cqueue_add_command() failed.");
    goto fail_destroy_command;
  }

  return true;

fail_destroy_command:
  free(cmd_ptr);
fail_free_name:
  free(studio_name_dup);
fail:
  return false;
}
