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

#ifndef MACHINA_NODE_HPP
#define MACHINA_NODE_HPP

#include <list>
#include <boost/utility.hpp>
#include <raul/SharedPtr.h>
#include "types.hpp"
#include "Action.hpp"

namespace Machina {

class Edge;


/** A node is a state (as in a FSM diagram), or "note".
 *
 * It contains a action, as well as a duration and pointers to it's
 * successors (states/nodes that (may) follow it).
 *
 * Initial nodes do not have enter actions (since they are entered at
 * an undefined point in time <= 0).
 */
class Node : public boost::noncopyable {
public:
	typedef std::string ID;

	Node(FrameCount duration=0, bool initial=false);

	void add_enter_action(Action* action);
	void remove_enter_action(Action* action);
	
	void add_exit_action(Action* action);
	void remove_exit_action(Action* action);

	void enter(Timestamp time);
	void exit(Timestamp time);

	void add_outgoing_edge(SharedPtr<Edge> edge);
	void remove_outgoing_edge(SharedPtr<Edge> edge);

	bool       is_initial() const         { return _is_initial; }
	void       set_initial(bool i)        { _is_initial = i; }
	bool       is_active() const          { return _is_active; }
	Timestamp  enter_time() const         { return _enter_time; }
	Timestamp  exit_time() const          { return _enter_time + _duration; }
	FrameCount duration()                 { return _duration; }
	void       set_duration(FrameCount d) { _duration = d; }
	
	typedef std::list<SharedPtr<Edge> > EdgeList;
	const EdgeList& outgoing_edges() const { return _outgoing_edges; }
	
private:
	bool       _is_initial;
	bool       _is_active;
	Timestamp  _enter_time; ///< valid iff _is_active
	FrameCount _duration;
	Action*    _enter_action;
	Action*    _exit_action;
	EdgeList   _outgoing_edges;
};


} // namespace Machina

#endif // MACHINA_NODE_HPP
