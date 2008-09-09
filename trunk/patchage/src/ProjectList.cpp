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

#include "ProjectList.hpp"
#include "Widget.hpp"
#include "Session.hpp"
#include "Project.hpp"
#include "ProjectPropertiesDialog.hpp"
#include "LashClient.hpp"
#include "LashProxy.hpp"

using boost::shared_ptr;
using namespace std;

struct ProjectList_column_record : public Gtk::TreeModel::ColumnRecord {
	Gtk::TreeModelColumn<Glib::ustring> name;
	Gtk::TreeModelColumn< shared_ptr<Project> > project;
};

struct ProjectListImpl : public sigc::trackable {
	Patchage*                    _app;
	Widget<Gtk::TreeView>        _widget;
	ProjectList_column_record    _columns;
	Glib::RefPtr<Gtk::TreeStore> _model;
	Gtk::Menu                    _menu_popup;

	ProjectListImpl(
	    Glib::RefPtr<Gnome::Glade::Xml> xml,
	    Patchage* app);

	void project_added(shared_ptr<Project> project);
	void project_closed(shared_ptr<Project> project);
	void project_renamed(Gtk::TreeModel::iterator iter);
	void client_added(shared_ptr<LashClient> client, Gtk::TreeModel::iterator iter);
	void client_removed(shared_ptr<LashClient> client, Gtk::TreeModel::iterator iter);

	bool on_button_press_event(GdkEventButton * event);

	void on_menu_popup_load_project();
	void on_menu_popup_save_all_projects();
	void on_menu_popup_save_project(shared_ptr<Project> project);
	void on_menu_popup_close_project(shared_ptr<Project> project);
	void on_menu_popup_project_properties(shared_ptr<Project> project);
	void on_menu_popup_close_all_projects();
};

ProjectList::ProjectList(
    Patchage* app,
    Session* session)
{
	_impl = new ProjectListImpl(app->xml(), app);
	session->_signal_project_added.connect(mem_fun(_impl, &ProjectListImpl::project_added));
	session->_signal_project_closed.connect(mem_fun(_impl, &ProjectListImpl::project_closed));
}

ProjectList::~ProjectList()
{
	delete _impl;
}

ProjectListImpl::ProjectListImpl(Glib::RefPtr<Gnome::Glade::Xml> xml, Patchage* app)
	: _app(app)
	, _widget(xml, "projects_list")
{
	_columns.add(_columns.name);
	_columns.add(_columns.project);

	_model = Gtk::TreeStore::create(_columns);
	_widget->set_model(_model);

	_widget->append_column("LASH projects", _columns.name);

	_menu_popup.accelerate(*_widget);

	_widget->signal_button_press_event().connect(sigc::mem_fun(*this, &ProjectListImpl::on_button_press_event), false);
}

bool
ProjectListImpl::on_button_press_event(GdkEventButton* event)
{
	// Then do our custom stuff:
	if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
		Glib::RefPtr<Gtk::TreeView::Selection> selection = _widget->get_selection();

		Gtk::TreeModel::Path path;
		Gtk::TreeViewColumn * column_ptr;
		int cell_x;
		int cell_y;

		Gtk::Menu::MenuList& menulist = _menu_popup.items();

		menulist.clear();

		menulist.push_back(Gtk::Menu_Helpers::MenuElem("_Load project...", sigc::mem_fun(*this, &ProjectListImpl::on_menu_popup_load_project)));

		menulist.push_back(Gtk::Menu_Helpers::MenuElem("Save _all projects", sigc::mem_fun(*this, &ProjectListImpl::on_menu_popup_save_all_projects)));

		if (_widget->get_path_at_pos((int)event->x, (int)event->y, path, column_ptr, cell_x, cell_y)) {
			//cout << path.to_string() << endl;
			selection->unselect_all();
			selection->select(path);

			Gtk::TreeIter iter = selection->get_selected();
			shared_ptr<Project> project = (*iter)[_columns.project];

			if (project) {
				const string& name = project->get_name();

				menulist.push_back(Gtk::Menu_Helpers::MenuElem(
				        (string)"_Save project '" + name + "'",
				        sigc::bind(
				            sigc::mem_fun(*this,
				                &ProjectListImpl::on_menu_popup_save_project),
				            project)));
				
				menulist.push_back(Gtk::Menu_Helpers::MenuElem(
				        (string)"_Close project '" + name + "'",
				        sigc::bind(
				            sigc::mem_fun(*this,
				                &ProjectListImpl::on_menu_popup_close_project),
				            project)));

				menulist.push_back(Gtk::Menu_Helpers::MenuElem(
				        (string)"_Project '" + name + "' properties",
				        sigc::bind(
				            sigc::mem_fun(*this,
				                &ProjectListImpl::on_menu_popup_project_properties),
				            project)));
			}
		} else {
			//cout << "No row" << endl;
			selection->unselect_all();
		}

		menulist.push_back(Gtk::Menu_Helpers::MenuElem("Cl_ose all projects", sigc::mem_fun(*this, &ProjectListImpl::on_menu_popup_close_all_projects)));

		_menu_popup.popup(event->button, event->time);

		return true;
	}

	return false;
}

void
ProjectListImpl::on_menu_popup_load_project()
{
	_app->show_load_project_dialog();
}

void
ProjectListImpl::on_menu_popup_save_all_projects()
{
	_app->lash_proxy()->save_all_projects();
}

void
ProjectListImpl::on_menu_popup_save_project(shared_ptr<Project> project)
{
	_app->lash_proxy()->save_project(project->get_name());
}

void
ProjectListImpl::on_menu_popup_close_project(shared_ptr<Project> project)
{
	_app->lash_proxy()->close_project(project->get_name());
}

void
ProjectListImpl::on_menu_popup_project_properties(shared_ptr<Project> project)
{
	ProjectPropertiesDialog dialog(_app->xml());
	dialog.run(project);
}

void
ProjectListImpl::on_menu_popup_close_all_projects()
{
	_app->lash_proxy()->close_all_projects();
}

void
ProjectListImpl::project_added(
    shared_ptr<Project> project)
{
	Gtk::TreeModel::iterator iter;
	Gtk::TreeModel::iterator child_iter;
	Gtk::TreeModel::Row row;
	string project_name = project->get_name();

	if (project->get_modified_status()) {
		project_name += " *";
	}

	iter = _model->append();
	row = *iter;
	row[_columns.name] = project_name;
	row[_columns.project] = project;

	project->_signal_renamed.connect(bind(mem_fun(this, &ProjectListImpl::project_renamed), iter));
	project->_signal_modified_status_changed.connect(bind(mem_fun(this, &ProjectListImpl::project_renamed), iter));
	project->_signal_client_added.connect(bind(mem_fun(this, &ProjectListImpl::client_added), iter));
	project->_signal_client_removed.connect(bind(mem_fun(this, &ProjectListImpl::client_removed), iter));
}

void
ProjectListImpl::project_closed(
    shared_ptr<Project> project)
{
	shared_ptr<Project> temp_project;
	Gtk::TreeModel::Children children = _model->children();
	Gtk::TreeModel::Children::iterator iter = children.begin();

	while (iter != children.end()) {
		Gtk::TreeModel::Row row = *iter;

		temp_project = row[_columns.project];

		if (temp_project == project) {
			_model->erase(iter);
			return;
		}

		iter++;
	}
}

void
ProjectListImpl::project_renamed(
    Gtk::TreeModel::iterator iter)
{
	shared_ptr<Project> project;

	Gtk::TreeModel::Row row = *iter;

	project = row[_columns.project];
	string project_name = project->get_name();

	if (project->get_modified_status())
		project_name += " *";

	row[_columns.name] = project_name;
}

void
ProjectListImpl::client_added(
    shared_ptr<LashClient> client,
    Gtk::TreeModel::iterator iter)
{
	Gtk::TreeModel::Path path = _model->get_path(iter);
	Gtk::TreeModel::Row row = *iter;

	iter = _model->append(row.children());
	row = *iter;
	row[_columns.name] = client->get_name();

	_widget->expand_row(path, false);
}

void
ProjectListImpl::client_removed(
    shared_ptr<LashClient> client,
    Gtk::TreeModel::iterator iter)
{
	Gtk::TreeModel::Path path = _model->get_path(iter);
	Gtk::TreeModel::Row row = *iter;

	Gtk::TreeNodeChildren childs = row.children();

	for (Gtk::TreeModel::iterator child_iter = childs.begin(); iter != childs.end(); child_iter++) {
		row = *child_iter;

		if (row[_columns.name] == client->get_name()) {
			_widget->expand_row(path, false);
			_model->erase(child_iter);
			return;
		}
	}
}

void
ProjectList::set_lash_available(bool lash_active)
{
	_impl->_widget->set_sensitive(lash_active);
}

