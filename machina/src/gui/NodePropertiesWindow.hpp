/* This file is part of Machina.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Machina is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Machina is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef NODEPROPERTIESWINDOW_H
#define NODEPROPERTIESWINDOW_H

#include <gtkmm.h>
#include <libglademm/xml.h>
#include "machina/Node.hpp"

class NodePropertiesWindow : public Gtk::Dialog
{
public:
	static void present(Gtk::Window* parent, SharedPtr<Machina::Node> node);

private:
	friend class Gnome::Glade::Xml;
	NodePropertiesWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& glade_xml);
	~NodePropertiesWindow();

	void set_node(SharedPtr<Machina::Node> node);

	void ok_clicked();
	void cancel_clicked();
	
	static NodePropertiesWindow* _instance;

	SharedPtr<Machina::Node> _node;

	Gtk::SpinButton* _note_spinbutton;
	Gtk::SpinButton* _duration_spinbutton;
	Gtk::Button*     _cancel_button;
	Gtk::Button*     _ok_button;
};


#endif // NODEPROPERTIESWINDOW_H
