/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008, 2009 Nedko Arnaudov <nedko@arnaudov.name>
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
#include "glade.h"

#if 0
#include <iostream>
#include <gtkmm.h>
#include <libglademm/xml.h>

#include "project_list.hpp"
#include "Widget.hpp"
#include "Patchage.hpp"
#include "session.hpp"
#include "project.hpp"
#include "project_properties.hpp"
#include "lash_client.hpp"
#include "globals.hpp"

struct project_list_column_record : public Gtk::TreeModel::ColumnRecord
{
  Gtk::TreeModelColumn<Glib::ustring> name;
  Gtk::TreeModelColumn<boost::shared_ptr<project> > project_ptr;
  Gtk::TreeModelColumn<boost::shared_ptr<lash_client> > client_ptr;
};

struct project_list_impl : public sigc::trackable
{
  Patchage *_app;
  Widget<Gtk::TreeView> _widget;
  project_list_column_record _columns;
  Glib::RefPtr<Gtk::TreeStore> _model;
  Gtk::Menu _menu_popup;

  project_list_impl(
    Glib::RefPtr<Gnome::Glade::Xml> xml,
    Patchage * app);

  void project_added(boost::shared_ptr<project> project_ptr);
  void project_closed(boost::shared_ptr<project> project_ptr);
  void project_renamed(Gtk::TreeModel::iterator iter);
  void client_added(boost::shared_ptr<lash_client> client_ptr, Gtk::TreeModel::iterator iter);
  void client_removed(boost::shared_ptr<lash_client> client_ptr, Gtk::TreeModel::iterator iter);
  void client_renamed(Gtk::TreeModel::iterator iter);

  bool on_button_press_event(GdkEventButton * event);

  void on_menu_popup_load_project();
  void on_menu_popup_save_all_projects();
  void on_menu_popup_save_project(boost::shared_ptr<project> project_ptr);
  void on_menu_popup_close_project(boost::shared_ptr<project> project_ptr);
  void on_menu_popup_project_properties(boost::shared_ptr<project> project_ptr);
  void on_menu_popup_close_all_projects();
};

project_list::project_list(
  Patchage * app,
  session * session_ptr)
{
  _impl_ptr = new project_list_impl(g_xml, app);
  session_ptr->_signal_project_added.connect(mem_fun(_impl_ptr, &project_list_impl::project_added));
  session_ptr->_signal_project_closed.connect(mem_fun(_impl_ptr, &project_list_impl::project_closed));
}

project_list::~project_list()
{
  delete _impl_ptr;
}

project_list_impl::project_list_impl(
  Glib::RefPtr<Gnome::Glade::Xml> xml,
  Patchage * app)
  : _app(app)
{
  _widget.init(xml, "projects_list");

  _columns.add(_columns.name);
  _columns.add(_columns.project_ptr);
  _columns.add(_columns.client_ptr);

  _model = Gtk::TreeStore::create(_columns);
  _widget->set_model(_model);

  _widget->append_column("LASH projects", _columns.name);

  _menu_popup.accelerate(*_widget);

  _widget->signal_button_press_event().connect(sigc::mem_fun(*this, &project_list_impl::on_button_press_event), false);
}

bool
project_list_impl::on_button_press_event(GdkEventButton * event_ptr)
{
  // Then do our custom stuff:
  if (event_ptr->type == GDK_BUTTON_PRESS && event_ptr->button == 3)
  {
    Glib::RefPtr<Gtk::TreeView::Selection> selection = _widget->get_selection();

    Gtk::TreeModel::Path path;
    Gtk::TreeViewColumn * column_ptr;
    int cell_x;
    int cell_y;

    Gtk::Menu::MenuList& menulist = _menu_popup.items();

    menulist.clear();

    menulist.push_back(Gtk::Menu_Helpers::MenuElem("_Load project...", sigc::mem_fun(*this, &project_list_impl::on_menu_popup_load_project)));

    menulist.push_back(Gtk::Menu_Helpers::MenuElem("Save _all projects", sigc::mem_fun(*this, &project_list_impl::on_menu_popup_save_all_projects)));

    if (_widget->get_path_at_pos((int)event_ptr->x, (int)event_ptr->y, path, column_ptr, cell_x, cell_y))
    {
      //cout << path.to_string() << endl;
      selection->unselect_all();
      selection->select(path);

      Gtk::TreeIter iter = selection->get_selected();
      boost::shared_ptr<project> project_ptr = (*iter)[_columns.project_ptr];
      std::string name;

      if (project_ptr)
      {
        project_ptr->get_name(name);

        menulist.push_back(
          Gtk::Menu_Helpers::MenuElem(
            (std::string)"_Save project '" + name + "'",
            sigc::bind(
              sigc::mem_fun(
                *this,
                &project_list_impl::on_menu_popup_save_project),
              project_ptr)));
        menulist.push_back(
          Gtk::Menu_Helpers::MenuElem(
            (std::string)"_Close project '" + name + "'",
            sigc::bind(
              sigc::mem_fun(
                *this,
                &project_list_impl::on_menu_popup_close_project),
              project_ptr)));

        menulist.push_back(
          Gtk::Menu_Helpers::MenuElem(
            (std::string)"_Project '" + name + "' properties",
            sigc::bind(
              sigc::mem_fun(
                *this,
                &project_list_impl::on_menu_popup_project_properties),
              project_ptr)));
      }
    }
    else
    {
      //cout << "No row" << endl;
      selection->unselect_all();
    }

    menulist.push_back(Gtk::Menu_Helpers::MenuElem("Cl_ose all projects", sigc::mem_fun(*this, &project_list_impl::on_menu_popup_close_all_projects)));

    _menu_popup.popup(event_ptr->button, event_ptr->time);

    return true;
  }

  return false;
}

void
project_list_impl::on_menu_popup_load_project()
{
  _app->load_project_ask();
}

void
project_list_impl::on_menu_popup_save_all_projects()
{
  _app->save_all_projects();
}

void
project_list_impl::on_menu_popup_save_project(
  boost::shared_ptr<project> project_ptr)
{
  std::string name;

  project_ptr->get_name(name);
  _app->save_project(name);
}

void
project_list_impl::on_menu_popup_close_project(
  boost::shared_ptr<project> project_ptr)
{
  std::string name;

  project_ptr->get_name(name);
  _app->close_project(name);
}

void
project_list_impl::on_menu_popup_project_properties(
  boost::shared_ptr<project> project_ptr)
{
  project_properties_dialog dialog;

  dialog.run(project_ptr);
}

void
project_list_impl::on_menu_popup_close_all_projects()
{
  _app->close_all_projects();
}

void
project_list_impl::project_added(
  boost::shared_ptr<project> project_ptr)
{
  Gtk::TreeModel::iterator iter;
  Gtk::TreeModel::iterator child_iter;
  Gtk::TreeModel::Row row;
  std::string project_name;

  project_ptr->get_name(project_name);

  if (project_ptr->get_modified_status())
  {
    project_name += " *";
  }

  iter = _model->append();
  row = *iter;
  row[_columns.name] = project_name;
  row[_columns.project_ptr] = project_ptr;

  project_ptr->_signal_renamed.connect(bind(mem_fun(this, &project_list_impl::project_renamed), iter));
  project_ptr->_signal_modified_status_changed.connect(bind(mem_fun(this, &project_list_impl::project_renamed), iter));
  project_ptr->_signal_client_added.connect(bind(mem_fun(this, &project_list_impl::client_added), iter));
  project_ptr->_signal_client_removed.connect(bind(mem_fun(this, &project_list_impl::client_removed), iter));
}

void
project_list_impl::project_closed(
  boost::shared_ptr<project> project_ptr)
{
  boost::shared_ptr<project> temp_project_ptr;
  Gtk::TreeModel::Children children = _model->children();
  Gtk::TreeModel::Children::iterator iter = children.begin();

  while(iter != children.end())
  {
    Gtk::TreeModel::Row row = *iter;

    temp_project_ptr = row[_columns.project_ptr];

    if (temp_project_ptr == project_ptr)
    {
      _model->erase(iter);
      return;
    }

    iter++;
  }
}

void
project_list_impl::project_renamed(
  Gtk::TreeModel::iterator iter)
{
  boost::shared_ptr<project> project_ptr;
  std::string project_name;

  Gtk::TreeModel::Row row = *iter;

  project_ptr = row[_columns.project_ptr];
  project_ptr->get_name(project_name);

  if (project_ptr->get_modified_status())
  {
    project_name += " *";
  }

  row[_columns.name] = project_name;
}

void
project_list_impl::client_added(
  boost::shared_ptr<lash_client> client_ptr,
  Gtk::TreeModel::iterator iter)
{
  std::string name;
  Gtk::TreeModel::Path path = _model->get_path(iter);
  Gtk::TreeModel::Row row = *iter;

  client_ptr->get_name(name);

  iter = _model->append(row.children());
  row = *iter;
  row[_columns.name] = name;
  row[_columns.client_ptr] = client_ptr;

  client_ptr->_signal_renamed.connect(bind(mem_fun(this, &project_list_impl::client_renamed), iter));

  _widget->expand_row(path, false);
}

void
project_list_impl::client_renamed(
  Gtk::TreeModel::iterator iter)
{
  boost::shared_ptr<lash_client> client_ptr;
  std::string client_name;

  Gtk::TreeModel::Row row = *iter;

  client_ptr = row[_columns.client_ptr];
  client_ptr->get_name(client_name);

  row[_columns.name] = client_name;
}

void
project_list_impl::client_removed(
  boost::shared_ptr<lash_client> client_ptr,
  Gtk::TreeModel::iterator iter)
{
  std::string name;
  Gtk::TreeModel::Path path = _model->get_path(iter);
  Gtk::TreeModel::Row row = *iter;

  client_ptr->get_name(name);

  Gtk::TreeNodeChildren childs = row.children();

  for (Gtk::TreeModel::iterator child_iter = childs.begin(); iter != childs.end(); child_iter++)
  {
    row = *child_iter;

    if (row[_columns.name] == name)
    {
      _widget->expand_row(path, false);
      _model->erase(child_iter);
      return;
    }
  }
}

void
project_list::set_lash_availability(
  bool lash_active)
{
  _impl_ptr->_widget->set_sensitive(lash_active);
}
#endif

enum
{
  COL_VIEW = 0,
  COL_NAME,
  NUM_COLS
};

GtkWidget * g_world_tree_widget;
GtkTreeStore * g_treestore;

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

  if (gtk_tree_model_get_iter(model, &iter, path))
  {
    gtk_tree_model_get(model, &iter, COL_VIEW, &view, -1);
    //lash_info("%s is going to be %s.", get_view_name(view), path_currently_selected ? "unselected" : "selected");
    if (!path_currently_selected)
    {
      activate_view(view);
    }
  }

  return TRUE;
}

void world_tree_init(void)
{
  GtkTreeViewColumn * col;
  GtkCellRenderer * renderer;
  GtkTreeSelection * selection;

  g_world_tree_widget = get_glade_widget("world_tree");
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(g_world_tree_widget), FALSE);

  col = gtk_tree_view_column_new();
  gtk_tree_view_column_set_title(col, "Name");
  gtk_tree_view_append_column(GTK_TREE_VIEW(g_world_tree_widget), col);
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "text", COL_NAME);

  g_treestore = gtk_tree_store_new(NUM_COLS, G_TYPE_POINTER, G_TYPE_STRING);
  gtk_tree_view_set_model(GTK_TREE_VIEW(g_world_tree_widget), GTK_TREE_MODEL(g_treestore));

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_world_tree_widget));
  gtk_tree_selection_set_select_function(selection, on_select, NULL, NULL);
}

void world_tree_add(graph_view_handle view, bool force_activate)
{
  GtkTreeIter iter;

  gtk_tree_store_append(g_treestore, &iter, NULL);
  gtk_tree_store_set(g_treestore, &iter, COL_VIEW, view, COL_NAME, get_view_name(view), -1);

  /* select the first top level item */
  if (force_activate || gtk_tree_model_iter_n_children(GTK_TREE_MODEL(g_treestore), NULL) == 1)
  {
    gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(g_world_tree_widget)), &iter);
  }
}

static bool find_view(graph_view_handle view, GtkTreeIter * iter_ptr)
{
  graph_view_handle view2;

  if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(g_treestore), iter_ptr))
  {
    do
    {
      gtk_tree_model_get(GTK_TREE_MODEL(g_treestore), iter_ptr, COL_VIEW, &view2, -1);
      if (view == view2)
      {
        return true;
      }
    }
    while (gtk_tree_model_iter_next(GTK_TREE_MODEL(g_treestore), iter_ptr));
  }

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
