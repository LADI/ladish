/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
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
#include "glade.h"
#include "world_tree.h"

struct graph_view
{
  struct list_head siblings;
  char * name;
  graph_canvas_handle graph_canvas;
  graph_proxy_handle graph;
  GtkWidget * canvas_widget;
};

struct list_head g_views;

GtkScrolledWindow * g_main_scrolledwin;
static struct graph_view * g_current_view;

void view_init(void)
{
  g_main_scrolledwin = GTK_SCROLLED_WINDOW(get_glade_widget("main_scrolledwin"));
  INIT_LIST_HEAD(&g_views);
  g_current_view = NULL;
}

bool
create_view(
  const char * name,
  const char * service,
  const char * object,
  bool graph_dict_supported,
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

  if (!graph_proxy_activate(view_ptr->graph))
  {
    goto detach_graph_canvas;
  }

  view_ptr->canvas_widget = canvas_get_widget(graph_canvas_get_canvas(view_ptr->graph_canvas));

  list_add_tail(&view_ptr->siblings, &g_views);

  gtk_widget_show(view_ptr->canvas_widget);

  world_tree_add((graph_view_handle)view_ptr, force_activate);

  *handle_ptr = (graph_view_handle)view_ptr;

  return true;

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
  free(view_ptr->name);
  free(view_ptr);
}

void activate_view(graph_view_handle view)
{
  attach_canvas(view_ptr);
  set_main_window_title(view);
}

const char * get_view_name(graph_view_handle view)
{
  return view_ptr->name;
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

canvas_handle get_current_canvas()
{
  if (g_current_view == NULL)
  {
    return NULL;
  }

  return graph_canvas_get_canvas(g_current_view->graph_canvas);
}
