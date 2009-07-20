/* This file is part of Patchage.
 * Copyright (C) 2008 Dave Robillard <dave@drobilla.net>
 * Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
 *
 * Patchage is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Patchage is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "common.hpp"
#include "lash_proxy.hpp"
#include "load_projects_dialog.hpp"
#include "Patchage.hpp"
#include "globals.hpp"

struct LoadProjectDialog
{
  LoadProjectDialog();

  void run(std::list<lash_project_info>& projects);

  void load_selected_project();
  bool on_button_press_event(GdkEventButton* event_ptr);
  bool on_key_press_event(GdkEventKey* event_ptr);

  int mtime_sorter(Gtk::TreeModel::iterator a, Gtk::TreeModel::iterator b);

  struct Record : public Gtk::TreeModel::ColumnRecord
  {
    Gtk::TreeModelColumn<Glib::ustring> name;
    Gtk::TreeModelColumn<Glib::ustring> modified;
    Gtk::TreeModelColumn<Glib::ustring> description;
    Gtk::TreeModelColumn<time_t> mtime;
  };

  Widget<Gtk::Dialog> _dialog;
  Widget<Gtk::TreeView> _widget;
  Record _columns;
  Glib::RefPtr<Gtk::ListStore> _model;
};

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


LoadProjectDialog::LoadProjectDialog()
  : _dialog(g_xml, "load_project_dialog")
  , _widget(g_xml, "loadable_projects_list")
{
  _dialog.init(g_xml, "load_project_dialog");
  _widget.init(g_xml, "loadable_projects_list");

  _columns.add(_columns.name);
  _columns.add(_columns.modified);
  _columns.add(_columns.description);
  _columns.add(_columns.mtime);

  _model = Gtk::ListStore::create(_columns);
  _widget->set_model(_model);

  _widget->remove_all_columns();

  _widget->append_column("Project Name", _columns.name);
  _widget->get_column(0)->set_sort_column(_columns.name);

  _widget->append_column("Modified", _columns.modified);
  _model->set_sort_func(_columns.modified, sigc::mem_fun(this, &LoadProjectDialog::mtime_sorter));
  _widget->get_column(1)->set_sort_column(_columns.modified);
  _model->set_sort_column(_columns.modified, Gtk::SORT_ASCENDING);

  _widget->append_column("Description", _columns.description);
  _widget->get_column(2)->set_sort_column(_columns.description);
}

int
LoadProjectDialog::mtime_sorter(Gtk::TreeModel::iterator a, Gtk::TreeModel::iterator b)
{
  time_t ta = (*a)[_columns.mtime];
  time_t tb = (*b)[_columns.mtime];

  return ta > tb ? -1 : (ta == tb ? 0 : 1);
}

void
LoadProjectDialog::run(std::list<lash_project_info>& projects)
{
  Gtk::TreeModel::Row row;
  int result;

  for (std::list<lash_project_info>::iterator iter = projects.begin(); iter != projects.end(); iter++)
  {
    std::string str;
    row = *(_model->append());
    row[_columns.name] = iter->name;
    convert_timestamp_to_string(iter->modification_time, str);
    row[_columns.modified] = str;
    row[_columns.description] = iter->description;
    row[_columns.mtime] = iter->modification_time;
  }

  _widget->signal_button_press_event().connect(sigc::mem_fun(*this, &LoadProjectDialog::on_button_press_event), false);
  _widget->signal_key_press_event().connect(sigc::mem_fun(*this, &LoadProjectDialog::on_key_press_event), false);

loop:
  result = _dialog->run();

  if (result == 2)
  {
    Glib::RefPtr<Gtk::TreeView::Selection> selection = _widget->get_selection();
    Gtk::TreeIter iter = selection->get_selected();
    if (!iter)
    {
      goto loop;
    }

    Glib::ustring project_name = (*iter)[_columns.name];
    g_app->load_project(project_name);
  }

  _dialog->hide();
}


void
LoadProjectDialog::load_selected_project()
{
  Glib::RefPtr<Gtk::TreeView::Selection> selection = _widget->get_selection();
  Glib::ustring name = (*selection->get_selected())[_columns.name];
  g_app->load_project(name);
  _dialog->hide();
}


bool
LoadProjectDialog::on_button_press_event(GdkEventButton * event_ptr)
{
  if (event_ptr->type == GDK_2BUTTON_PRESS && event_ptr->button == 1)
  {
    load_selected_project();
    return true;
  }
  return false;
}


bool
LoadProjectDialog::on_key_press_event(GdkEventKey * event_ptr)
{
  if (event_ptr->type == GDK_KEY_PRESS &&
      (event_ptr->keyval == GDK_Return ||
       event_ptr->keyval == GDK_KP_Enter))
  {
    load_selected_project();
    return true;
  }

  return false;
}

void run_load_project_dialog(std::list<lash_project_info>& projects)
{
  LoadProjectDialog dialog;
  dialog.run(projects);
}
