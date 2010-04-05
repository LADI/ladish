/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains implementation of the "create room" dialog
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

#include <gdk/gdkkeysyms.h>

#include "create_room_dialog.h"
#include "gtk_builder.h"
#include "../proxies/control_proxy.h"

enum
{
  COL_NAME = 0,
  NUM_COLS
};

static GtkWidget * g_dialog;
static GtkListStore * g_liststore;
static GtkTreeView * g_treeview;

bool on_key_press_event(GtkWidget * widget_ptr, GdkEventKey * event_ptr, gpointer user_data)
{
  if (event_ptr->type == GDK_KEY_PRESS &&
      (event_ptr->keyval == GDK_Return ||
       event_ptr->keyval == GDK_KP_Enter))
  {
    gtk_dialog_response(GTK_DIALOG(g_dialog), 2);
    return true;
  }

  return false;
}

static void on_row_activated(GtkTreeView * treeview, GtkTreePath * path, GtkTreeViewColumn * col, gpointer userdata)
{
  gtk_dialog_response(GTK_DIALOG(g_dialog), 2);
}

void create_room_dialog_init(void)
{
  GtkTreeViewColumn * col;
  GtkCellRenderer * renderer;

  g_dialog = get_gtk_builder_widget("create_room_dialog");
  g_treeview = GTK_TREE_VIEW(get_gtk_builder_widget("room_templates_treeview"));

  gtk_tree_view_set_headers_visible(g_treeview, FALSE);

  col = gtk_tree_view_column_new();
  gtk_tree_view_column_set_title(col, "Name");
  gtk_tree_view_append_column(g_treeview, col);

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text", COL_NAME);

  g_liststore = gtk_list_store_new(NUM_COLS, G_TYPE_STRING);
  gtk_tree_view_set_model(g_treeview, GTK_TREE_MODEL(g_liststore));

  g_signal_connect(G_OBJECT(g_treeview), "key-press-event", G_CALLBACK(on_key_press_event), NULL);
  g_signal_connect(G_OBJECT(g_treeview), "row-activated", G_CALLBACK(on_row_activated), NULL);
}

void create_room_dialog_uninit(void)
{
  g_object_unref(g_liststore);
}

static void fill_templates_callback(void * context, const char * template_name)
{
  GtkTreeIter iter;

  gtk_list_store_append(g_liststore, &iter);
  gtk_list_store_set(g_liststore, &iter, COL_NAME, template_name, -1);

  /* select the first item */
  if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(g_liststore), NULL) == 1)
  {
    gtk_tree_selection_select_iter(gtk_tree_view_get_selection(g_treeview), &iter);
  }
}

static bool fill_templates(void)
{
  gtk_list_store_clear(g_liststore);
  return control_proxy_get_room_template_list(fill_templates_callback, NULL);
}

bool create_room_dialog_run(char ** name_ptr_ptr, char ** template_ptr_ptr)
{
  guint result;
  bool ok;
  GtkWidget * dialog = get_gtk_builder_widget("create_room_dialog");
  GtkEntry * entry = GTK_ENTRY(get_gtk_builder_widget("create_room_name_entry"));
  GtkTreeSelection * selection;
  GtkTreeIter iter;
  char * template;
  char * name;

  /* text-select old name so it is cleared if user starts typing
   * but keep it so user can use slighly modified name */
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
  gtk_window_set_focus(GTK_WINDOW(dialog), GTK_WIDGET(entry));

  if (!fill_templates())
  {
    log_error("Cannot fetch room template list");
    return false;
  }

  gtk_widget_show(dialog);

again:
  result = gtk_dialog_run(GTK_DIALOG(dialog));
  ok = result == 2;
  if (ok)
  {
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_treeview));
    if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
    {
      goto again;
    }

    gtk_tree_model_get(GTK_TREE_MODEL(g_liststore), &iter, COL_NAME, &template, -1);
    template = strdup(template);
    if (template == NULL)
    {
      log_error("strdup failed for template name (create room dialog)");
      ok = false;
      goto exit;
    }

    name = (char *)gtk_entry_get_text(entry);
    if (strlen(name) == 0)
    {
      goto again;
    }

    name = strdup(name);
    if (name == NULL)
    {
      log_error("strdup failed for room name (create room dialog)");
      free(template);
      ok = false;
      goto exit;
    }

    *name_ptr_ptr = name;
    *template_ptr_ptr = template;
  }

exit:
  gtk_widget_hide(dialog);

  return ok;
}
