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

#ifndef FLOWCANVAS_CONNECTABLE_H
#define FLOWCANVAS_CONNECTABLE_H

#include <boost/shared_ptr.hpp>

namespace LibFlowCanvas {

class Connection;


class Connectable {
public:
	virtual ~Connectable() {}

	virtual Gnome::Art::Point connection_point() { return Gnome::Art::Point(0, 0); }
	
	virtual void add_connection(boost::shared_ptr<Connection> c) = 0;
	virtual void remove_connection(boost::shared_ptr<Connection> c) = 0;
};


} // namespace LibFlowCanvas

#endif // FLOWCANVAS_CONNECTABLE_H
