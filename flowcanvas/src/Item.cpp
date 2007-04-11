/* This file is part of FlowCanvas.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * FlowCanvas is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * FlowCanvas is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/shared_ptr.hpp>
#include "Item.h"
#include "FlowCanvas.h"

namespace LibFlowCanvas {


Item::Item(boost::shared_ptr<FlowCanvas> canvas,
     const string&                 name,
     double                        x,
     double                        y,
     uint32_t                      color)
	: Gnome::Canvas::Group(*canvas->root(), x, y)
	, _canvas(canvas)
	, _name(name)
	, _width(1)
	, _height(1)
	, _color(color)
	, _selected(false)
{
	signal_event().connect(sigc::mem_fun(this, &Item::item_event));

	signal_clicked.connect(       sigc::mem_fun(this, &Item::on_click));
	signal_double_clicked.connect(sigc::mem_fun(this, &Item::on_double_click));
	signal_dragged.connect(       sigc::mem_fun(this, &Item::on_drag));
}


void
Item::set_selected(bool s)
{
	_selected = s;

	if (s)
		signal_selected.emit();
	else
		signal_unselected.emit();
}


/** Event handler to fire (higher level, abstracted) Item signals from Gtk events.
 */
bool
Item::item_event(GdkEvent* event)
{
	boost::shared_ptr<FlowCanvas> canvas = _canvas.lock();
	if (!canvas || !event)
		return false;

	static double x, y;
	static double drag_start_x, drag_start_y;
	static bool double_click = false;
	static bool dragging = false;
	double click_x, click_y;
	
	click_x = event->button.x;
	click_y = event->button.y;

	property_parent().get_value()->w2i(click_x, click_y);
	
	switch (event->type) {

	case GDK_2BUTTON_PRESS:
		signal_double_clicked.emit(&event->button);
		double_click = true;
		break;

	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {
			x = click_x;
			y = click_y;
			// Set these so we can tell on a button release if a drag actually
			// happened (if not, it's just a click)
			drag_start_x = x;
			drag_start_y = y;
			grab(GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK,
			           Gdk::Cursor(Gdk::FLEUR),
			           event->button.time);
			dragging = true;
		} else {
			signal_clicked.emit(&event->button);
		}
		break;
	
	case GDK_MOTION_NOTIFY:
		if ((dragging && (event->motion.state & GDK_BUTTON1_MASK))) {
			double new_x = click_x;
			double new_y = click_y;

			if (event->motion.is_hint) {
				gint t_x;
				gint t_y;
				GdkModifierType state;
				gdk_window_get_pointer(event->motion.window, &t_x, &t_y, &state);
				new_x = t_x;
				new_y = t_y;
			}

			signal_dragged.emit(new_x - x, new_y - y);

			x = new_x;
			y = new_y;
		}
		break;

	case GDK_BUTTON_RELEASE:
		if (dragging) {
			ungrab(event->button.time);
			dragging = false;
			if (click_x != drag_start_x || click_y != drag_start_y) {
				signal_dropped(click_x, click_y);
			} else if (!double_click) {
				on_click(&event->button);
				signal_clicked.emit(&event->button);
			}
		}
		double_click = false;
		break;

	case GDK_ENTER_NOTIFY:
		signal_pointer_entered.emit();
		//set_highlighted(true);
		raise_to_top();
		break;

	case GDK_LEAVE_NOTIFY:
		signal_pointer_exited.emit();
		//set_highlighted(false);
		break;

	default:
		break;
	}

	// Never stop event propagation so derived classes have full access
	// to GTK events.
	return false;
}


void
Item::on_click(GdkEventButton* event)
{
	if (event->button == 1) {
		if (_selected) {
			_canvas.lock()->unselect_item(shared_from_this());
			assert(!_selected);
		} else {
			if ( !(event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)))
				_canvas.lock()->clear_selection();
			_canvas.lock()->select_item(shared_from_this());
		}
	}
}


void
Item::on_double_click(GdkEventButton*)
{
	_canvas.lock()->clear_selection();
}


void
Item::on_drag(double dx, double dy)
{
	// Move any other selected modules if we're selected
	if (_selected) {
		for (list<boost::shared_ptr<LibFlowCanvas::Item> >::iterator i = _canvas.lock()->selected_items().begin();
				i != _canvas.lock()->selected_items().end(); ++i) {
			(*i)->move(dx, dy);
		}
	} else {
		move(dx, dy);
	}
}


} // namespace LibFlowCanvas
