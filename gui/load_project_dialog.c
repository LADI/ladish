/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the load project dialog
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

#include <sys/stat.h>

#include "load_project_dialog.h"
#include "gtk_builder.h"
#include "../common/catdup.h"

enum
{
  COL_NAME = 0,
  COL_MODIFIED,
  COL_DESCRIPTION,
  COL_MTIME,
  NUM_COLS
};

#if 0
static
void
convert_timestamp_to_string(
  const time_t timestamp,
  std::string& timestamp_string)
{
  GDate mtime, now;
  gint days_diff;
  struct tm tm_mtime;
  time_t time_now;
  const gchar *format;
  gchar buf[256];

  if (timestamp == 0)
  {
    timestamp_string = "Unknown";
    return;
  }

  localtime_r(&timestamp, &tm_mtime);

  g_date_set_time_t(&mtime, timestamp);
  time_now = time(NULL);
  g_date_set_time_t(&now, time_now);

  days_diff = g_date_get_julian(&now) - g_date_get_julian(&mtime);

  if (days_diff == 0)
  {
    format = "Today at %H:%M";
  }
  else if (days_diff == 1)
  {
    format = "Yesterday at %H:%M";
  }
  else
  {
    if (days_diff > 1 && days_diff < 7)
    {
      format = "%A"; /* Days from last week */
    }
    else
    {
      format = "%x"; /* Any other date */
    }
  }

  if (strftime(buf, sizeof(buf), format, &tm_mtime) != 0)
  {
    timestamp_string = buf;
  }
  else
  {
    timestamp_string = "Unknown";
  }
}
#endif

#if 0
static int mtime_sorter(GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b, gpointer user_data)
{
  uint64_t ta;
  uint64_t tb;

  gtk_tree_model_get(GTK_TREE_MODEL(user_data), a, COL_MTIME, &ta, -1);
  gtk_tree_model_get(GTK_TREE_MODEL(user_data), b, COL_MTIME, &tb, -1);

  return ta > tb ? -1 : (ta == tb ? 0 : 1);
}
#endif

static gboolean reject_filter(const GtkFileFilterInfo * filter_info, gpointer data)
{
  //log_info("filter: '%s'", filter_info->filename);
  return FALSE;
}

static bool is_project_dir(const char * dir)
{
  char * file;
  struct stat st;
  int ret;

  file = catdup(dir, "/ladish-project.xml");
  if (file == NULL)
  {
    log_error("catdup() failed to compose project file path");
    return false;
  }

  ret = stat(file, &st);
  free(file);

  return ret == 0;
}

#if 0
static void on_dir_select(GtkWidget * widget, gpointer data)
{
  char * dir;
  gboolean sensitive;

  //log_info("selection-changed signal");

  dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(data));
  sensitive = FALSE;
  if (dir != NULL)
  {
    sensitive = is_project_dir(dir);
    g_free(dir);
  }

  gtk_widget_set_sensitive(gtk_dialog_get_widget_for_response(GTK_DIALOG(data), GTK_RESPONSE_ACCEPT), sensitive);
}
#endif

static void dir_changed(GtkWidget * widget, gpointer data)
{
  char * dir;

  //log_info("current-folder-changed signal");

  dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(data));
  if (dir == NULL)
  {
    return;
  }

  //log_info("dir changed: '%s'", dir);

  if (is_project_dir(dir))
  {
    gtk_widget_activate(gtk_dialog_get_widget_for_response(GTK_DIALOG(data), GTK_RESPONSE_ACCEPT));
  }
}

//static void on_browse(GtkWidget * widget, gpointer data)
void ladish_run_load_project_dialog(ladish_room_proxy_handle room)
{
  GtkFileFilter * filter;
  GtkWidget * dialog;
  char * filename;

  dialog = gtk_file_chooser_dialog_new(
    "Load project",
    NULL,
    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
    NULL);

  gtk_file_chooser_set_create_folders(GTK_FILE_CHOOSER(dialog), FALSE);

  filter = gtk_file_filter_new();
  gtk_file_filter_add_custom(filter, GTK_FILE_FILTER_FILENAME, reject_filter, dialog, NULL); /* reject all files */
  gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
  //g_signal_connect(G_OBJECT(dialog), "selection-changed", G_CALLBACK(on_dir_select), dialog);
  g_signal_connect(G_OBJECT(dialog), "current-folder-changed", G_CALLBACK(dir_changed), dialog);

loop:
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
  {
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

    if (!is_project_dir(filename))
    {
      GtkWidget * dialog;
      dialog = get_gtk_builder_widget("error_dialog");
      gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog), "<b><big>Not a project dir</big></b>");
      gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(dialog), "%s", filename);
      gtk_widget_show(dialog);
      gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_hide(dialog);
      goto loop;
    }

    //gtk_entry_set_text(GTK_ENTRY(get_gtk_builder_widget("load_project_path_entry")), filename);
    log_info("Loading project from '%s'", filename);
    if (!ladish_room_proxy_load_project(room, filename))
    {
      log_error("ladish_room_proxy_load_project() failed.");
    }

    g_free(filename);

    //gtk_widget_activate(gtk_dialog_get_widget_for_response(GTK_DIALOG(data), GTK_RESPONSE_OK));
  }
  gtk_widget_destroy(dialog);
  return;
}

#if 0
void ladish_run_load_project_dialog(ladish_room_proxy_handle room)
{
  GtkWidget * dialog;
  GtkWidget * browse_button;
  GtkTreeView * view;
  GtkResponseType response;
  GtkListStore * store;
  GtkCellRenderer * renderer;
  GtkTreeSortable * sortable;
  gint colno;
  GtkTreeViewColumn * column;
  GtkEntry * path;

  dialog = get_gtk_builder_widget("load_project_dialog");
  browse_button = get_gtk_builder_widget("load_project_path_browse_button");
  path = GTK_ENTRY(get_gtk_builder_widget("load_project_path_entry"));
  view = GTK_TREE_VIEW(get_gtk_builder_widget("loadable_project_list"));

  store = GTK_LIST_STORE(gtk_tree_view_get_model(view));
  if (store == NULL)
  {
    store = gtk_list_store_new(NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64);
    sortable = GTK_TREE_SORTABLE(store);
    gtk_tree_view_set_model(view, GTK_TREE_MODEL(store));

    renderer = gtk_cell_renderer_text_new();

    colno = gtk_tree_view_insert_column_with_attributes(view, -1, "Project Name", renderer, NULL) - 1;
    column = gtk_tree_view_get_column(view, colno);
    gtk_tree_view_column_set_sort_column_id(column, COL_NAME);

    colno = gtk_tree_view_insert_column_with_attributes(view, -1, "Modified", renderer, NULL) - 1;
    column = gtk_tree_view_get_column(view, colno);
    gtk_tree_view_column_set_sort_column_id(column, COL_MODIFIED);
    gtk_tree_sortable_set_sort_func(sortable, colno, mtime_sorter, store, NULL);
    gtk_tree_sortable_set_sort_column_id(sortable, colno, GTK_SORT_ASCENDING);

    colno = gtk_tree_view_insert_column_with_attributes(view, -1, "Description", renderer, NULL) - 1;
    column = gtk_tree_view_get_column(view, colno);
    gtk_tree_view_column_set_sort_column_id(column, COL_DESCRIPTION);
  }

  //log_info("store = %p", store);

  /* Gtk::TreeModel::Row row; */
  /* int result; */

  /* for (std::list<lash_project_info>::iterator iter = projects.begin(); iter != projects.end(); iter++) */
  /* { */
  /*   std::string str; */
  /*   row = *(_model->append()); */
  /*   row[_columns.name] = iter->name; */
  /*   convert_timestamp_to_string(iter->modification_time, str); */
  /*   row[_columns.modified] = str; */
  /*   row[_columns.description] = iter->description; */
  /*   row[_columns.mtime] = iter->modification_time; */
  /* } */

  /* _widget->signal_button_press_event().connect(sigc::mem_fun(*this, &LoadProjectDialog::on_button_press_event), false); */
  /* _widget->signal_key_press_event().connect(sigc::mem_fun(*this, &LoadProjectDialog::on_key_press_event), false); */

  g_signal_connect(G_OBJECT(browse_button), "clicked", G_CALLBACK(on_browse), dialog);

  gtk_entry_set_text(path, "");

  gtk_widget_show(dialog);
  response = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_hide(dialog);
  if (response == GTK_RESPONSE_OK)
  {
    log_info("Loading project from '%s'", gtk_entry_get_text(path));
    if (!ladish_room_proxy_load_project(room, gtk_entry_get_text(path)))
    {
      log_error("ladish_room_proxy_load_project() failed.");
    }
  }
}
#endif
