/* This file is part of FlowCanvas.  Copyright (C) 2005 Dave Robillard.
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
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Module.h"
#include "FlowCanvas.h"
#include <functional>
#include <list>
#include <algorithm>
#include <cstdlib>
#include <cassert>
#include <cmath>
using std::string;

namespace LibFlowCanvas {

static const int MODULE_FILL_COLOUR           = 0x292929C8;
static const int MODULE_HILITE_FILL_COLOUR    = 0x292929F4;
static const int MODULE_OUTLINE_COLOUR        = 0x686868E8;
static const int MODULE_HILITE_OUTLINE_COLOUR = 0x808080F4;
static const int MODULE_TITLE_COLOUR          = 0xFFFFFFFF;


/** Construct a Module
 *
 * Note you must call resize() at some point or the module will look ridiculous.
 * This it to avoid unecessary text measuring and resizing, which is insanely
 * expensive.
 *
 * If @a name is the empty string, the space where the title would usually be
 * is not created (eg the module will be shorter).
 */
Module::Module(boost::shared_ptr<FlowCanvas> canvas, const string& name, double x, double y, bool show_title)
: Gnome::Canvas::Group(*canvas->root(), x, y),
  _name(name),
  _selected(false),
  _title_visible(show_title),
  _canvas(canvas),
  _module_box(*this, 0, 0, 0, 0), // w, h set later
  _canvas_title(*this, 0, 8, name) // x set later
{
	_module_box.property_fill_color_rgba() = MODULE_FILL_COLOUR;

	_module_box.property_outline_color_rgba() = MODULE_OUTLINE_COLOUR;
	
	if (canvas->property_aa())
		set_border_width(0.5);
	else
		set_border_width(1.0);

	if (show_title) {
		_canvas_title.property_size_set() = true;
		_canvas_title.property_size() = 9000;
		_canvas_title.property_weight_set() = true;
		_canvas_title.property_weight() = 400;
		_canvas_title.property_fill_color_rgba() = MODULE_TITLE_COLOUR;
	} else {
		_canvas_title.hide();
	}

	set_width(10.0);
	set_height(10.0);

	signal_event().connect(sigc::mem_fun(this, &Module::module_event));
}


Module::~Module()
{
}


/** Set the border width of the module.
 *
 * Do NOT directly set the width_units property on the rect, use this function.
 */
void
Module::set_border_width(double w)
{
	_border_width = w;
	_module_box.property_width_units() = w;
}


bool
Module::module_event(GdkEvent* event)
{
	boost::shared_ptr<FlowCanvas> canvas = _canvas.lock();
	if (!canvas)
		return false;

	/* FIXME:  Some things need to be handled here (ie dragging), but handling double
	 * clicks and whatnot here is just filthy (look at this method.  I mean, c'mon now).
	 * Move double clicks out to on_double_click_event overridden methods etc. */
	
	assert(event);

	static double x, y;
	static double drag_start_x, drag_start_y;
	double module_x, module_y; // FIXME: bad name, actually mouse click loc
	static bool dragging = false;
	bool handled = true;
	static bool double_click = false;
	
	module_x = event->button.x;
	module_y = event->button.y;

	property_parent().get_value()->w2i(module_x, module_y);
	
	switch (event->type) {

	case GDK_2BUTTON_PRESS:
		canvas->clear_selection();
		double_click = true;
		on_double_click(&event->button);
		handled = true;
		break;

	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {
			x = module_x;
			y = module_y;
			// Set these so we can tell on a button release if a drag actually
			// happened (if not, it's just a click)
			drag_start_x = x;
			drag_start_y = y;
			grab(GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK,
			           Gdk::Cursor(Gdk::FLEUR),
			           event->button.time);
			dragging = true;
			handled = true;
		} else if (event->button.button == 2) {
			on_middle_click(&event->button);
			handled = true;
		} else if (event->button.button == 3) {
			on_right_click(&event->button);
		} else {
			handled = false;
		}
		break;
	
	case GDK_MOTION_NOTIFY:
		if ((dragging && (event->motion.state & GDK_BUTTON1_MASK))) {
			double new_x = module_x;
			double new_y = module_y;

			if (event->motion.is_hint) {
				gint t_x;
				gint t_y;
				GdkModifierType state;
				gdk_window_get_pointer(event->motion.window, &t_x, &t_y, &state);
				new_x = t_x;
				new_y = t_y;
			}

			// Move any other selected modules if we're selected
			if (_selected) {
				for (list<boost::shared_ptr<Module> >::iterator i = canvas->selected_modules().begin();
						i != canvas->selected_modules().end(); ++i) {
					(*i)->move(new_x - x, new_y - y);
				}
			} else {
				move(new_x - x, new_y - y);
			}

			x = new_x;
			y = new_y;
		} else {
			handled = false;
		}
		break;

	case GDK_BUTTON_RELEASE:
		if (dragging) {
			ungrab(event->button.time);
			dragging = false;
			if (module_x != drag_start_x || module_y != drag_start_y) { // dragged
				store_location();
			} else if (!double_click) { // just a single-click release
				if (_selected) {
					canvas->unselect_module(_name);
					assert(!_selected);
				} else {
					if ( !(event->button.state & GDK_CONTROL_MASK))
						canvas->clear_selection();
					canvas->select_module(_name);
				}
			}
		} else {
			handled = false;
		}
		double_click = false;
		break;

	case GDK_ENTER_NOTIFY:
		set_highlighted(true);
		raise_to_top();
		for (PortVector::iterator p = _ports.begin(); p != _ports.end(); ++p)
			(*p)->raise_connections();
		break;

	case GDK_LEAVE_NOTIFY:
		set_highlighted(false);
		break;

	default:
		handled = false;
	}

	return handled;
	//return false;
}


void
Module::zoom(double z)
{
	_canvas_title.property_size() = static_cast<int>(floor((double)9000.0f * z));
	for (PortVector::iterator p = _ports.begin(); p != _ports.end(); ++p)
		(*p)->zoom(z);
}


void
Module::set_highlighted(bool b)
{
	if (b) {
		_module_box.property_fill_color_rgba() = MODULE_HILITE_FILL_COLOUR;
		_module_box.property_outline_color_rgba() = MODULE_HILITE_OUTLINE_COLOUR;
	} else {
		_module_box.property_fill_color_rgba() = MODULE_FILL_COLOUR;
		_module_box.property_outline_color_rgba() = MODULE_OUTLINE_COLOUR;
	}
}


void
Module::set_selected(bool selected)
{
	boost::shared_ptr<FlowCanvas> canvas = _canvas.lock();
	if (!canvas)
		return;

	_selected = selected;
	if (selected) {
		_module_box.property_fill_color_rgba() = MODULE_HILITE_FILL_COLOUR;
		_module_box.property_outline_color_rgba() = MODULE_HILITE_OUTLINE_COLOUR;
		_module_box.property_dash() = canvas->select_dash();
	} else {
		_module_box.property_fill_color_rgba() = MODULE_FILL_COLOUR;
		_module_box.property_outline_color_rgba() = MODULE_OUTLINE_COLOUR;
		_module_box.property_dash() = NULL;
	}
}


/** Get the port on this module at world coordinate @a x @a y.
 */
boost::shared_ptr<Port>
Module::port_at(double x, double y)
{
	x -= property_x();
	y -= property_y();

	for (PortVector::iterator p = _ports.begin(); p != _ports.end(); ++p) {
		boost::shared_ptr<Port> port = *p;
		if (x > port->property_x() && x < port->property_x() + port->width()
				&& y > port->property_y() && y < port->property_y() + port->height()) {
			return port;
		} 
	}

	return boost::shared_ptr<Port>();
}


bool
Module::is_within(const Gnome::Canvas::Rect& rect)
{
	const double x1 = rect.property_x1();
	const double y1 = rect.property_y1();
	const double x2 = rect.property_x2();
	const double y2 = rect.property_y2();

	if (x1 < x2 && y1 < y2) {
		return (property_x() > x1
			&& property_y() > y1
			&& property_x() + width() < x2
			&& property_y() + height() < y2);
	} else if (x2 < x1 && y2 < y1) {
		return (property_x() > x2
			&& property_y() > y2
			&& property_x() + width() < x1
			&& property_y() + height() < y1);
	} else if (x1 < x2 && y2 < y1) {
		return (property_x() > x1
			&& property_y() > y2
			&& property_x() + width() < x2
			&& property_y() + height() < y1);
	} else if (x2 < x1 && y1 < y2) {
		return (property_x() > x2
			&& property_y() > y1
			&& property_x() + width() < x1
			&& property_y() + height() < y2);
	} else {
		return false;
	}
}


boost::shared_ptr<Port>
Module::remove_port(const string& name)
{
	boost::shared_ptr<Port> ret = get_port(name);

	if (ret)
		remove_port(ret);
	
	return ret;
}


void
Module::remove_port(boost::shared_ptr<Port> port)
{
	PortVector::iterator i = std::find(_ports.begin(), _ports.end(), port);

	if (i != _ports.end()) {
		_ports.erase(i);
		resize();
	} else {
		std::cerr << "Unable to find port " << port->name() << " to remove." << std::endl;
	}
}


void
Module::set_width(double w)
{
	_width = w;
	_module_box.property_x2() = _module_box.property_x1() + w;
}


void
Module::set_height(double h)
{
	_height = h;
	_module_box.property_y2() = _module_box.property_y1() + h;
}


/** Move relative to current location.
 *
 * @param dx distance to move along x axis (in world units)
 * @param dy distance to move along y axis (in world units)
 */
void
Module::move(double dx, double dy)
{
	boost::shared_ptr<FlowCanvas> canvas = _canvas.lock();
	if (!canvas)
		return;

	double new_x = property_x() + dx;
	double new_y = property_y() + dy;
	
	if (new_x < 0)
		dx = property_x() * -1;
	else if (new_x + _width > canvas->width())
		dx = canvas->width() - property_x() - _width;
	
	if (new_y < 0)
		dy = property_y() * -1;
	else if (new_y + _height > canvas->height())
		dy = canvas->height() - property_y() - _height;

	Gnome::Canvas::Group::move(dx, dy);

	// Deal with moving the connection lines
	for (PortVector::iterator p = _ports.begin(); p != _ports.end(); ++p)
		(*p)->move_connections();
}


/** Move to the specified absolute coordinate on the canvas.
 *
 * @param x x coordinate to move to (in world units)
 * @param y y coordinate to move to (in world units)
 */
void
Module::move_to(double x, double y)
{
	boost::shared_ptr<FlowCanvas> canvas = _canvas.lock();
	if (!canvas)
		return;

	assert(canvas->width() > 0);
	assert(canvas->height() > 0);

	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x + _width > canvas->width()) x = canvas->width() - _width - 1;
	if (y + _height > canvas->height()) y = canvas->height() - _height - 1;
		
	assert(x >= 0);
	assert(x + _width < canvas->width());
	assert(y >= 0);
	assert(y + _height < canvas->height());

	// Man, not many things left to try to get the damn things to move! :)
	property_x() = x;
	property_y() = y;
	
	// Update any connection line positions
	for (PortVector::iterator p = _ports.begin(); p != _ports.end(); ++p)
		(*p)->move_connections();
}


void
Module::set_name(const string& n)
{
	if (_name != n) {
		string old_name = _name;
		_name = n;
		_canvas_title.property_text() = _name;
		if (_title_visible)
			resize();

		boost::shared_ptr<FlowCanvas> canvas = _canvas.lock();
		if (canvas)
			canvas->rename_module(old_name, _name);
	}
}


/** Add a port to this module.
 *
 * A reference to p is held until remove_port is called to remove it.  Note
 * that the module will not be resized (for performance reasons when adding
 * many ports in succession), so you must explicitly call resize() after this
 * for the module to look at all sensible.
 */
void
Module::add_port(boost::shared_ptr<Port> p)
{
	PortVector::const_iterator i = std::find(_ports.begin(), _ports.end(), p);
	if (i != _ports.end()) // already added
		return;             // so do nothing
	
	_ports.push_back(p);
	
	boost::shared_ptr<FlowCanvas> canvas = _canvas.lock();
	if (canvas) {
		p->signal_event().connect(
			sigc::bind(sigc::mem_fun(_canvas.lock().get(), &FlowCanvas::port_event), p));
	}
}


/** Resize the module to fit its contents best.
 */
void
Module::resize()
{
	double widest_in = 0.0;
	double widest_out = 0.0;
	
	// The amount of space between a port edge and the module edge (on the
	// side that the port isn't right on the edge).
	double hor_pad = 5.0;
	if (!_title_visible)
		hor_pad = 15.0; // leave more room for something to grab for dragging

	boost::shared_ptr<Port> p;
	
	// Find widest in/out ports
	for (PortVector::iterator i = _ports.begin(); i != _ports.end(); ++i) {
		p = (*i);
		assert(p);
		if (p->is_input() && p->width() > widest_in)
			widest_in = p->width();
		else if (p->is_output() && p->width() > widest_out)
			widest_out = p->width();
	}
	
	// Make sure module is wide enough for ports
	set_width(std::max(widest_in, widest_out) + hor_pad + border_width()*2.0);
	
	// Make sure module is wide enough for title
	if (_title_visible && _canvas_title.property_text_width() + 8.0 > _width)
		set_width(_canvas_title.property_text_width() + 8.0);

	// Set height to contain ports and title
	double height_base = 2;
	if (_title_visible)
		height_base += 2 + _canvas_title.property_text_height();

	double h = height_base;
	if (_ports.size() > 0)
		h += _ports.size() * ((*_ports.begin())->height()+2.0);

	if (!_title_visible && _ports.size() > 0)
		h += 0.5;

	set_height(h);
	
	// Move ports to appropriate locations
	
	double y;
	int i = 0;
	for (PortVector::iterator pi = _ports.begin(); pi != _ports.end(); ++pi, ++i) {
		boost::shared_ptr<Port> p = (*pi);

		y = height_base + (i * (p->height() + 2.0));
		if (p->is_input()) {
			p->set_width(widest_in);
			p->property_x() = 0.0;
			p->property_y() = y;
		} else {
			p->set_width(widest_out);
			p->property_x() = _width - p->width() - 0.0;
			p->property_y() = y;
		}
	}

	// Center title
	_canvas_title.property_x() = _width/2.0;

	// Update connection locations if we've moved/resized
	for (PortVector::iterator pi = _ports.begin(); pi != _ports.end(); ++pi, ++i) {
		(*pi)->move_connections();
	}
	
	// Make things actually move to their new locations (?!)
	move(0, 0);
}


/** Port offset, for connection drawing.
 * See doc/port_offsets.dia
 */
double
Module::port_connection_point_offset(boost::shared_ptr<Port> port)
{
	if (_ports.size() == 0)
		return port->connection_point().get_y();
	else
		return (port->connection_point().get_y() - _ports.front()->connection_point().get_y());
}


/** Range of port offsets, for connection drawing.
 * See doc/port_offsets.dia
 */
double
Module::port_connection_points_range()
{
	if (_ports.size() > 0) {
		double ret = fabs(_ports.back()->connection_point().get_y()
				- _ports.front()->connection_point().get_y());
	
		return (ret < 1.0) ? 1.0 : ret;
	} else {
		return 1.0;
	}
}


} // namespace LibFlowCanvas
