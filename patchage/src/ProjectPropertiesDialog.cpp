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
#include "Patchage.hpp"
#include "Project.hpp"
#include "ProjectPropertiesDialog.hpp"
#include "Widget.hpp"

using boost::shared_ptr;
using namespace std;


struct ProjectPropertiesDialogImpl {
	ProjectPropertiesDialogImpl(Glib::RefPtr<Gnome::Glade::Xml> xml);

	Widget<Gtk::Dialog> _dialog;
	Widget<Gtk::Entry> _name;
	Widget<Gtk::Entry> _description;
	Widget<Gtk::TextView> _notes;
};


ProjectPropertiesDialog::ProjectPropertiesDialog(Glib::RefPtr<Gnome::Glade::Xml> xml)
{
	_impl = new ProjectPropertiesDialogImpl(xml);
}


ProjectPropertiesDialog::~ProjectPropertiesDialog()
{
	delete _impl;
}


void
ProjectPropertiesDialog::run(shared_ptr<Project> project)
{
	Glib::RefPtr<Gtk::TextBuffer> buffer;
	int result;

	_impl->_name->set_text(project->get_name());

	_impl->_description->set_text(project->get_description());

	buffer = _impl->_notes->get_buffer();
	buffer->set_text(project->get_notes());

	result = _impl->_dialog->run();
	if (result == 2) {
		project->do_change_description(_impl->_description->get_text());
		project->do_change_notes(buffer->get_text());
		project->do_rename(_impl->_name->get_text());
	}

	_impl->_dialog->hide();
}


ProjectPropertiesDialogImpl::ProjectPropertiesDialogImpl(Glib::RefPtr<Gnome::Glade::Xml> xml)
	: _dialog(xml, "project_properties_dialog")
	, _name(xml, "project_name")
	, _description(xml, "project_description")
	, _notes(xml, "project_notes")
{
}

