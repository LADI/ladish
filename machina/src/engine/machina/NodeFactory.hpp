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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MACHINA_NODEFACTORY_HPP
#define MACHINA_NODEFACTORY_HPP

#include <raul/SharedPtr.h>
#include "types.hpp"
#include "Node.hpp"

namespace Machina {


class NodeFactory {
public:
	virtual ~NodeFactory() {}

	virtual SharedPtr<Node> create_node(Node::ID      id,
	                                    unsigned char note,
	                                    FrameCount    duration) = 0;
};


} // namespace Machina

#endif // MACHINA_NODEFACTORY_HPP
