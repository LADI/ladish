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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef MACHINA_MACHINE_HPP
#define MACHINA_MACHINE_HPP

#include <vector>
#include <map>
#include <raul/SharedPtr.h>
#include <raul/List.h>
#include "types.hpp"
#include "Node.hpp"

namespace Machina {


class Machine {
public:
	Machine();
	~Machine();

	// Main context
	void activate()   { _is_activated = true; }
	void deactivate() { _is_activated = false; }
	
	bool is_finished() { return _is_finished; }

	void add_node(SharedPtr<Node> node);

	// Audio context
	void            reset();
	bool            run(FrameCount nframes);
	
	// Any context
	FrameCount time() { return _time; }

private:
	typedef Raul::List<SharedPtr<Node> > Nodes;
	
	// Audio context
	SharedPtr<Node> earliest_node() const;
	void            exit_node(const SharedPtr<Node>);

	bool       _is_activated;
	bool       _is_finished;
	FrameCount _time;
	Nodes      _nodes;
};


} // namespace Machina

#endif // MACHINA_MACHINE_HPP
