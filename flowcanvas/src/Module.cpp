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
#include <cstdlib>
#include <cassert>
#include <cmath>
using std::string;

namespace LibFlowCanvas {

static const int MODULE_FILL_COLOUR           = 0x292929FF;
static const int MODULE_HILITE_FILL_COLOUR    = 0x393939FF;
static const int MODULE_OUTLINE_COLOUR        = 0x505050FF;
static const int MODULE_HILITE_OUTLINE_COLOUR = 0x606060FF;
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
Module::Module(boost::shared_ptr<FlowCanvas> canvas, const string& name, double x, double y)
: Gnome::Canvas::Group(*canvas->root(), x, y),
  m_name(name),
  m_selected(false),
  m_canvas(canvas),
  m_module_box(*this, 0, 0, 0, 0), // w, h set later
  m_canvas_title(*this, 0, 6, name) // x set later
{
	m_module_box.property_fill_color_rgba() = MODULE_FILL_COLOUR;

	m_module_box.property_outline_color_rgba() = MODULE_OUTLINE_COLOUR;
	set_border_width(1.0);

	m_canvas_title.property_size_set() = true;
	m_canvas_title.property_size() = 9000;
	m_canvas_title.property_weight_set() = true;
	m_canvas_title.property_weight() = 400;
	m_canvas_title.property_fill_color_rgba() = MODULE_TITLE_COLOUR;

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
	m_border_width = w;
	m_module_box.property_width_units() = w;
}


bool
Module::module_event(GdkEvent* event)
{
	boost::shared_ptr<FlowCanvas> canvas = m_canvas.lock();
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
			if (m_selected) {
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
				if (m_selected) {
					canvas->unselect_module(m_name);
					assert(!m_selected);
				} else {
					if ( !(event->button.state & GDK_CONTROL_MASK))
						canvas->clear_selection();
					canvas->select_module(m_name);
					assert(m_selected);
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
		for (PortVector::iterator p = m_ports.begin(); p != m_ports.end(); ++p)
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
	m_canvas_title.property_size() = static_cast<int>(floor((double)9000.0f * z));
	for (PortVector::iterator p = m_ports.begin(); p != m_ports.end(); ++p)
		(*p)->zoom(z);
}


void
Module::set_highlighted(bool b)
{
	if (b) {
		m_module_box.property_fill_color_rgba() = MODULE_HILITE_FILL_COLOUR;
	} else {
		m_module_box.property_fill_color_rgba() = MODULE_FILL_COLOUR;
	}
}


void
Module::set_selected(bool selected)
{
	boost::shared_ptr<FlowCanvas> canvas = m_canvas.lock();
	if (!canvas)
		return;

	m_selected = selected;
	if (selected) {
		m_module_box.property_fill_color_rgba() = MODULE_HILITE_FILL_COLOUR;
		m_module_box.property_outline_color_rgba() = MODULE_HILITE_OUTLINE_COLOUR;
		m_module_box.property_dash() = canvas->select_dash();
	} else {
		m_module_box.property_fill_color_rgba() = MODULE_FILL_COLOUR;
		m_module_box.property_outline_color_rgba() = MODULE_OUTLINE_COLOUR;
		m_module_box.property_dash() = NULL;
	}
}


/** Get the port on this module at world coordinate @a x @a y.
 */
boost::shared_ptr<Port>
Module::port_at(double x, double y)
{
	x -= property_x();
	y -= property_y();

	for (PortVector::iterator p = m_ports.begin(); p != m_ports.end(); ++p) {
		boost::shared_ptr<Port> port = *p;
		if (x > port->property_x() && x < port->property_x() + port->width()
				&& y > port->property_y() && y < port->property_y() + port->height()) {
			return port;
		} 
	}

	return boost::shared_ptr<Port>();
}


bool
Module::is_within(const Gnome::Canvas::Rect* rect)
{
	const double x1 = rect->property_x1();
	const double y1 = rect->property_y1();
	const double x2 = rect->property_x2();
	const double y2 = rect->property_y2();

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
	PortVector::iterator i = std::find(m_ports.begin(), m_ports.end(), port);

	if (i != m_ports.end())
		m_ports.erase(i);
}


void
Module::set_width(double w)
{
	m_width = w;
	m_module_box.property_x2() = m_module_box.property_x1() + w;
}


void
Module::set_height(double h)
{
	m_height = h;
	m_module_box.property_y2() = m_module_box.property_y1() + h;
}


/** Move relative to current location.
 *
 * @param dx distance to move along x axis (in world units)
 * @param dy distance to move along y axis (in world units)
 */
void
Module::move(double dx, double dy)
{
	boost::shared_ptr<FlowCanvas> canvas = m_canvas.lock();
	if (!canvas)
		return;

	double new_x = property_x() + dx;
	double new_y = property_y() + dy;
	
	if (new_x < 0)
		dx = property_x() * -1;
	else if (new_x + m_width > canvas->width())
		dx = canvas->width() - property_x() - m_width;
	
	if (new_y < 0)
		dy = property_y() * -1;
	else if (new_y + m_height > canvas->height())
		dy = canvas->height() - property_y() - m_height;

	Gnome::Canvas::Group::move(dx, dy);

	// Deal with moving the connection lines
	for (PortVector::iterator p = m_ports.begin(); p != m_ports.end(); ++p)
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
	boost::shared_ptr<FlowCanvas> canvas = m_canvas.lock();
	if (!canvas)
		return;

	assert(canvas->width() > 0);
	assert(canvas->height() > 0);

	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x + m_width > canvas->width()) x = canvas->width() - m_width - 1;
	if (y + m_height > canvas->height()) y = canvas->height() - m_height - 1;
		
	assert(x >= 0);
	assert(x + m_width < canvas->width());
	assert(y >= 0);
	assert(y + m_height < canvas->height());

	// Man, not many things left to try to get the damn things to move! :)
	property_x() = x;
	property_y() = y;
	//move(0, 0);
	
	// Update any connection line positions
	for (PortVector::iterator p = m_ports.begin(); p != m_ports.end(); ++p)
		(*p)->move_connections();
}


void
Module::set_name(const string& n)
{
	if (m_name != n) {
		string old_name = m_name;
		m_name = n;
		m_canvas_title.property_text() = m_name;
		resize();

		boost::shared_ptr<FlowCanvas> canvas = m_canvas.lock();
		if (canvas)
			canvas->rename_module(old_name, m_name);
	}
}


/** Add a port to this module.
 *
 * A reference to p is held until remove_port is called to remove it.
 */
void
Module::add_port(boost::shared_ptr<Port> p)
{
	PortVector::const_iterator i = std::find(m_ports.begin(), m_ports.end(), p);
	if (i != m_ports.end()) // already added
		return;             // so do nothing
	
	m_ports.push_back(p);
	
	boost::shared_ptr<FlowCanvas> canvas = m_canvas.lock();
	if (canvas) {
		p->signal_event().connect(
			sigc::bind(sigc::mem_fun(m_canvas.lock().get(), &FlowCanvas::port_event), p));
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
	if (m_name.length() == 0)
		hor_pad = 15.0; // leave more room for something to grab for dragging

	boost::shared_ptr<Port> p;
	
	// Find widest in/out ports
	for (PortVector::iterator i = m_ports.begin(); i != m_ports.end(); ++i) {
		p = (*i);
		if (p->is_input() && p->width() > widest_in)
			widest_in = p->width();
		else if (p->is_output() && p->width() > widest_out)
			widest_out = p->width();
	}
	
	// Make sure module is wide enough for ports
	if (widest_in > widest_out)
		set_width(widest_in + hor_pad + border_width()*2.0);
	else
		set_width(widest_out + hor_pad + border_width()*2.0);
	
	// Make sure module is wide enough for title
	if (m_canvas_title.property_text_width() + 6.0 > m_width)
		set_width(m_canvas_title.property_text_width() + 6.0);

	// Set height to contain ports and title
	double height_base = 2;
	if (m_name.length() > 0)
		height_base += m_canvas_title.property_text_height();

	double h = height_base;
	if (m_ports.size() > 0)
		h += m_ports.size() * ((*m_ports.begin())->height()+2.0);
	set_height(h);
	
	// Move ports to appropriate locations
	
	double y;
	int i = 0;
	for (PortVector::iterator pi = m_ports.begin(); pi != m_ports.end(); ++pi, ++i) {
		boost::shared_ptr<Port> p = (*pi);

		y = height_base + (i * (p->height() + 2.0));
		if (p->is_input()) {
			p->set_width(widest_in);
			p->property_x() = 1.0;//border_width();
			p->property_y() = y;
		} else {
			p->set_width(widest_out);
			p->property_x() = m_width - p->width() - 1.0;//p->border_width();
			p->property_y() = y;
		}
	}

	// Center title
	m_canvas_title.property_x() = m_width/2.0;

	// Update connection locations if we've moved/resized
	for (PortVector::iterator pi = m_ports.begin(); pi != m_ports.end(); ++pi, ++i) {
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
	if (m_ports.size() == 0)
		return port->connection_point().get_y();
	else
		return (port->connection_point().get_y() - m_ports.front()->connection_point().get_y());
}


/** Range of port offsets, for connection drawing.
 * See doc/port_offsets.dia
 */
double
Module::port_connection_points_range()
{
	if (m_ports.size() > 0) {
		double ret = fabs(m_ports.back()->connection_point().get_y()
				- m_ports.front()->connection_point().get_y());
	
		return (ret < 1.0) ? 1.0 : ret;
	} else {
		return 1.0;
	}
}


} // namespace LibFlowCanvas
