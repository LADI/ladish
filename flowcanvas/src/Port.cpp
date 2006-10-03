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
Port::Port(boost::shared_ptr<Module> module, const string& name, bool is_input, int color)
: Gnome::Canvas::Group(*module.get(), 0, 0),
  m_module(module),
  m_name(name),
  m_is_input(is_input),
  m_color(color),
  m_label(*this, 1, 1, m_name),
  m_rect(*this, 0, 0, 0, 0)
{
	m_menu.items().push_back(Gtk::Menu_Helpers::MenuElem(
		"Disconnect All", sigc::mem_fun(this, &Port::disconnect_all)));

	m_rect.property_fill_color_rgba() = color;
	
	// Make rectangle pretty
	//m_rect.property_outline_color_rgba() = 0x8899AAFF;
	m_rect.property_outline_color_rgba() = color;
	m_rect.property_join_style() = Gdk::JOIN_MITER;
	set_border_width(1.0);
	
	// Make label pretty
	m_label.property_size() = PORT_LABEL_SIZE;
	m_label.property_fill_color_rgba() = 0xFFFFFFFF;
	m_label.property_weight() = 200;
	
	m_width = m_label.property_text_width() + 4.0;
	m_height = m_label.property_text_height();

	// Place everything
	m_rect.property_x1() = 0;
	m_rect.property_y1() = 0;
	m_rect.property_x2() = m_width;	
	m_rect.property_y2() = m_height;
	m_label.property_x() = m_label.property_text_width() / 2 + 1;
	m_label.property_y() = m_height / 2;

	m_label.raise_to_top();
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
	m_border_width = w;
	m_rect.property_width_units() = w;
}


void
Port::set_name(const string& n)
{
	m_name = n;
	
	// Reposition label
	m_label.property_text() = m_name;
	m_width = m_label.property_text_width() + 4.0;
	m_height = m_label.property_text_height();
	m_rect.property_x2() = m_width;	
	m_rect.property_y2() = m_height;
	m_label.property_x() = m_label.property_text_width() / 2 + 1;
	m_label.property_y() = m_height / 2;

	signal_renamed.emit(n);
}


void
Port::zoom(float z)
{
	m_label.property_size() = static_cast<int>(8000 * z);
}


/** Update the location of all connections to/from this port if this port has moved */
void
Port::move_connections()
{
	for (list<boost::weak_ptr<Connection> >::iterator i = m_connections.begin(); i != m_connections.end(); i++) {
		boost::shared_ptr<Connection> c = i->lock();
		if (c) {
			c->update_location();
		}
	}
}


/** Add a connection to this port.
 *
 * A reference to the connection is not retained (only a weak_ptr is stored).
 */
void
Port::add_connection(boost::shared_ptr<Connection> c)
{
	//list<boost::weak_ptr<Connection> >::iterator i = find(m_connections.begin(), m_connections.end(), boost::weak_ptr<Connection>(c));
	//if (i == m_connections.end())
		m_connections.push_back(c);
}


void
Port::remove_connection(boost::shared_ptr<Connection> c)
{
	for (list<boost::weak_ptr<Connection> >::iterator i = m_connections.begin(); i != m_connections.end(); i++) {
		boost::shared_ptr<Connection> connection = i->lock();
		if (connection && connection == c) {
			m_connections.erase(i);
			break;
		}
	}
}

void
Port::disconnect_all()
{
	boost::shared_ptr<Module> module = m_module.lock();
	if (!module)
		return;

	list<boost::weak_ptr<Connection> > connections = m_connections; // copy
	for (list<boost::weak_ptr<Connection> >::iterator i = connections.begin(); i != connections.end(); ++i) {
		boost::shared_ptr<Connection> c = (*i).lock();
		if (c)
			module->canvas().lock()->disconnect(c->source_port().lock(), c->dest_port().lock());
	}

	m_connections.clear();
}


void
Port::set_highlighted(bool b)
{
	boost::shared_ptr<Module> module = m_module.lock();
	if (module)
		module->set_highlighted(b);

	for (list<boost::weak_ptr<Connection> >::iterator i = m_connections.begin(); i != m_connections.end(); ++i) {
		boost::shared_ptr<Connection> connection = (*i).lock();
		if (connection) {
			connection->set_highlighted(b);
			if (b)
				connection->raise_to_top();
		}
	}
	
	if (b) {
		raise_to_top();
		m_rect.raise_to_top();
		m_label.raise_to_top();
		m_rect.property_fill_color_rgba() = m_color + 0x33333300;
		m_rect.property_outline_color_rgba() = m_color + 0x33333300;
	} else {
		m_rect.property_fill_color_rgba() = m_color;
		m_rect.property_outline_color_rgba() = m_color;
	}
}
	

void
Port::raise_connections()
{
	for (list<boost::weak_ptr<Connection> >::iterator i = m_connections.begin(); i != m_connections.end(); ++i) {
		boost::shared_ptr<Connection> connection = (*i).lock();
		if (connection)
			connection->raise_to_top();
	}
}


// Returns the world-relative coordinates of where a connection line
// should attach
Gnome::Art::Point
Port::connection_point()
{
	double x = (is_input()) ? m_rect.property_x1()-1.0 : m_rect.property_x2()+1.0;
	double y = m_rect.property_y1() + m_height / 2;
	
	i2w(x, y); // convert to world-relative coords
	
	return Gnome::Art::Point(x, y);
}


void
Port::set_width(double w)
{
	double diff = w - m_width;
	m_rect.property_x2() = m_rect.property_x2() + diff;
	m_width = w;
}


} // namespace LibFlowCanvas
