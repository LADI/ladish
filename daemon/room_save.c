/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010,2011,2012 Nedko Arnaudov <nedko@arnaudov.name>
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
#include <libgen.h>             /* POSIX basename() */

#include "room_internal.h"
#include "../common/catdup.h"
#include "save.h"
#include "../common/dirhelpers.h"
#include "escape.h"

#define PROJECT_HEADER_TEXT BASE_NAME " Project.\n"
#define DEFAULT_PROJECT_BASE_DIR "/ladish-projects/"
#define DEFAULT_PROJECT_BASE_DIR_LEN ((size_t)(sizeof(DEFAULT_PROJECT_BASE_DIR) - 1))

struct ladish_room_save_context
{
  struct ladish_room * room;
  char * project_dir;
  char * project_name;
  char * old_project_dir;
  char * old_project_name;

  void * context;
  ladish_room_save_complete_callback callback;
};

static void ladish_room_save_context_destroy(struct ladish_room_save_context * ctx_ptr)
{
  if (ctx_ptr->project_name != NULL && ctx_ptr->project_name != ctx_ptr->room->project_name)
  {
    free(ctx_ptr->room->project_name);
  }

  if (ctx_ptr->project_dir != NULL && ctx_ptr->project_dir != ctx_ptr->room->project_dir)
  {
    free(ctx_ptr->room->project_dir);
  }

  /* free strings that are allocated and stored only in the stack */
  if (ctx_ptr->project_name != NULL && ctx_ptr->project_name != ctx_ptr->room->project_name)
  {
    free(ctx_ptr->project_name);
  }

  if (ctx_ptr->project_dir != NULL && ctx_ptr->project_dir != ctx_ptr->room->project_dir)
  {
    free(ctx_ptr->project_dir);
  }

  free(ctx_ptr);
}

static void ladish_room_save_complete(struct ladish_room_save_context * ctx_ptr, bool success)
{
  if (!success)
  {
    ctx_ptr->room->project_name = ctx_ptr->old_project_name;
    ctx_ptr->room->project_dir = ctx_ptr->old_project_dir;
  }

  ctx_ptr->callback(ctx_ptr->context, success);

  ladish_room_save_context_destroy(ctx_ptr);
}

static bool ladish_room_save_project_xml(struct ladish_room * room_ptr)
{
  bool ret;
  time_t timestamp;
  char timestamp_str[26];
  char uuid_str[37];
  char * filename;
  char * bak_filename;
  int fd;

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

  if (!ladish_write_string_escape(fd, room_ptr->project_name))
  {
    goto close;
  }

  if (!ladish_write_string(fd, "\" uuid=\""))
  {
    goto close;
  }

  if (!ladish_write_string(fd, uuid_str))
  {
    goto close;
  }

  if (!ladish_write_string(fd, "\">\n"))
  {
    goto close;
  }

  if (room_ptr->project_description != NULL)
  {
    if (!ladish_write_indented_string(fd, 1, "<description>"))
    {
      goto close;
    }

    if (!ladish_write_string_escape(fd, room_ptr->project_description))
    {
      goto close;
    }

    if (!ladish_write_string(fd, "</description>\n"))
    {
      goto close;
    }
  }

  if (room_ptr->project_notes != NULL)
  {
    if (!ladish_write_indented_string(fd, 1, "<notes>"))
    {
      goto close;
    }

    if (!ladish_write_string_escape(fd, room_ptr->project_notes))
    {
      goto close;
    }

    if (!ladish_write_string(fd, "</notes>\n"))
    {
      goto close;
    }
  }

  if (!ladish_write_indented_string(fd, 1, "<room>\n"))
  {
    goto close;
  }

  if (!ladish_write_room_link_ports(fd, 2, (ladish_room_handle)room_ptr))
  {
    log_error("ladish_write_room_link_ports() failed");
    goto close;
  }

  if (!ladish_write_indented_string(fd, 1, "</room>\n"))
  {
    goto close;
  }

  if (!ladish_write_indented_string(fd, 1, "<jack>\n"))
  {
    goto close;
  }

  if (!ladish_write_jgraph(fd, 2, room_ptr->graph, room_ptr->app_supervisor))
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

#define ctx_ptr ((struct ladish_room_save_context *)context)

static void ladish_room_apps_save_complete(void * context, bool success)
{
  log_info("Project '%s' apps in room '%s' %s to '%s'", ctx_ptr->room->project_name, ctx_ptr->room->name, success ? "saved successfully" : "failed to save", ctx_ptr->room->project_dir);

  if (!success)
  {
    ladish_room_save_complete(ctx_ptr, false);
    return;
  }

  if (!ladish_room_save_project_xml(ctx_ptr->room))
  {
    ladish_room_save_complete(ctx_ptr, false);
    /* TODO: try to rollback apps save stage */
    return;
  }

  ladish_room_emit_project_properties_changed(ctx_ptr->room);

  ctx_ptr->room->project_state = ROOM_PROJECT_STATE_LOADED;

  ladish_room_save_complete(ctx_ptr, true);
}

#undef ctx_ptr

static void ladish_room_save_project_do(struct ladish_room_save_context * ctx_ptr)
{
  log_info("Saving project '%s' in room '%s' to '%s'", ctx_ptr->room->project_name, ctx_ptr->room->name, ctx_ptr->room->project_dir);

  ladish_check_integrity();

  if (!ensure_dir_exist(ctx_ptr->room->project_dir, 0777))
  {
    ladish_room_save_complete(ctx_ptr, false);
    return;
  }

  ladish_app_supervisor_save(ctx_ptr->room->app_supervisor, ctx_ptr, ladish_room_apps_save_complete);
}

/* TODO: base dir must be a runtime setting */
char * compose_project_dir_from_name(const char * project_name)
{
  const char * home_dir;
  char * project_dir;
  size_t home_dir_len;

  home_dir = getenv("HOME");
  if (home_dir == NULL)
  {
    log_error("HOME env var is not set. Cannot decude project directory.");
    return NULL;
  }

  home_dir_len = strlen(home_dir);

  project_dir = malloc(home_dir_len + DEFAULT_PROJECT_BASE_DIR_LEN + max_escaped_length(strlen(project_name)) + 1);
  if (project_dir == NULL)
  {
    log_error("malloc() failed to allocate buffer for project dir");
    return NULL;
  }

  memcpy(project_dir, home_dir, home_dir_len);
  memcpy(project_dir + home_dir_len, DEFAULT_PROJECT_BASE_DIR, DEFAULT_PROJECT_BASE_DIR_LEN);
  escape_simple(project_name, project_dir + home_dir_len + DEFAULT_PROJECT_BASE_DIR_LEN, LADISH_ESCAPE_FLAG_ALL);

  return project_dir;
}

#define room_ptr ((struct ladish_room *)room_handle)

void
ladish_room_save_project(
  ladish_room_handle room_handle,
  const char * project_dir_param,
  const char * project_name_param,
  void * context,
  ladish_room_save_complete_callback callback)
{
  struct ladish_room_save_context * ctx_ptr;
  bool first_time;
  bool dir_supplied;
  bool name_supplied;
  char * buffer;

  ctx_ptr = malloc(sizeof(struct ladish_room_save_context));
  if (ctx_ptr == NULL)
  {
    log_error("malloc() failed to allocate memory for room save context struct");
    callback(context, false);
    return;
  }

  ctx_ptr->room = room_ptr;
  ctx_ptr->project_name = NULL;
  ctx_ptr->project_dir = NULL;
  ctx_ptr->context = context;
  ctx_ptr->callback = callback;

  ctx_ptr->old_project_dir = room_ptr->project_dir;
  ctx_ptr->old_project_name = room_ptr->project_name;

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
      goto destroy_ctx;
    }

    if (dir_supplied && name_supplied)
    {
      ctx_ptr->project_dir = strdup(project_dir_param);
      ctx_ptr->project_name = strdup(project_name_param);
    }
    else if (dir_supplied)
    {
      ASSERT(!name_supplied);

      buffer = strdup(project_dir_param);
      if (buffer == NULL)
      {
        log_error("strdup() failed for project dir");
        goto destroy_ctx;
      }

      ctx_ptr->project_name = basename(buffer);
      log_info("Project name for dir '%s' will be '%s'", project_dir_param, ctx_ptr->project_name);
      ctx_ptr->project_name = strdup(ctx_ptr->project_name);
      free(buffer);
      ctx_ptr->project_dir = strdup(project_dir_param); /* buffer cannot be used because it may be modified by basename() */
    }
    else if (name_supplied)
    {
      ASSERT(!dir_supplied);

      ctx_ptr->project_dir = compose_project_dir_from_name(project_name_param);
      if (ctx_ptr->project_dir == NULL)
      {
        goto destroy_ctx;
      }

      log_info("Project dir for name '%s' will be '%s'", project_name_param, ctx_ptr->project_dir);

      ctx_ptr->project_name = strdup(project_name_param);
    }
    else
    {
      ASSERT_NO_PASS;
      goto destroy_ctx;
    }

    ladish_app_supervisor_set_directory(room_ptr->app_supervisor, ctx_ptr->project_dir);
  }
  else
  {
    ASSERT(room_ptr->project_name != NULL);
    ASSERT(room_ptr->project_dir != NULL);

    ctx_ptr->project_name = name_supplied ? strdup(project_name_param) : room_ptr->project_name;
    ctx_ptr->project_dir = dir_supplied ? strdup(project_dir_param) : room_ptr->project_dir;
  }

  if (ctx_ptr->project_name == NULL || ctx_ptr->project_dir == NULL)
  {
    log_error("strdup() failed for project name or dir");
    goto destroy_ctx;
  }

  room_ptr->project_name = ctx_ptr->project_name;
  room_ptr->project_dir = ctx_ptr->project_dir;

  ladish_room_save_project_do(ctx_ptr);
  return;

destroy_ctx:
  ladish_room_save_complete(ctx_ptr, false);
}

#undef room_ptr
