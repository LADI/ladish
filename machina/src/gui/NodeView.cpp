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
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <iostream>
#include <machina/MidiAction.hpp>
#include "NodeView.hpp"
#include "NodePropertiesWindow.hpp"

using namespace std;


NodeView::NodeView(Gtk::Window*                  window,
                   SharedPtr<FlowCanvas::Canvas> canvas,
                   SharedPtr<Machina::Node>      node,
                   const std::string&            name,
                   double                        x,
                   double                        y)
	: FlowCanvas::Ellipse(canvas, name, x, y, 20, 20, false)
	, _window(window)
	, _node(node)
{
	signal_clicked.connect(sigc::mem_fun(this, &NodeView::handle_click));
	update_state(false);
}


void
NodeView::on_double_click(GdkEventButton*)
{
	NodePropertiesWindow::present(_window, _node);
}


void
NodeView::handle_click(GdkEventButton* event)
{
	if (event->state & GDK_CONTROL_MASK) {
		if (event->button == 1) {
			bool is_initial = _node->is_initial();
			_node->set_initial( ! is_initial );
			update_state(_label != NULL);
		} else if (event->button == 3) {
			bool is_selector = _node->is_selector();
			_node->set_selector( ! is_selector );
			update_state(_label != NULL);
		}
	}
}


void
NodeView::show_label(bool show)
{
	char str[4];

	SharedPtr<Machina::MidiAction> action
		= PtrCast<Machina::MidiAction>(_node->enter_action());

	if (show) {
		if (action && action->event_size() > 1
				&& (action->event()[0] & 0xF0) == 0x90) {
			uint8_t note = action->event()[1];
			snprintf(str, 4, "%d", note);
			set_name(str);
		}
	} else {
		set_name("");
	}
}


void
NodeView::update_state(bool show_labels)
{
	static const uint32_t active_color = 0xA0A0AAFF;
	
	if (_node->is_active()) {
		if (_color != active_color) {
			_old_color = _color;
			set_base_color(active_color);
		}
	} else if (_color == active_color) {
		set_base_color(_old_color);
	}
		
	if (_node->is_selector())
		if (_node->is_active())
			set_base_color(0x00FF00FF);
		else	
			set_base_color(0x00A000FF);
	
	set_border_width(_node->is_initial() ? 4.0 : 1.0);

	if (show_labels)
		show_label(true);
}

