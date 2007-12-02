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

#include <string>
#include "machina/MidiAction.hpp"
#include "machina/ActionFactory.hpp"
#include "NodePropertiesWindow.hpp"
#include "GladeXml.hpp"

using namespace std;
using namespace Machina;


NodePropertiesWindow* NodePropertiesWindow::_instance = NULL;


NodePropertiesWindow::NodePropertiesWindow(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& xml)
	: Gtk::Dialog(cobject)
{
	property_visible() = false;

	xml->get_widget("node_properties_note_spinbutton", _note_spinbutton);
	xml->get_widget("node_properties_duration_spinbutton", _duration_spinbutton);
	xml->get_widget("node_properties_apply_button", _apply_button);
	xml->get_widget("node_properties_cancel_button", _cancel_button);
	xml->get_widget("node_properties_ok_button", _ok_button);

	_apply_button->signal_clicked().connect(sigc::mem_fun(this, &NodePropertiesWindow::apply_clicked));
	_cancel_button->signal_clicked().connect(sigc::mem_fun(this, &NodePropertiesWindow::cancel_clicked));
	_ok_button->signal_clicked().connect(sigc::mem_fun(this, &NodePropertiesWindow::ok_clicked));
}


NodePropertiesWindow::~NodePropertiesWindow()
{
}


void
NodePropertiesWindow::apply_clicked()
{
	const uint8_t note = _note_spinbutton->get_value();
	if (!_node->enter_action()) {
		_node->set_enter_action(ActionFactory::note_on(note));
		_node->set_exit_action(ActionFactory::note_off(note));
	} else {
		SharedPtr<MidiAction> action = PtrCast<MidiAction>(_node->enter_action());
		action->event()[1] = note;
		action = PtrCast<MidiAction>(_node->exit_action());
		action->event()[1] = note;
	}

	double duration = _duration_spinbutton->get_value();
	_node->set_duration(duration);
	_node->set_changed();
}

	
void
NodePropertiesWindow::cancel_clicked()
{
	assert(this == _instance);
	delete _instance;
	_instance = NULL;
}

	
void
NodePropertiesWindow::ok_clicked()
{
	apply_clicked();
	cancel_clicked();
}


void
NodePropertiesWindow::set_node(SharedPtr<Machina::Node> node)
{
	_node = node;
	SharedPtr<MidiAction> enter_action = PtrCast<MidiAction>(node->enter_action());
	if (enter_action && enter_action->event_size() > 1
			&& (enter_action->event()[0] & 0xF0) == 0x90) {
		_note_spinbutton->set_value(enter_action->event()[1]);
		_note_spinbutton->show();
	} else if (!enter_action) {
		_note_spinbutton->set_value(60);
		_note_spinbutton->show();
	} else {
		_note_spinbutton->hide();
	}
	_duration_spinbutton->set_value(node->duration());
}


void
NodePropertiesWindow::present(Gtk::Window* parent, SharedPtr<Machina::Node> node)
{
	if (!_instance) {
		Glib::RefPtr<Gnome::Glade::Xml> xml = GladeXml::create();

		xml->get_widget_derived("node_properties_dialog", _instance);
	
		if (parent)
			_instance->set_transient_for(*parent);
	}

	_instance->set_node(node);
	_instance->show();
}

