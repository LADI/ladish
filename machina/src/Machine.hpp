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

#ifndef MACHINA_MACHINE_HPP
#define MACHINA_MACHINE_HPP

#include <vector>
#include <map>
#include "raul/SharedPtr.h"
#include "types.hpp"
#include "Node.hpp"

namespace Machina {


class Machine {
public:
	Machine(size_t poly);
	~Machine();

	// Main context
	void activate()   { _activated = true; }
	void deactivate() { _activated = false; }
	void add_node(const Node::ID& id, SharedPtr<Node> node);

	// Audio context
	void reset();
	void process(FrameCount nframes);
	
	SharedPtr<Node> initial_node() { return _initial_node; }

private:
	bool               _activated;
	SharedPtr<Node>    _initial_node;
	std::vector<Node*> _voices;
	
	std::map<Node::ID, SharedPtr<Node> > _nodes;

	FrameCount _time;
};


} // namespace Machina

#endif // MACHINA_MACHINE_HPP
