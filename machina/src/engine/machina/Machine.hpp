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

#include <raul/SharedPtr.h>
#include <raul/List.h>
#include <raul/RDFWriter.h> 
#include <raul/TimeSlice.h>
#include "types.hpp"
#include "LearnRequest.hpp"
#include "Node.hpp"

namespace Machina {


class Machine : public Raul::Stateful {
public:
	Machine();
	~Machine();

	// Main context
	void activate()   { _is_activated = true; }
	void deactivate() { _is_activated = false; }
	
	bool is_empty()    { return _nodes.empty(); }
	bool is_finished() { return _is_finished; }

	void add_node(SharedPtr<Node> node);
	void learn(SharedPtr<LearnRequest> learn);

	void write_state(Raul::RDFWriter& writer);

	// Audio context
	void      reset();
	BeatCount run(const Raul::TimeSlice& time);
	
	// Any context
	Raul::BeatTime time() { return _time; }

	//LearnRequest pop_learn()     { return _pending_learns.pop_front(); }
	//SharedPtr<LearnRequest> first_learn() { return *_pending_learns.begin(); }
	SharedPtr<LearnRequest> pending_learn()   { return _pending_learn; }
	void clear_pending_learn() { _pending_learn.reset(); }

	typedef Raul::List<SharedPtr<Node> > Nodes;
	const Nodes& nodes() { return _nodes; }

	void set_sink(SharedPtr<Raul::MIDISink> sink);
	
private:
	
	// Audio context
	SharedPtr<Node> earliest_node() const;
	void            exit_node(const SharedPtr<Raul::MIDISink> sink, const SharedPtr<Node>);

	WeakPtr<Raul::MIDISink> _sink;
	bool                    _is_activated;
	bool                    _is_finished;
	Raul::BeatTime          _time;
	Nodes                   _nodes;

	//Raul::List<SharedPtr<LearnRequest> > _pending_learns;
	SharedPtr<LearnRequest> _pending_learn;
};


} // namespace Machina

#endif // MACHINA_MACHINE_HPP
