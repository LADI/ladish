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

#include <boost/weak_ptr.hpp>
#include <libgnomecanvasmm.h>
#include <flowcanvas/Connectable.hpp>
#include <flowcanvas/Connection.hpp>

using std::list;

namespace FlowCanvas {
	

/** Update the location of all connections to/from this item if we've moved */
void
Connectable::move_connections()
{
	for (list<boost::weak_ptr<Connection> >::iterator i = _connections.begin(); i != _connections.end(); i++) {
		boost::shared_ptr<Connection> c = i->lock();
		if (c) {
			c->update_location();
		}
	}
}


/** Add a Connection to this item.
 *
 * A reference to the connection is not retained (only a weak_ptr is stored).
 */
void
Connectable::add_connection(boost::shared_ptr<Connection> connection)
{
	/*if (is_output())
		assert(connection->source().lock().get() == this);
	else if (is_input())
		assert(connection->dest().lock().get() == this);*/
	assert(connection->source().lock().get() == this
		|| connection->dest().lock().get() == this);

	for (list<boost::weak_ptr<Connection> >::iterator i = _connections.begin(); i != _connections.end(); i++) {
		boost::shared_ptr<Connection> c = (*i).lock();
		if (c && c == connection)
			return;
	}
	
	_connections.push_back(connection);
}


/** Remove a Connection from this item.
 *
 * Cuts all references to @a c held by the Connectable.
 */
void
Connectable::remove_connection(boost::shared_ptr<Connection> c)
{
	for (list<boost::weak_ptr<Connection> >::iterator i = _connections.begin(); i != _connections.end(); i++) {
		boost::shared_ptr<Connection> connection = i->lock();
		if (connection && connection == c) {
			_connections.erase(i);
			break;
		}
	}
}


void
Connectable::raise_connections()
{
	for (list<boost::weak_ptr<Connection> >::iterator i = _connections.begin(); i != _connections.end(); ++i) {
		boost::shared_ptr<Connection> connection = (*i).lock();
		if (connection)
			connection->raise_to_top();

	}
}
	

bool
Connectable::is_connected_to(boost::shared_ptr<Connectable> other)
{
	for (list<boost::weak_ptr<Connection> >::iterator i = _connections.begin(); i != _connections.end(); ++i) {
		boost::shared_ptr<Connection> connection = (*i).lock();
		if (connection->source().lock() == other || connection->dest().lock() == other)
			return true;
	}

	return false;
}


} // namespace FlowCanvas
