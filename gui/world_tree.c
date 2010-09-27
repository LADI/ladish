/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the project_list class
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
#include "world_tree.h"
#include "gtk_builder.h"
#include "../common/catdup.h"
#include "menu.h"

enum entry_type
{
  entry_type_view,
  entry_type_app
};

enum
{
  COL_TYPE = 0,
  COL_NAME,
  COL_VIEW,
  COL_ID,
  COL_RUNNING,
  COL_TERMINAL,
  COL_LEVEL,
  NUM_COLS
};

GtkWidget * g_world_tree_widget;
GtkTreeStore * g_treestore;

bool get_app_view(GtkTreeIter * app_iter_ptr, graph_view_handle * view_ptr)
{
  GtkTreeIter view_iter;
  gint type;

  if (!gtk_tree_model_iter_parent(GTK_TREE_MODEL(g_treestore), &view_iter, app_iter_ptr))
  {
    ASSERT_NO_PASS;
    return false;
  }

  gtk_tree_model_get(
    GTK_TREE_MODEL(g_treestore),
    &view_iter,
    COL_TYPE, &type,
    COL_VIEW, view_ptr,
    -1);
  if (type != entry_type_view)
  {
    ASSERT_NO_PASS;
    return false;
  }

  return true;
}

static
gboolean
on_select(
  GtkTreeSelection * selection,
  GtkTreeModel * model,
  GtkTreePath * path,
  gboolean path_currently_selected,
  gpointer data)
{
  GtkTreeIter iter;
  graph_view_handle view;
  gint type;
  uint64_t id;

  if (gtk_tree_model_get_iter(model, &iter, path))
  {
    gtk_tree_model_get(model, &iter, COL_TYPE, &type, COL_VIEW, &view, COL_ID, &id, -1);
    switch (type)
    {
    case entry_type_app:
      if (!get_app_view(&iter, &view))
      {
        ASSERT_NO_PASS;
        break;
      }
    case entry_type_view:
      //log_info("%s is going to be %s.", get_view_name(view), path_currently_selected ? "unselected" : "selected");
      if (!path_currently_selected)
      {
        activate_view(view);
      }
      break;
    }
  }

  return TRUE;
}

bool get_selected_app_id(graph_view_handle * view_ptr, uint64_t * id_ptr)
{
  GtkTreeSelection * selection;
  GtkTreeIter app_iter;
  gint type;
  uint64_t id;
  graph_view_handle view;

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_world_tree_widget));
  if (!gtk_tree_selection_get_selected(selection, NULL, &app_iter))
  {
    return false;
  }

  gtk_tree_model_get(
    GTK_TREE_MODEL(g_treestore),
    &app_iter,
    COL_TYPE, &type,
    COL_ID, &id,
    -1);
  if (type != entry_type_app)
  {
    return false;
  }

  if (!get_app_view(&app_iter, &view))
  {
    ASSERT_NO_PASS;
    return false;
  }

  *view_ptr = view;
  *id_ptr = id;

  return true;
}

void on_popup_menu_action_app_start(GtkWidget * menuitem, gpointer userdata)
{
  uint64_t id;
  ladish_app_supervisor_proxy_handle proxy;
  graph_view_handle view;

  if (!get_selected_app_id(&view, &id))
  {
    return;
  }

  log_info("start app %"PRIu64, id);

  proxy = graph_view_get_app_supervisor(view);
  ladish_app_supervisor_proxy_start_app(proxy, id);
}

void on_popup_menu_action_app_stop(GtkWidget * menuitem, gpointer userdata)
{
  uint64_t id;
  ladish_app_supervisor_proxy_handle proxy;
  graph_view_handle view;

  if (!get_selected_app_id(&view, &id))
  {
    return;
  }

  log_info("stop app %"PRIu64, id);

  proxy = graph_view_get_app_supervisor(view);
  ladish_app_supervisor_proxy_stop_app(proxy, id);
}

void on_popup_menu_action_app_kill(GtkWidget * menuitem, gpointer userdata)
{
  uint64_t id;
  ladish_app_supervisor_proxy_handle proxy;
  graph_view_handle view;

  if (!get_selected_app_id(&view, &id))
  {
    return;
  }

  log_info("kill app %"PRIu64, id);

  proxy = graph_view_get_app_supervisor(view);
  ladish_app_supervisor_proxy_kill_app(proxy, id);
}

void on_popup_menu_action_app_remove(GtkWidget * menuitem, gpointer userdata)
{
  uint64_t id;
  ladish_app_supervisor_proxy_handle proxy;
  graph_view_handle view;

  if (!get_selected_app_id(&view, &id))
  {
    return;
  }

  log_info("remove app %"PRIu64, id);

  proxy = graph_view_get_app_supervisor(view);
  ladish_app_supervisor_proxy_remove_app(proxy, id);
}

void on_popup_menu_action_app_properties(GtkWidget * menuitem, gpointer userdata)
{
  uint64_t id;
  ladish_app_supervisor_proxy_handle proxy;
  graph_view_handle view;
  char * name;
  char * command;
  bool running;
  bool terminal;
  uint8_t level;
  guint result;
  GtkEntry * command_entry = GTK_ENTRY(get_gtk_builder_widget("app_command_entry"));
  GtkEntry * name_entry = GTK_ENTRY(get_gtk_builder_widget("app_name_entry"));
  GtkToggleButton * terminal_button = GTK_TOGGLE_BUTTON(get_gtk_builder_widget("app_terminal_check_button"));
  GtkToggleButton * level0_button = GTK_TOGGLE_BUTTON(get_gtk_builder_widget("app_level0"));
  GtkToggleButton * level1_button = GTK_TOGGLE_BUTTON(get_gtk_builder_widget("app_level1"));
  GtkToggleButton * level2_button = GTK_TOGGLE_BUTTON(get_gtk_builder_widget("app_level2"));
  GtkToggleButton * level3_button = GTK_TOGGLE_BUTTON(get_gtk_builder_widget("app_level3"));

  if (!get_selected_app_id(&view, &id))
  {
    return;
  }

  log_info("app %"PRIu64" properties", id);

  proxy = graph_view_get_app_supervisor(view);

  if (!ladish_app_supervisor_get_app_properties(proxy, id, &name, &command, &running, &terminal, &level))
  {
    error_message_box("Cannot get app properties");
    return;
  }

  log_info("'%s':'%s' %s level %"PRIu8, name, command, terminal ? "terminal" : "shell", level);

  gtk_entry_set_text(name_entry, name);
  gtk_entry_set_text(command_entry, command);
  gtk_toggle_button_set_active(terminal_button, terminal);

  gtk_widget_set_sensitive(GTK_WIDGET(command_entry), !running);
  gtk_widget_set_sensitive(GTK_WIDGET(terminal_button), !running);
  gtk_widget_set_sensitive(GTK_WIDGET(level0_button), !running);
  gtk_widget_set_sensitive(GTK_WIDGET(level1_button), !running);

  switch (level)
  {
  case 0:
    gtk_toggle_button_set_active(level0_button, TRUE);
    break;
  case 1:
    gtk_toggle_button_set_active(level1_button, TRUE);
    break;
  case 2:
    gtk_toggle_button_set_active(level2_button, TRUE);
    break;
  case 3:
    gtk_toggle_button_set_active(level3_button, TRUE);
    break;
  default:
    log_error("unknown level");
    ASSERT_NO_PASS;
    gtk_toggle_button_set_active(level0_button, TRUE);
  }

  free(name);
  free(command);

  gtk_window_set_focus(GTK_WINDOW(g_app_dialog), running ? GTK_WIDGET(name_entry) : GTK_WIDGET(command_entry));
  gtk_window_set_title(GTK_WINDOW(g_app_dialog), "App properties");

  gtk_widget_show(g_app_dialog);

  result = gtk_dialog_run(GTK_DIALOG(g_app_dialog));
  if (result == 2)
  {
    if (gtk_toggle_button_get_active(level0_button))
    {
      level = 0;
    }
    else if (gtk_toggle_button_get_active(level1_button))
    {
      level = 1;
    }
    else if (gtk_toggle_button_get_active(level2_button))
    {
      level = 2;
    }
    else if (gtk_toggle_button_get_active(level3_button))
    {
      level = 3;
    }
    else
    {
      log_error("unknown level");
      ASSERT_NO_PASS;
      level = 0;
    }

    log_info("'%s':'%s' %s level %"PRIu8, gtk_entry_get_text(name_entry), gtk_entry_get_text(command_entry), gtk_toggle_button_get_active(terminal_button) ? "terminal" : "shell", level);
    if (!ladish_app_supervisor_set_app_properties(proxy, id, gtk_entry_get_text(name_entry), gtk_entry_get_text(command_entry), gtk_toggle_button_get_active(terminal_button), level))
    {
      error_message_box("Cannot set app properties.");
    }
  }

  gtk_widget_hide(g_app_dialog);
}

void popup_menu(GtkWidget * treeview, GdkEventButton * event)
{
  GtkTreeSelection * selection;
  GtkTreeIter iter;
  GtkWidget * menu;
  GtkWidget * menuitem;
  gint type;
  graph_view_handle view;
  uint64_t id;
  gboolean running;
  gboolean terminal;
  uint8_t level;

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_world_tree_widget));
  if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
  {
    return;
  }

  gtk_tree_model_get(
    GTK_TREE_MODEL(g_treestore),
    &iter,
    COL_TYPE, &type,
    COL_VIEW, &view,
    COL_ID, &id,
    COL_RUNNING, &running,
    COL_TERMINAL, &terminal,
    COL_LEVEL, &level,
    -1);

  menu = gtk_menu_new();

  if (type == entry_type_app)
  {
    if (running)
    {
      menuitem = gtk_menu_item_new_with_label("Stop");
      g_signal_connect(menuitem, "activate", (GCallback)on_popup_menu_action_app_stop, NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

      menuitem = gtk_menu_item_new_with_label("Kill");
      g_signal_connect(menuitem, "activate", (GCallback)on_popup_menu_action_app_kill, NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    }
    else
    {
      menuitem = gtk_menu_item_new_with_label("Start");
      g_signal_connect(menuitem, "activate", (GCallback)on_popup_menu_action_app_start, NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    }

    menuitem = gtk_menu_item_new_with_label("Properties");
    g_signal_connect(menuitem, "activate", (GCallback)on_popup_menu_action_app_properties, NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_separator_menu_item_new(); /* separator */
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    menuitem = gtk_menu_item_new_with_label("Remove");
    g_signal_connect(menuitem, "activate", (GCallback)on_popup_menu_action_app_remove, NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  }
  else if (type == entry_type_view)
  {
    fill_view_popup_menu(GTK_MENU(menu), view);
  }
  else
  {
    g_object_ref(menu);
    g_object_unref(menu);
    return;
  }

  gtk_widget_show_all(menu);

  /* Note: event can be NULL here when called from view_onPopupMenu;
   *  gdk_event_get_time() accepts a NULL argument */
  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, (event != NULL) ? event->button : 0, gdk_event_get_time((GdkEvent *)event));
}

gboolean on_button_pressed(GtkWidget * treeview, GdkEventButton * event, gpointer userdata)
{
  /* single click with the right mouse button? */
  if (event->type == GDK_BUTTON_PRESS && event->button == 3)
  {
    /* select row if no row is selected or only one other row is selected */
    {
      GtkTreeSelection *selection;

      selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

      /* Note: gtk_tree_selection_count_selected_rows() does not exist in gtk+-2.0, only in gtk+ >= v2.2 ! */
      if (gtk_tree_selection_count_selected_rows(selection) <= 1)
      {
        GtkTreePath *path;

        /* Get tree path for row that was clicked */
        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview), (gint)event->x, (gint)event->y, &path, NULL, NULL, NULL))
        {
          gtk_tree_selection_unselect_all(selection);
          gtk_tree_selection_select_path(selection, path);
          gtk_tree_path_free(path);
        }
      }
    }

    popup_menu(treeview, event);

    return TRUE; /* we handled this */
  }

  return FALSE; /* we did not handle this */
}

gboolean on_popup_menu(GtkWidget * treeview, gpointer userdata)
{
  popup_menu(treeview, NULL);
  return TRUE; /* we handled this */
}

static void on_row_activated(GtkTreeView * treeview, GtkTreePath * path, GtkTreeViewColumn * col, gpointer userdata)
{
  GtkTreeIter iter;
  gint type;
  graph_view_handle view;
  uint64_t id;
  gboolean running;
  ladish_app_supervisor_proxy_handle proxy;

  if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(g_treestore), &iter, path))
  {
    return;
  }

  gtk_tree_model_get(GTK_TREE_MODEL(g_treestore), &iter, COL_TYPE, &type, COL_ID, &id, COL_RUNNING, &running, -1);
  if (type == entry_type_app)
  {
    log_info("%s app %"PRIu64, running ? "stop" : "start", id);

    if (!get_app_view(&iter, &view))
    {
      ASSERT_NO_PASS;
      return;
    }

    proxy = graph_view_get_app_supervisor(view);
    (running ? ladish_app_supervisor_proxy_stop_app : ladish_app_supervisor_proxy_start_app)(proxy, id);
  }
}

void world_tree_init(void)
{
  GtkTreeViewColumn * col;
  GtkCellRenderer * renderer;
  GtkTreeSelection * selection;

  g_world_tree_widget = get_gtk_builder_widget("world_tree");
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(g_world_tree_widget), FALSE);

  col = gtk_tree_view_column_new();
  gtk_tree_view_column_set_title(col, "Name");
  gtk_tree_view_append_column(GTK_TREE_VIEW(g_world_tree_widget), col);
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text", COL_NAME);

  g_treestore = gtk_tree_store_new(
    NUM_COLS,
    G_TYPE_INT,
    G_TYPE_STRING,
    G_TYPE_POINTER,
    G_TYPE_UINT64,
    G_TYPE_BOOLEAN,
    G_TYPE_BOOLEAN,
    G_TYPE_CHAR);
  gtk_tree_view_set_model(GTK_TREE_VIEW(g_world_tree_widget), GTK_TREE_MODEL(g_treestore));

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_world_tree_widget));
  gtk_tree_selection_set_select_function(selection, on_select, NULL, NULL);

  g_signal_connect(g_world_tree_widget, "button-press-event", (GCallback)on_button_pressed, NULL);
  g_signal_connect(g_world_tree_widget, "popup-menu", (GCallback)on_popup_menu, NULL);
  g_signal_connect(g_world_tree_widget, "row-activated", (GCallback)on_row_activated, NULL);
}

void world_tree_add(graph_view_handle view, bool force_activate)
{
  GtkTreeIter iter;

  gtk_tree_store_append(g_treestore, &iter, NULL);
  gtk_tree_store_set(g_treestore, &iter, COL_TYPE, entry_type_view, COL_VIEW, view, COL_NAME, get_view_name(view), -1);

  /* select the first top level item */
  if (force_activate || gtk_tree_model_iter_n_children(GTK_TREE_MODEL(g_treestore), NULL) == 1)
  {
    gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(g_world_tree_widget)), &iter);
  }
}

graph_view_handle world_tree_find_by_opath(const char * opath)
{
  gint type;
  graph_view_handle view;
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(g_treestore), &iter))
  {
    do
    {
      gtk_tree_model_get(GTK_TREE_MODEL(g_treestore), &iter, COL_TYPE, &type, COL_VIEW, &view, -1);
      if (type == entry_type_view)
      {
        if (strcmp(get_view_opath(view), opath) == 0)
        {
          return view;
        }
      }
    }
    while (gtk_tree_model_iter_next(GTK_TREE_MODEL(g_treestore), &iter));
  }

  return NULL;
}

static bool find_view(graph_view_handle view, GtkTreeIter * iter_ptr)
{
  gint type;
  graph_view_handle view2;

  if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(g_treestore), iter_ptr))
  {
    do
    {
      gtk_tree_model_get(GTK_TREE_MODEL(g_treestore), iter_ptr, COL_TYPE, &type, COL_VIEW, &view2, -1);
      if (type == entry_type_view && view == view2)
      {
        return true;
      }
    }
    while (gtk_tree_model_iter_next(GTK_TREE_MODEL(g_treestore), iter_ptr));
  }

  return false;
}

static bool find_app(graph_view_handle view, uint64_t id, GtkTreeIter * view_iter_ptr, GtkTreeIter * app_iter_ptr)
{
  gint type;
  uint64_t id2;

  if (!find_view(view, view_iter_ptr))
  {
    return false;
  }

  if (!gtk_tree_model_iter_children(GTK_TREE_MODEL(g_treestore), app_iter_ptr, view_iter_ptr))
  {
    return false;
  }

  do
  {
    gtk_tree_model_get(GTK_TREE_MODEL(g_treestore), app_iter_ptr, COL_TYPE, &type, COL_ID, &id2, -1);
    if (type == entry_type_app && id == id2)
    {
      return true;
    }
  }
  while (gtk_tree_model_iter_next(GTK_TREE_MODEL(g_treestore), app_iter_ptr));

  return false;
}

void world_tree_remove(graph_view_handle view)
{
  GtkTreeIter iter;

  if (find_view(view, &iter))
  {
    gtk_tree_store_remove(g_treestore, &iter);
  }
}

void world_tree_activate(graph_view_handle view)
{
  GtkTreeIter iter;

  if (find_view(view, &iter))
  {
    gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(g_world_tree_widget)), &iter);
  }
}

void world_tree_name_changed(graph_view_handle view)
{
  GtkTreeIter iter;

  if (find_view(view, &iter))
  {
    gtk_tree_store_set(g_treestore, &iter, COL_NAME, get_view_name(view), -1);
  }
}

static char * get_app_name_string(const char * app_name, bool running, bool terminal, uint8_t level)
{
  char * app_name_with_status;
  const char * level_string;

  switch (level)
  {
  case 0:
    level_string = "[L0]";
    break;
  case 1:
    level_string = "[L1]";
    break;
  default:
    level_string = "[L?]";
  }

  app_name_with_status = catdup3(level_string, running ? " " : " (inactive) ", app_name);
  if (app_name_with_status == NULL)
  {
    log_error("catdup failed for app name");
    return NULL;
  }

  return app_name_with_status;
}

void world_tree_add_app(graph_view_handle view, uint64_t id, const char * app_name, bool running, bool terminal, uint8_t level)
{
  GtkTreeIter iter;
  const char * view_name;
  GtkTreeIter child;
  GtkTreePath * path;
  char * app_name_with_status;

  if (!find_view(view, &iter))
  {
    ASSERT_NO_PASS;
    return;
  }

  path = gtk_tree_model_get_path(GTK_TREE_MODEL(g_treestore), &iter);

  gtk_tree_model_get(GTK_TREE_MODEL(g_treestore), &iter, COL_NAME, &view_name, -1);

  log_info("adding app '%s' to '%s'", app_name, view_name);

  app_name_with_status = get_app_name_string(app_name, running, terminal, level);
  if (app_name_with_status == NULL)
  {
    goto free_path;
  }

  gtk_tree_store_append(g_treestore, &child, &iter);
  gtk_tree_store_set(
    g_treestore,
    &child,
    COL_TYPE, entry_type_app,
    COL_NAME, app_name_with_status,
    COL_ID, id,
    COL_RUNNING, running,
    COL_TERMINAL, terminal,
    COL_LEVEL, level,
    -1);
  gtk_tree_view_expand_row(GTK_TREE_VIEW(g_world_tree_widget), path, false);

  free(app_name_with_status);
free_path:
  gtk_tree_path_free(path);
}

void world_tree_app_state_changed(graph_view_handle view, uint64_t id, const char * app_name, bool running, bool terminal, uint8_t level)
{
  GtkTreeIter view_iter;
  GtkTreeIter app_iter;
  const char * view_name;
  GtkTreePath * path;
  char * app_name_with_status;

  if (!find_app(view, id, &view_iter, &app_iter))
  {
    log_error("removed app not found");
    return;
  }

  path = gtk_tree_model_get_path(GTK_TREE_MODEL(g_treestore), &view_iter);

  gtk_tree_model_get(GTK_TREE_MODEL(g_treestore), &view_iter, COL_NAME, &view_name, -1);

  log_info("changing app state '%s':'%s'", view_name, app_name);

  app_name_with_status = get_app_name_string(app_name, running, terminal, level);
  if (app_name_with_status == NULL)
  {
    goto free_path;
  }

  gtk_tree_view_expand_row(GTK_TREE_VIEW(g_world_tree_widget), path, false);
  gtk_tree_store_set(
    g_treestore,
    &app_iter,
    COL_NAME, app_name_with_status,
    COL_RUNNING, running,
    COL_TERMINAL, terminal,
    COL_LEVEL, level,
    -1);

  free(app_name_with_status);
free_path:
  gtk_tree_path_free(path);
}

void world_tree_remove_app(graph_view_handle view, uint64_t id)
{
  GtkTreeIter view_iter;
  GtkTreeIter app_iter;
  const char * view_name;
  const char * app_name;
  GtkTreePath * path;

  if (!find_app(view, id, &view_iter, &app_iter))
  {
    log_error("removed app not found");
    return;
  }

  path = gtk_tree_model_get_path(GTK_TREE_MODEL(g_treestore), &view_iter);

  gtk_tree_model_get(GTK_TREE_MODEL(g_treestore), &view_iter, COL_NAME, &view_name, -1);
  gtk_tree_model_get(GTK_TREE_MODEL(g_treestore), &app_iter, COL_NAME, &app_name, -1);

  log_info("removing app '%s' from '%s'", app_name, view_name);

  gtk_tree_view_expand_row(GTK_TREE_VIEW(g_world_tree_widget), path, false);
  gtk_tree_store_remove(g_treestore, &app_iter);

  gtk_tree_path_free(path);
}

void world_tree_destroy_room_views(void)
{
  gint type;
  graph_view_handle view;
  GtkTreeIter iter;
  bool valid;

  if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(g_treestore), &iter))
  {
    return;
  }

loop:
  gtk_tree_model_get(GTK_TREE_MODEL(g_treestore), &iter, COL_TYPE, &type, COL_VIEW, &view, -1);
  if (type == entry_type_view && is_room_view(view))
  {
    //log_info("removing view for room %s", get_view_opath(view));
    valid = gtk_tree_store_remove(g_treestore, &iter);

    destroy_view(view);

    if (!valid)
    {
      /* no more entries */
      return;
    }

    goto loop;
  }

  if (gtk_tree_model_iter_next(GTK_TREE_MODEL(g_treestore), &iter))
  {
    goto loop;
  }
}
