/* This file is part of Machina.  Copyright (C) 2007 Dave Robillard.
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
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MACHINA_EDGE_HPP
#define MACHINA_EDGE_HPP

#include <list>
#include <boost/utility.hpp>
#include "types.hpp"
#include "Action.hpp"

namespace Machina {

class Node;

class Edge : boost::noncopyable {
public:

	Edge(Node* dst) : _src(NULL) , _dst(dst) {}

	Node* src() { return _src; }
	Node* dst() { return _dst; }

	void set_src(Node* src) { _src = src; }

private:
	Node* _src;
	Node* _dst;
};


} // namespace Machina

#endif // MACHINA_EDGE_HPP
