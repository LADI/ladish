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

#ifndef MACHINA_NODE_HPP
#define MACHINA_NODE_HPP

#include <raul/SharedPtr.hpp>
#include <raul/List.hpp>
#include <raul/Stateful.hpp>
#include <raul/MIDISink.hpp>
#include "Action.hpp"
#include "Schrodinbit.hpp"

namespace Machina {

class Edge;
using Raul::BeatCount;
using Raul::BeatTime;


/** A node is a state (as in a FSM diagram), or "note".
 *
 * It contains a action, as well as a duration and pointers to it's
 * successors (states/nodes that (may) follow it).
 *
 * Initial nodes do not have enter actions (since they are entered at
 * an undefined point in time <= 0).
 */
class Node : public Raul::Stateful {
public:
	typedef std::string ID;

	Node(BeatCount duration=0, bool initial=false);
	Node(const Node& copy);

	void set_enter_action(SharedPtr<Action> action);
	void set_exit_action(SharedPtr<Action> action);

	SharedPtr<Action> enter_action() { return _enter_action; }
	SharedPtr<Action> exit_action()  { return _exit_action; }

	void enter(SharedPtr<Raul::MIDISink> driver, BeatTime time);
	void exit(SharedPtr<Raul::MIDISink> driver, BeatTime time);

	void add_edge(SharedPtr<Edge> edge);
	void remove_edge(SharedPtr<Edge> edge);
	void remove_edges_to(SharedPtr<Node> node);
	bool connected_to(SharedPtr<Node> node);

	void write_state(Redland::Model& model);

	bool      is_initial() const        { return _is_initial; }
	void      set_initial(bool i)       { _is_initial = i; }
	bool      is_active() const         { return _is_active; }
	BeatTime  enter_time() const        { assert(_is_active); return _enter_time; }
	BeatTime  exit_time() const         { assert(_is_active); return _enter_time + _duration; }
	BeatCount duration()                { return _duration; }
	void      set_duration(BeatCount d) { _duration = d; }
	bool      is_selector() const       { return _is_selector; }
	void      set_selector(bool i);

	inline bool changed()     { return _changed; }
	inline void set_changed() { _changed = true; }
	
	typedef Raul::List<SharedPtr<Edge> > Edges;
	Edges& edges() { return _edges; }
	
	SharedPtr<Edge> random_edge();
	
private:
	Schrodinbit       _changed;
	bool              _is_initial;
	bool              _is_selector;
	bool              _is_active;
	BeatTime          _enter_time; ///< valid iff _is_active
	BeatCount         _duration;
	SharedPtr<Action> _enter_action;
	SharedPtr<Action> _exit_action;
	Edges             _edges;
};


} // namespace Machina

#endif // MACHINA_NODE_HPP
