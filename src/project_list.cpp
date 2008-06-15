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
	Widget<Gtk::TreeView> widget;
	project_list_column_record columns;
	Glib::RefPtr<Gtk::ListStore> model;
};

project_list::project_list(
	Glib::RefPtr<Gnome::Glade::Xml> xml)
{
	_impl_ptr = new project_list_impl;

	_impl_ptr->widget.init(xml, "projects_list");

	_impl_ptr->columns.add(_impl_ptr->columns.name);

	_impl_ptr->model = Gtk::ListStore::create(_impl_ptr->columns);
	_impl_ptr->widget->set_model(_impl_ptr->model);

	_impl_ptr->widget->append_column("Project Name", _impl_ptr->columns.name);
}

project_list::~project_list()
{
	delete _impl_ptr;
}

void
project_list::set_lash_availability(
	bool lash_active)
{
	_impl_ptr->widget->set_sensitive(lash_active);
}

void
project_list::project_added(
	const string& project_name)
{
	Gtk::TreeModel::Row row;

	row = *(_impl_ptr->model->append());
	row[_impl_ptr->columns.name] = project_name;
}

void
project_list::project_closed(
	const string& project_name)
{
	Gtk::TreeModel::Children children = _impl_ptr->model->children();
	Gtk::TreeModel::Children::iterator iter = children.begin();

	while(iter != children.end())
	{
		Gtk::TreeModel::Row row = *iter;

		if (row[_impl_ptr->columns.name] == project_name)
		{
			_impl_ptr->model->erase(iter);
			return;
		}

		iter++;
	}
}
