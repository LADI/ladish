/* This file is part of FlowCanvas.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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

#ifndef FLOWCANVAS_CONNECTABLE_HPP
#define FLOWCANVAS_CONNECTABLE_HPP

#include <list>

#include <boost/shared_ptr.hpp>

namespace FlowCanvas {

class Connection;


/** An object a Connection can connect to.
 */
class Connectable {
public:
	virtual ~Connectable() {}

	virtual Gnome::Art::Point src_connection_point() = 0;
	virtual Gnome::Art::Point dst_connection_point(const Gnome::Art::Point& src) = 0;
	virtual Gnome::Art::Point connection_point_vector(double dx, double dy) = 0;

	virtual void add_connection(boost::shared_ptr<Connection> c);
	virtual void remove_connection(boost::shared_ptr<Connection> c);

	virtual void move_connections();
	virtual void raise_connections();

	bool is_connected_to(boost::shared_ptr<Connectable> other);

	typedef std::list< boost::weak_ptr<Connection> > Connections;
	Connections& connections() { return _connections; }

protected:
	Connections _connections; ///< needed for dragging
};


} // namespace FlowCanvas

#endif // FLOWCANVAS_CONNECTABLE_HPP
