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


Machine::Machine(size_t poly)
	: _initial_node(new Node())
	, _voices(poly, NULL)//_initial_node)
	, _time(0)
{
	/* reserve poly spaces in _voices, so accessing it
	 * with operator[] should be realtime safe.
	 */
}


Machine::~Machine()
{
	delete _initial_node;
}


void
Machine::reset()
{
	for (std::vector<Node*>::iterator i = _voices.begin();
			i != _voices.end(); ++i) {
		*i = NULL;
	}
}


void
Machine::process(FrameCount nframes)
{
	const FrameCount cycle_end = _time + nframes;
	bool             done      = false;

	FrameCount latest_event = _time;

	std::cerr << "--------- " << _time << " - " << _time + nframes << std::endl;

	// FIXME: way too much iteration
	
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
}


} // namespace Machina

