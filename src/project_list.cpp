// -*- Mode: C++ ; indent-tabs-mode: t -*-
/* This file is part of Patchage.
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

#include <iostream>
#include <gtkmm.h>
#include <libglademm/xml.h>

#include "common.hpp"
#include "project_list.hpp"
#include "Widget.hpp"

struct project_list_column_record : public Gtk::TreeModel::ColumnRecord
{
	Gtk::TreeModelColumn<Glib::ustring> name;
};

struct project_list_impl
{
	Widget<Gtk::TreeView> _widget;
	project_list_column_record _columns;
	Glib::RefPtr<Gtk::ListStore> _model;
	Gtk::Menu _menu_popup;

	project_list_impl(Glib::RefPtr<Gnome::Glade::Xml> xml);

	bool on_button_press_event(GdkEventButton * event);

	void on_menu_popup_load_project();
	void on_menu_popup_save_all_projects();
	void on_menu_popup_save_project(const Glib::ustring& project_name);
	void on_menu_popup_close_project(const Glib::ustring& project_name);
};

project_list::project_list(
	Glib::RefPtr<Gnome::Glade::Xml> xml)
{
	_impl_ptr = new project_list_impl(xml);
}

project_list::~project_list()
{
	delete _impl_ptr;
}

project_list_impl::project_list_impl(Glib::RefPtr<Gnome::Glade::Xml> xml)
{
	_widget.init(xml, "projects_list");

	_columns.add(_columns.name);

	_model = Gtk::ListStore::create(_columns);
	_widget->set_model(_model);

	_widget->append_column("Project Name", _columns.name);

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

			Glib::ustring name = (*selection->get_selected())[_columns.name];

			menulist.push_back(Gtk::Menu_Helpers::MenuElem("_Save project", sigc::bind(sigc::mem_fun(*this, &project_list_impl::on_menu_popup_save_project), name)));
			menulist.push_back(Gtk::Menu_Helpers::MenuElem("_Close project", sigc::bind(sigc::mem_fun(*this, &project_list_impl::on_menu_popup_close_project), name)));
		}
		else
		{
			//cout << "No row" << endl;
			selection->unselect_all();
		}

		_menu_popup.popup(event_ptr->button, event_ptr->time);

		return true;
	}

	return false;
}

void
project_list_impl::on_menu_popup_load_project()
{
	cout << "Load project!" << endl;
}

void
project_list_impl::on_menu_popup_save_all_projects()
{
	cout << "Save all projects!" << endl;
}

void
project_list_impl::on_menu_popup_save_project(
	const Glib::ustring& project_name)
{
	cout << "Save project! '" << project_name << "'" << endl;
}

void
project_list_impl::on_menu_popup_close_project(
	const Glib::ustring& project_name)
{
	cout << "Close project! '" << project_name << "'" << endl;
}

void
project_list::set_lash_availability(
	bool lash_active)
{
	_impl_ptr->_widget->set_sensitive(lash_active);
}

void
project_list::project_added(
	const string& project_name)
{
	Gtk::TreeModel::Row row;

	row = *(_impl_ptr->_model->append());
	row[_impl_ptr->_columns.name] = project_name;
}

void
project_list::project_closed(
	const string& project_name)
{
	Gtk::TreeModel::Children children = _impl_ptr->_model->children();
	Gtk::TreeModel::Children::iterator iter = children.begin();

	while(iter != children.end())
	{
		Gtk::TreeModel::Row row = *iter;

		if (row[_impl_ptr->_columns.name] == project_name)
		{
			_impl_ptr->_model->erase(iter);
			return;
		}

		iter++;
	}
}
