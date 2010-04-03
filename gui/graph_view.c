/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the graph view object
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
#include "graph_view.h"
#include "gtk_builder.h"
#include "world_tree.h"
#include "menu.h"

struct graph_view
{
  struct list_head siblings;
  char * name;
  graph_canvas_handle graph_canvas;
  graph_proxy_handle graph;
  GtkWidget * canvas_widget;
  ladish_app_supervisor_proxy_handle app_supervisor;
};

struct list_head g_views;

GtkScrolledWindow * g_main_scrolledwin;
static struct graph_view * g_current_view;
GtkWidget * g_view_label;

const char * g_view_label_text =
  "If you've started ladish for the first time, you should:\n\n"
  " 1. Create a new studio (in the menu, Studio -> New Studio)\n"
  " 2. Configure JACK (in the menu, Tools -> Configure JACK)\n"
  " 3. Start the studio (in the menu, Studio -> Start Studio)\n"
  " 4. Start apps (in the menu, Application -> Run)\n"
  " 5. Connect their ports by click & drag on canvas\n"
  " 6. Save the studio (in the menu, Studio -> Save Studio)\n";

void view_init(void)
{
  g_main_scrolledwin = GTK_SCROLLED_WINDOW(get_gtk_builder_widget("main_scrolledwin"));
  INIT_LIST_HEAD(&g_views);

  g_view_label = gtk_label_new(g_view_label_text);
  g_object_ref(g_view_label);
  gtk_widget_show(g_view_label);

  g_current_view = NULL;
  gtk_scrolled_window_add_with_viewport(g_main_scrolledwin, g_view_label);
}

static void app_added(void * context, uint64_t id, const char * name, bool running, bool terminal, uint8_t level)
{
  //log_info("app added. id=%"PRIu64", name='%s', %srunning, %s, level %u", id, name, running ? "" : "not ", terminal ? "terminal" : "shell", (unsigned int)level);
  world_tree_add_app(context, id, name, running, terminal, level);
}

static void app_state_changed(void * context, uint64_t id, const char * name, bool running, bool terminal, uint8_t level)
{
  //log_info("app state changed. id=%"PRIu64", name='%s', %srunning, %s, level %u", id, name, running ? "" : "not ", terminal ? "terminal" : "shell", (unsigned int)level);
  world_tree_app_state_changed(context, id, name, running, terminal, level);
}

static void app_removed(void * context, uint64_t id)
{
  //log_info("app removed. id=%"PRIu64, id);
  world_tree_remove_app(context, id);
}

bool
create_view(
  const char * name,
  const char * service,
  const char * object,
  bool graph_dict_supported,
  bool app_supervisor_supported,
  bool force_activate,
  graph_view_handle * handle_ptr)
{
  struct graph_view * view_ptr;

  view_ptr = malloc(sizeof(struct graph_view));
  if (view_ptr == NULL)
  {
    log_error("malloc() failed for struct graph_view");
    goto fail;
  }

  view_ptr->name = strdup(name);
  if (view_ptr->name == NULL)
  {
    log_error("strdup() failed for \"%s\"", name);
    goto free_view;
  }

  view_ptr->app_supervisor = NULL;

  if (!graph_proxy_create(service, object, graph_dict_supported, &view_ptr->graph))
  {
    goto free_name;
  }

  if (!graph_canvas_create(1600 * 2, 1200 * 2, &view_ptr->graph_canvas))
  {
    goto destroy_graph;
  }

  if (!graph_canvas_attach(view_ptr->graph_canvas, view_ptr->graph))
  {
    goto destroy_graph_canvas;
  }

  view_ptr->canvas_widget = canvas_get_widget(graph_canvas_get_canvas(view_ptr->graph_canvas));

  list_add_tail(&view_ptr->siblings, &g_views);

  gtk_widget_show(view_ptr->canvas_widget);

  world_tree_add((graph_view_handle)view_ptr, force_activate);

  if (app_supervisor_supported)
  {
    if (!ladish_app_supervisor_proxy_create(service, object, view_ptr, app_added, app_state_changed, app_removed, &view_ptr->app_supervisor))
    {
      goto detach_graph_canvas;
    }
  }

  if (!graph_proxy_activate(view_ptr->graph))
  {
    goto free_app_supervisor;
  }

  *handle_ptr = (graph_view_handle)view_ptr;

  return true;

free_app_supervisor:
  if (view_ptr->app_supervisor != NULL)
  {
    ladish_app_supervisor_proxy_destroy(view_ptr->app_supervisor);
  }
detach_graph_canvas:
  graph_canvas_detach(view_ptr->graph_canvas);
destroy_graph_canvas:
  graph_canvas_destroy(view_ptr->graph_canvas);
destroy_graph:
  graph_proxy_destroy(view_ptr->graph);
free_name:
  free(view_ptr->name);
free_view:
  free(view_ptr);
fail:
  return false;
}

static void attach_canvas(struct graph_view * view_ptr)
{
  GtkWidget * child;

  child = gtk_bin_get_child(GTK_BIN(g_main_scrolledwin));

  if (child == view_ptr->canvas_widget)
  {
    return;
  }

  if (child != NULL)
  {
    gtk_container_remove(GTK_CONTAINER(g_main_scrolledwin), child);
  }

  g_current_view = view_ptr;
  gtk_container_add(GTK_CONTAINER(g_main_scrolledwin), view_ptr->canvas_widget);

  //_main_scrolledwin->property_hadjustment().get_value()->set_step_increment(10);
  //_main_scrolledwin->property_vadjustment().get_value()->set_step_increment(10);
}

static void detach_canvas(struct graph_view * view_ptr)
{
  GtkWidget * child;

  child = gtk_bin_get_child(GTK_BIN(g_main_scrolledwin));
  if (child == view_ptr->canvas_widget)
  {
    gtk_container_remove(GTK_CONTAINER(g_main_scrolledwin), view_ptr->canvas_widget);
    g_current_view = NULL;
    gtk_widget_show(g_view_label);
    gtk_scrolled_window_add_with_viewport(g_main_scrolledwin, g_view_label);
  }
}

#define view_ptr ((struct graph_view *)view)

void destroy_view(graph_view_handle view)
{
  list_del(&view_ptr->siblings);
  if (!list_empty(&g_views))
  {
    world_tree_activate((graph_view_handle)list_entry(g_views.next, struct graph_view, siblings));
  }
  else
  {
    set_main_window_title(NULL);
  }

  detach_canvas(view_ptr);

  world_tree_remove(view);

  graph_canvas_detach(view_ptr->graph_canvas);
  graph_canvas_destroy(view_ptr->graph_canvas);
  graph_proxy_destroy(view_ptr->graph);

  if (view_ptr->app_supervisor != NULL)
  {
    ladish_app_supervisor_proxy_destroy(view_ptr->app_supervisor);
  }

  free(view_ptr->name);
  free(view_ptr);
}

void activate_view(graph_view_handle view)
{
  attach_canvas(view_ptr);
  set_main_window_title(view);
  menu_view_activated(is_room_view(view));
}

const char * get_view_name(graph_view_handle view)
{
  return view_ptr->name;
}

const char * get_view_opath(graph_view_handle view)
{
  return graph_proxy_get_object(view_ptr->graph);
}

bool set_view_name(graph_view_handle view, const char * cname)
{
  char * name;

  name = strdup(cname);
  if (name == NULL)
  {
    log_error("strdup() failed for \"%s\"", name);
    return false;
  }

  free(view_ptr->name);
  view_ptr->name = name;

  world_tree_name_changed(view);

  if (g_current_view == view_ptr)
  {
    set_main_window_title(view);
  }

  return true;
}

canvas_handle get_current_canvas(void)
{
  if (g_current_view == NULL)
  {
    return NULL;
  }

  return graph_canvas_get_canvas(g_current_view->graph_canvas);
}

const char * get_current_view_room_name(void)
{
  if (g_current_view == NULL || !is_room_view((graph_view_handle)g_current_view))
  {
    return NULL;
  }

  return g_current_view->name;
}

graph_view_handle get_current_view(void)
{
  return (graph_view_handle)g_current_view;
}

bool is_room_view(graph_view_handle view)
{
  return strcmp(graph_proxy_get_object(view_ptr->graph), STUDIO_OBJECT_PATH) != 0;
}

bool app_run_custom(graph_view_handle view, const char * command, const char * name, bool run_in_terminal, uint8_t level)
{
  return ladish_app_supervisor_proxy_run_custom(view_ptr->app_supervisor, command, name, run_in_terminal, level);
}

ladish_app_supervisor_proxy_handle graph_view_get_app_supervisor(graph_view_handle view)
{
  return view_ptr->app_supervisor;
}
