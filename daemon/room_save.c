/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the parts of room object implementation
 * that are related to project save functionality
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
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "room_internal.h"
#include "../catdup.h"
#include "save.h"

#define PROJECT_HEADER_TEXT BASE_NAME " Project.\n"

static bool ladish_room_save_project_do(struct ladish_room * room_ptr)
{
  bool ret;
  time_t timestamp;
  char timestamp_str[26];
  char uuid_str[37];
  char * filename;
  char * bak_filename;
  int fd;

  log_info("Saving project '%s' in room '%s' to '%s'", room_ptr->project_name, room_ptr->name, room_ptr->project_dir);

  time(&timestamp);
  ctime_r(&timestamp, timestamp_str);
  timestamp_str[24] = 0;

  ret = false;

  uuid_generate(room_ptr->project_uuid); /* TODO: the uuid should be changed on "save as" but not on "rename" */
  uuid_unparse(room_ptr->project_uuid, uuid_str);

  filename = catdup(room_ptr->project_dir, LADISH_PROJECT_FILENAME);
  if (filename == NULL)
  {
    log_error("catdup() failed to compose project xml filename");
    goto exit;
  }

  bak_filename = catdup(filename, ".bak");
  if (bak_filename == NULL)
  {
    log_error("catdup() failed to compose project xml backup filename");
    goto free_filename;
  }

  fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0666);
  if (fd == -1)
  {
    log_error("open(%s) failed: %d (%s)", filename, errno, strerror(errno));
    goto free_bak_filename;
  }

  if (!ladish_write_string(fd, "<?xml version=\"1.0\"?>\n"))
  {
    goto close;
  }

  if (!ladish_write_string(fd, "<!--\n"))
  {
    goto close;
  }

  if (!ladish_write_string(fd, PROJECT_HEADER_TEXT))
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

  if (!ladish_write_string(fd, "<project name=\""))
  {
    goto close;
  }

  if (!ladish_write_string(fd, room_ptr->project_name)) /* TODO: escaping */
  {
    return false;
  }

  if (!ladish_write_string(fd, "\" uuid=\""))
  {
    return false;
  }

  if (!ladish_write_string(fd, uuid_str))
  {
    return false;
  }

  if (!ladish_write_string(fd, "\">\n"))
  {
    goto close;
  }

  if (!ladish_write_indented_string(fd, 1, "<room>\n"))
  {
    goto close;
  }

  if (!ladish_write_room_link_ports(fd, 2, (ladish_room_handle)room_ptr))
  {
    log_error("ladish_write_room_link_ports() failed");
    return false;
  }

  if (!ladish_write_indented_string(fd, 1, "</room>\n"))
  {
    goto close;
  }

  if (!ladish_write_indented_string(fd, 1, "<jack>\n"))
  {
    goto close;
  }

  if (!ladish_write_jgraph(fd, 2, room_ptr->graph))
  {
    log_error("ladish_write_jgraph() failed for room graph");
    goto close;
  }

  if (!ladish_write_indented_string(fd, 1, "</jack>\n"))
  {
    goto close;
  }

  if (!ladish_write_vgraph(fd, 1, room_ptr->graph, room_ptr->app_supervisor))
  {
    log_error("ladish_write_vgraph() failed for studio");
    goto close;
  }

  if (!ladish_write_dict(fd, 1, ladish_graph_get_dict(room_ptr->graph)))
  {
    goto close;
  }

  if (!ladish_write_string(fd, "</project>\n"))
  {
    goto close;
  }

  ladish_app_supervisor_save_L1(room_ptr->app_supervisor);

  ladish_room_emit_project_properties_changed(room_ptr);

  ret = true;

close:
  close(fd);
free_bak_filename:
  free(bak_filename);
free_filename:
  free(filename);
exit:
  return ret;
}

#define room_ptr ((struct ladish_room *)room_handle)

bool
ladish_room_save_project(
  ladish_room_handle room_handle,
  const char * project_dir_param,
  const char * project_name_param)
{
  bool first_time;
  bool dir_supplied;
  bool name_supplied;
  char * project_dir;
  char * project_name;
  char * old_project_dir;
  char * old_project_name;
  bool ret;

  ret = false;
  project_name = NULL;
  project_dir = NULL;

  /* project has either both name and dir no none of them */
  ASSERT((room_ptr->project_dir == NULL && room_ptr->project_name == NULL) || (room_ptr->project_dir != NULL && room_ptr->project_name != NULL));
  first_time = room_ptr->project_dir == NULL;

  dir_supplied = strlen(project_dir_param) != 0;
  name_supplied = strlen(project_name_param) != 0;

  if (first_time)
  {
    if (!dir_supplied && !name_supplied)
    {
      log_error("Cannot save unnamed project in room '%s'", room_ptr->name);
      goto exit;
    }

    if (dir_supplied && name_supplied)
    {
      project_dir = strdup(project_dir_param);
      project_name = strdup(project_name_param);
    }
    else if (dir_supplied)
    {
      ASSERT(!name_supplied);
      /* TODO */
      log_error("Deducing project name from project dir is not implemented yet");
      goto exit;
    }
    else if (name_supplied)
    {
      ASSERT(!dir_supplied);
      /* TODO */
      log_error("Deducing project dir from project name is not implemented yet");
      goto exit;
    }
    else
    {
      ASSERT_NO_PASS;
      goto exit;
    }

    ladish_app_supervisor_set_directory(room_ptr->app_supervisor, project_dir);
  }
  else
  {
    ASSERT(room_ptr->project_name != NULL);
    ASSERT(room_ptr->project_dir != NULL);

    project_name = name_supplied ? strdup(project_name_param) : room_ptr->project_name;
    project_dir = dir_supplied ? strdup(project_dir_param) : room_ptr->project_dir;
  }

  if (project_name == NULL || project_dir == NULL)
  {
    log_error("strdup() failed for project name or dir");
    goto exit;
  }

  old_project_dir = room_ptr->project_dir;
  old_project_name = room_ptr->project_name;
  room_ptr->project_name = project_name;
  room_ptr->project_dir = project_dir;
  ret = ladish_room_save_project_do(room_ptr);
  if (!ret)
  {
    room_ptr->project_name = old_project_name;
    room_ptr->project_dir = old_project_dir;
  }

exit:
  if (project_name != NULL && project_name != room_ptr->project_name)
  {
    free(room_ptr->project_name);
  }

  if (project_dir != NULL && project_dir != room_ptr->project_dir)
  {
    free(room_ptr->project_dir);
  }

  /* free strings that are allocated and stored only in the stack */
  if (project_name != NULL && project_name != room_ptr->project_name)
  {
    free(project_name);
  }
  if (project_dir != NULL && project_dir != room_ptr->project_dir)
  {
    free(project_dir);
  }

  return ret;
}

#undef room_ptr
