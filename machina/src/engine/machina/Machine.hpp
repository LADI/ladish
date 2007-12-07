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
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef MACHINA_MACHINE_HPP
#define MACHINA_MACHINE_HPP

#include <vector>
#include <boost/utility.hpp>
#include <raul/SharedPtr.hpp>
#include <raul/WeakPtr.hpp>
#include <raul/List.hpp>
#include <raul/TimeSlice.hpp>
#include <redlandmm/Model.hpp> 
#include "types.hpp"
#include "LearnRequest.hpp"
#include "Node.hpp"

namespace Machina {


/** A (Finite State) Machine.
 */
class Machine : public Raul::Stateful {
public:
	Machine();
	Machine(const Machine& copy);

	Machine& operator=(const Machine& other);

	// Kluge to appease Eugene
	bool operator==(const Machine& other) { return false; }
	
	// Main context
	void activate()   { _is_activated = true; }
	void deactivate() { _is_activated = false; }
	
	bool is_empty()     { return _nodes.empty(); }
	bool is_finished()  { return _is_finished; }
	bool is_activated() { return _is_activated; }

	void add_node(SharedPtr<Node> node);
	void remove_node(SharedPtr<Node> node);
	void learn(SharedPtr<LearnRequest> learn);

	void write_state(Redland::Model& model);

	// Audio context
	void      reset(Raul::BeatTime time);
	BeatCount run(const Raul::TimeSlice& time);
	
	// Any context
	inline Raul::BeatTime time() const { return _time; }

	SharedPtr<LearnRequest> pending_learn() { return _pending_learn; }
	void clear_pending_learn()              { _pending_learn.reset(); }

	typedef Raul::List< SharedPtr<Node> > Nodes;
	Nodes&       nodes()       { return _nodes; }
	const Nodes& nodes() const { return _nodes; }

	SharedPtr<Node> random_node();
	SharedPtr<Edge> random_edge();

	void set_sink(SharedPtr<Raul::MIDISink> sink);
	
private:
	
	// Audio context
	SharedPtr<Node> earliest_node() const;
	bool enter_node(SharedPtr<Raul::MIDISink> sink, SharedPtr<Node> node);
	void exit_node(SharedPtr<Raul::MIDISink> sink, SharedPtr<Node>);

	static const size_t MAX_ACTIVE_NODES = 128;
	std::vector< SharedPtr<Node> > _active_nodes;

	bool                    _is_activated;
	bool                    _is_finished;
	Raul::BeatTime          _time;
	SharedPtr<LearnRequest> _pending_learn;
	WeakPtr<Raul::MIDISink> _sink;
	Nodes                   _nodes;
};


} // namespace Machina

#endif // MACHINA_MACHINE_HPP
