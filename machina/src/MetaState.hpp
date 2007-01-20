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

#ifndef MACHINA_METASTATE_HPP
#define MACHINA_METASTATE_HPP

#include <vector>
#include "types.hpp"

namespace Machina {

class Node;


class MetaState {
public:
	MetaState(size_t poly);
	~MetaState();

	Node* initial_node() { return _initial_node; }

	void reset();
	void process(FrameCount nframes);

private:
	Node*              _initial_node;
	std::vector<Node*> _voices;

	FrameCount _time;
};


} // namespace Machina

#endif // MACHINA_METASTATE_HPP
