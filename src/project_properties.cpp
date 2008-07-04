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
#include "project.hpp"
#include "project_properties.hpp"
#include "Widget.hpp"
#include "globals.hpp"

struct project_properties_dialog_impl
{
	Widget<Gtk::Dialog> _dialog;
	Widget<Gtk::Entry> _name;

	project_properties_dialog_impl();
};

project_properties_dialog::project_properties_dialog()
{
	_impl_ptr = new project_properties_dialog_impl;

}

project_properties_dialog::~project_properties_dialog()
{
	delete _impl_ptr;
}

void
project_properties_dialog::run(
	shared_ptr<project> project_ptr)
{
	string name;
	int result;

	project_ptr->get_name(name);
	_impl_ptr->_name->set_text(name);

	result = _impl_ptr->_dialog->run();
	if (result == 2)
	{
		project_ptr->do_rename(_impl_ptr->_name->get_text());
	}

	_impl_ptr->_dialog->hide();
}

project_properties_dialog_impl::project_properties_dialog_impl()
{
	_dialog.init(g_xml, "project_properties_dialog");
	_name.init(g_xml, "project_name");
}
