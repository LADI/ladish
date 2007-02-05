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

#include <algorithm>
#include "Machine.hpp"
#include "Node.hpp"
#include "Edge.hpp"

namespace Machina {


Machine::Machine()
	: _is_activated(false)
	, _is_finished(false)
	, _time(0)
{
}


Machine::~Machine()
{
}


void
Machine::add_node(SharedPtr<Node> node)
{
	assert(!_is_activated);

	_nodes.push_back(node);
}


/** Exit all active states and reset time to 0.
 */
void
Machine::reset()
{
	if (!_is_finished) {
		for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n) {
			const SharedPtr<Node> node = (*n);

			if (node->is_active())
				node->exit(_time);
		}
	}

	_time = 0;
	_is_finished = false;
}


/** Return the active Node with the earliest exit time.
 */
SharedPtr<Node>
Machine::earliest_node() const
{	
	SharedPtr<Node> earliest;
	
	for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n) {
		const SharedPtr<Node> node = (*n);
		
		if (node->is_active())
			if (!earliest || node->exit_time() < earliest->exit_time())
				earliest = node;
	}

	return earliest;
}


/** Exit an active node at the current _time.
 */
void
Machine::exit_node(const SharedPtr<Node> node)
{
	node->exit(_time);

	// Activate all successors to this node
	// (that aren't aready active right now)
	for (Node::EdgeList::const_iterator s = node->outgoing_edges().begin();
			s != node->outgoing_edges().end(); ++s) {
		SharedPtr<Node> dst = (*s)->dst();

		if (!dst->is_active())
			dst->enter(_time);

	}
}


/** Run the machine for @a nframes frames.
 *
 * Returns false when the machine has finished running (i.e. there are
 * no currently active states).
 *
 * If this returns false, time() will return the exact time stamp the
 * machine actually finished on (so it can be restarted immediately
 * with sample accuracy if necessary).
 */
bool
Machine::run(FrameCount nframes)
{
	if (_is_finished)
		return false;

	const FrameCount cycle_end = _time + nframes;

	assert(_is_activated);

	//std::cerr << "--------- " << _time << " - " << _time + nframes << std::endl;

	// Initial run, enter all initial states
	if (_time == 0)
		for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n)
			if ((*n)->is_initial())
				(*n)->enter(0);
	
	while (true) {

		SharedPtr<Node> earliest = earliest_node();

		// No more active states, machine is finished
		if (!earliest) {
			_is_finished = true;
			return false;

		// Earliest active state ends this cycle
		} else if (earliest->exit_time() < cycle_end) {
			_time = earliest->exit_time();
			exit_node(earliest);

		// Earliest active state ends in the future, done this cycle
		} else {
			_time = cycle_end;
			return true;
		}

	}

#if 0
	while (!done) {

		done = true;

		for (std::vector<Node*>::iterator i = _voices.begin();
				i != _voices.end(); ++i) {
			
			Node* const n = *i;

			// Active voice which ends within this cycle, transition
			if (n && n->is_active() && n->end_time() < cycle_end) {
				// Guaranteed to be within this cycle
				const FrameCount end_time = std::max(_time, n->end_time());
				n->exit(std::max(_time, n->end_time()));
				done = false;

				// Greedily grab one of the successors with the voice already
				// on this node so voices follow paths nicely
				for (Node::EdgeList::const_iterator s = n->outgoing_edges().begin();
						s != n->outgoing_edges().end(); ++s) {
					Node* dst = (*s)->dst();
					if (!dst->is_active()) {
						dst->enter(end_time);
						*i = dst;
						break;
					}
				}

				latest_event = end_time;
			}

		}

		// FIXME: use free voices to claim any 'free successors'
		// (when nodes have multiple successors and one gets chosen in the
		// greedy bit above)

		// If every voice is on the initial node...
		bool is_reset = true;
		for (std::vector<Node*>::iterator i = _voices.begin();
				i != _voices.end(); ++i)
			if ((*i) != NULL && (*i)->is_active())
				is_reset = false;

		// ... then start
		if (is_reset) {

			std::vector<Node*>::iterator n = _voices.begin();
			for (Node::EdgeList::const_iterator s = _initial_node->outgoing_edges().begin();
					s != _initial_node->outgoing_edges().end() && n != _voices.end();
					++s, ++n) {
				(*s)->dst()->enter(latest_event);
				done = false;
				*n = (*s)->dst();
			}
		}
	}
	_time += nframes;

	return false;
#endif
}


} // namespace Machina

