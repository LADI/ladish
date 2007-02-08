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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/weak_ptr.hpp>
#include <libgnomecanvasmm.h>
#include "Port.h"
#include "Module.h"
#include "FlowCanvas.h"

namespace LibFlowCanvas {
	

/** Contruct a Port on an existing module.
 *
 * A reference to @a module is not retained (only a weak_ptr is stored).
 */
Port::Port(boost::shared_ptr<Module> module, const string& name, bool is_input, uint32_t color)
: Gnome::Canvas::Group(*module.get(), 0, 0),
  _module(module),
  _name(name),
  _is_input(is_input),
  _color(color),
  _label(*this, 1, 1, _name),
  _rect(*this, 0, 0, 0, 0)
{
	_menu.items().push_back(Gtk::Menu_Helpers::MenuElem(
		"Disconnect All", sigc::mem_fun(this, &Port::disconnect_all)));

	_rect.property_fill_color_rgba() = color;
	
	// Make rectangle pretty
	//m_rect.property_outline_color_rgba() = 0x8899AAFF;
	_rect.property_outline_color_rgba() = color;
	_rect.property_join_style() = Gdk::JOIN_MITER;
	set_border_width(0.0);
	
	// Make label pretty
	_label.property_size() = PORT_LABEL_SIZE;
	_label.property_fill_color_rgba() = 0xFFFFFFFF;
	_label.property_weight() = 200;
	
	_width = _label.property_text_width() + 6.0;
	_height = _label.property_text_height();
	
	// Place everything
	_rect.property_x1() = 0;
	_rect.property_y1() = 0;
	_rect.property_x2() = _width;	
	_rect.property_y2() = _height;
	_label.property_x() = _label.property_text_width() / 2.0 + 3.0;
	_label.property_y() = (_height / 2.0) - 1.0;

	_label.raise_to_top();
}


Port::~Port()
{
}


/** Set the border width of the port's rectangle.
 *
 * Do NOT directly set the width_units property on the rect, use this function.
 */
void
Port::set_border_width(double w)
{
	_border_width = w;
	_rect.property_width_units() = w;
}


void
Port::set_name(const string& n)
{
	_name = n;
	
	// Reposition label
	_label.property_text() = _name;
	_width = _label.property_text_width() + 4.0;
	_height = _label.property_text_height();
	_rect.property_x2() = _width;	
	_rect.property_y2() = _height;
	_label.property_x() = _label.property_text_width() / 2 + 1;
	_label.property_y() = _height / 2;

	signal_renamed.emit(n);
}


void
Port::zoom(float z)
{
	_label.property_size() = static_cast<int>(8000 * z);
}


void
Port::disconnect_all()
{
	boost::shared_ptr<Module> module = _module.lock();
	if (!module)
		return;

	list<boost::weak_ptr<Connection> > connections = _connections; // copy
	for (list<boost::weak_ptr<Connection> >::iterator i = connections.begin(); i != connections.end(); ++i) {
		boost::shared_ptr<Connection> c = (*i).lock();
		if (c) {
			module->canvas().lock()->disconnect(c->source().lock(), c->dest().lock());
		}
	}

	_connections.clear();
}


void
Port::set_highlighted(bool b)
{
	boost::shared_ptr<Module> module = _module.lock();
	if (module)
		module->set_highlighted(b);

	for (list<boost::weak_ptr<Connection> >::iterator i = _connections.begin(); i != _connections.end(); ++i) {
		boost::shared_ptr<Connection> connection = (*i).lock();
		if (connection) {
			connection->set_highlighted(b);
			if (b)
				connection->raise_to_top();
		}
	}
	
	if (b) {
		raise_to_top();
		_rect.raise_to_top();
		_label.raise_to_top();
		_rect.property_fill_color_rgba() = _color + 0x33333300;
		_rect.property_outline_color_rgba() = _color + 0x33333300;
	} else {
		_rect.property_fill_color_rgba() = _color;
		_rect.property_outline_color_rgba() = _color;
	}
}
	

// Returns the world-relative coordinates of where a connection line
// should attach
Gnome::Art::Point
Port::connection_point()
{
	double x = (is_input()) ? _rect.property_x1()-1.0 : _rect.property_x2()+1.0;
	double y = _rect.property_y1() + _height / 2.0;
	
	i2w(x, y); // convert to world-relative coords
	
	return Gnome::Art::Point(x, y);
}


void
Port::set_width(double w)
{
	double diff = w - _width;
	_rect.property_x2() = _rect.property_x2() + diff;
	_width = w;
}


} // namespace LibFlowCanvas
