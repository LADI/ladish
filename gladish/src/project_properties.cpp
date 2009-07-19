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
	Widget<Gtk::Entry> _description;
	Widget<Gtk::TextView> _notes;

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
	string description;
	string notes;
	Glib::RefPtr<Gtk::TextBuffer> buffer;
	int result;

	project_ptr->get_name(name);
	_impl_ptr->_name->set_text(name);

	project_ptr->get_description(description);
	_impl_ptr->_description->set_text(description);

	project_ptr->get_notes(notes);
	buffer = _impl_ptr->_notes->get_buffer();
	buffer->set_text(notes);

	result = _impl_ptr->_dialog->run();
	if (result == 2)
	{
		project_ptr->do_change_description(_impl_ptr->_description->get_text());
		project_ptr->do_change_notes(buffer->get_text());
		project_ptr->do_rename(_impl_ptr->_name->get_text());
	}

	_impl_ptr->_dialog->hide();
}

project_properties_dialog_impl::project_properties_dialog_impl()
{
	_dialog.init(g_xml, "project_properties_dialog");
	_name.init(g_xml, "project_name");
	_description.init(g_xml, "project_description");
	_notes.init(g_xml, "project_notes");
}
