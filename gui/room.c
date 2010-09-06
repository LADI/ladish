/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains room related code
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

#include "create_room_dialog.h"
#include "load_project_dialog.h"
#include "save_project_dialog.h"
#include "../proxies/studio_proxy.h"
#include "../proxies/room_proxy.h"
#include "graph_view.h"
#include "menu.h"
#include "world_tree.h"

void menu_request_create_room(void)
{
  char * name;
  char * template;

  log_info("create room request");

  if (create_room_dialog_run(&name, &template))
  {
    log_info("Creating new room '%s' from template '%s'", name, template);

    if (!studio_proxy_create_room(name, template))
    {
      error_message_box("Room creation failed, please inspect logs.");
    }

    free(name);
    free(template);
  }
}

void menu_request_destroy_room(void)
{
  const char * room;

  room = get_current_view_room_name();
  if (room == NULL)
  {
    ASSERT_NO_PASS;
    return;
  }

  log_info("destroy room '%s' request", room);

  if (!studio_proxy_delete_room(room))
  {
    error_message_box("Room deletion failed, please inspect logs.");
  }
}

void menu_request_load_project(void)
{
  ladish_run_load_project_dialog(graph_view_get_room(get_current_view()));
}

void menu_request_unload_project(void)
{
  if (!ladish_room_proxy_unload_project(graph_view_get_room(get_current_view())))
  {
    log_error("ladish_room_proxy_unload_project() failed");
  }
}

void menu_request_save_project(void)
{
  bool new_project;
  char * project_dir;
  char * project_name;

  if (!ladish_room_proxy_get_project_properties(graph_view_get_room(get_current_view()), &project_dir, &project_name))
  {
    error_message_box("Get project properties failed, please inspect logs.");
    return;
  }

  new_project = strlen(project_dir) == 0;

  free(project_name);
  free(project_dir);

  if (new_project)
  {
    menu_request_save_as_project();
    return;
  }

  if (!ladish_room_proxy_save_project(graph_view_get_room(get_current_view()), "", ""))
  {
    log_error("ladish_room_proxy_unload_project() failed");
  }
}

void menu_request_save_as_project(void)
{
  ladish_run_save_project_dialog(graph_view_get_room(get_current_view()));
}

static void room_appeared(const char * opath, const char * name, const char * template)
{
  graph_view_handle graph_view;

  log_info("room \"%s\" appeared (%s). template is \"%s\"", name, opath, template);

  if (!create_view(name, SERVICE_NAME, opath, true, true, false, &graph_view))
  {
    log_error("create_view() failed for room \"%s\"", name);
  }
}

static void room_disappeared(const char * opath, const char * name, const char * template)
{
  graph_view_handle graph_view;

  log_info("room \"%s\" disappeared (%s). template is \"%s\"", name, opath, template);

  graph_view = world_tree_find_by_opath(opath);
  if (graph_view == NULL)
  {
    log_error("Unknown room disappeared");
    return;
  }

  destroy_view(graph_view);
}

static void room_changed(const char * opath, const char * name, const char * template)
{
  log_info("%s changed. name is \"%s\". template is \"%s\"", opath, name, template);
}

void set_room_callbacks(void)
{
  studio_proxy_set_room_callbacks(room_appeared, room_disappeared, room_changed);
}
