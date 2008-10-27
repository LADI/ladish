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

#ifndef PATCHAGE_LOAD_PROJECT_DIALOG_H
#define PATCHAGE_LOAD_PROJECT_DIALOG_H

#include <list>
#include <gtkmm.h>
#include "Widget.hpp"

class Patchage;
class ProjectInfo;

class LoadProjectDialog {
public:
	LoadProjectDialog(Patchage* app);

	void run(std::list<ProjectInfo>& projects);

private:
	struct Record : public Gtk::TreeModel::ColumnRecord {
		Gtk::TreeModelColumn<Glib::ustring> name;
		Gtk::TreeModelColumn<Glib::ustring> modified;
		Gtk::TreeModelColumn<Glib::ustring> description;
	};

	void load_selected_project();
	bool on_button_press_event(GdkEventButton* event_ptr);
	bool on_key_press_event(GdkEventKey* event_ptr);

	Patchage*                    _app;
	Widget<Gtk::Dialog>          _dialog;
	Widget<Gtk::TreeView>        _widget;
	Record                       _columns;
	Glib::RefPtr<Gtk::ListStore> _model;
};

#endif // PATCHAGE_LOAD_PROJECT_DIALOG_H
