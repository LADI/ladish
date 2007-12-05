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

#include <cstdlib>
#include <raul/SharedPtr.hpp>
#include <redlandmm/Model.hpp>
#include <redlandmm/World.hpp>
#include "machina/Edge.hpp"
#include "machina/Gene.hpp"
#include "machina/Machine.hpp"
#include "machina/MidiAction.hpp"
#include "machina/Node.hpp"

using namespace std;
using namespace Raul;

namespace Machina {


Machine::Machine()
	: _is_activated(false)
	, _is_finished(false)
	, _time(0)
{
}

	
Machine::Machine(SharedPtr<Gene> genotype)
	: _is_activated(false)
	, _is_finished(false)
	, _time(0)
	, _genotype(genotype)
{
}


Machine::~Machine()
{
}
	

SharedPtr<Gene>
Machine::genotype()
{
	if (_genotype)
		return _genotype;

	_genotype = SharedPtr<Gene>(new Gene(_nodes.size()));

	size_t node_id = 0;
	for (Nodes::iterator n = _nodes.begin(); n != _nodes.end(); ++n, ++node_id) {
		size_t edge_id = 0;
		for (Node::Edges::iterator e = (*n)->outgoing_edges().begin();
				e != (*n)->outgoing_edges().end(); ++e, ++edge_id) {
			(*_genotype.get())[node_id].push_back(edge_id);
		}
	}

	return _genotype;
}


/** Set the MIDI sink to be used for executing MIDI actions.
 *
 * MIDI actions will silently do nothing unless this call is passed an
 * existing Raul::MIDISink before running.
 */
void
Machine::set_sink(SharedPtr<Raul::MIDISink> sink)
{
	_sink = sink;
}


void
Machine::add_node(SharedPtr<Node> node)
{
	assert(_nodes.find(node) == _nodes.end());
	_nodes.push_back(node);
}


void
Machine::remove_node(SharedPtr<Node> node)
{
	_nodes.erase(_nodes.find(node));
	
	for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n)
		(*n)->remove_outgoing_edges_to(node);
}


/** Exit all active states and reset time to 0.
 */
void
Machine::reset(Raul::BeatTime time)
{
	for (size_t i=0; i < MAX_ACTIVE_NODES; ++i) {
		_active_nodes[i].reset();
	}

	if (!_is_finished) {
		for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n) {
			const SharedPtr<Node> node = (*n);

			if (node->is_active())
				node->exit(_sink.lock(), time);

			assert(! node->is_active());
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
	
	for (size_t i=0; i < MAX_ACTIVE_NODES; ++i) {
		const SharedPtr<Node> node = _active_nodes[i];
		
		if (node) {
			assert(node->is_active());
			if (!earliest || node->exit_time() < earliest->exit_time()) {
				earliest = node;
			}
		}
	}

	return earliest;
}


/** Enter a state at the current _time.
 * 
 * Returns true if node was entered, or false if the maximum active nodes has been reached.
 */
bool
Machine::enter_node(const SharedPtr<Raul::MIDISink> sink, const SharedPtr<Node> node)
{
	assert(!node->is_active());

	/* FIXME: Would be best to use the MIDI note here as a hash key, at least
	 * while all actions are still MIDI notes... */
	size_t index = (rand() % MAX_ACTIVE_NODES);
	for (size_t i=0; i < MAX_ACTIVE_NODES; ++i) {
		if (_active_nodes[index] == NULL) {
			node->enter(sink, _time);
			assert(node->is_active());
			_active_nodes[index] = node;
			return true;
		}
		index = (index + 1) % MAX_ACTIVE_NODES;
	}

	// If we get here, ran out of active node spots.  Don't enter node
	return false;
}



/** Exit an active node at the current _time.
 */
void
Machine::exit_node(SharedPtr<Raul::MIDISink> sink, const SharedPtr<Node> node)
{
	node->exit(sink, _time);
	assert(!node->is_active());

	for (size_t i=0; i < MAX_ACTIVE_NODES; ++i) {
		if (_active_nodes[i] == node) {
			_active_nodes[i].reset();
		}
	}

	// Activate successors to this node
	// (that aren't aready active right now)
	
	if (node->is_selector()) {

		const double rand_normal = rand() / (double)RAND_MAX; // [0, 1]
		double range_min = 0;

		for (Node::Edges::const_iterator s = node->outgoing_edges().begin();
				s != node->outgoing_edges().end(); ++s) {
			
			if (!(*s)->head()->is_active()
					&& rand_normal > range_min
					&& rand_normal < range_min + (*s)->probability()) {

				enter_node(sink, (*s)->head());
				break;

			} else {
				range_min += (*s)->probability();
			}
		}

	} else {

		for (Node::Edges::const_iterator s = node->outgoing_edges().begin();
				s != node->outgoing_edges().end(); ++s) {

			const double rand_normal = rand() / (double)RAND_MAX; // [0, 1]

			if (rand_normal <= (*s)->probability()) {
				SharedPtr<Node> head = (*s)->head();

				if ( ! head->is_active())
					enter_node(sink, head);
			}
		}

	}
}


/** Run the machine for @a nframes frames.
 *
 * Returns the duration of time the machine actually ran (from 0 to nframes).
 * 
 * Caller can check is_finished() to determine if the machine still has any
 * active states.  If not, time() will return the exact time stamp the
 * machine actually finished on (so it can be restarted immediately
 * with sample accuracy if necessary).
 */
BeatCount
Machine::run(const Raul::TimeSlice& time)
{
	if (_is_finished)
		return 0;

	const SharedPtr<Raul::MIDISink> sink = _sink.lock();

	const TickTime  cycle_end_ticks = time.start_ticks() + time.length_ticks() - 1;
	const BeatCount cycle_end_beats = time.ticks_to_beats(cycle_end_ticks);

	assert(_is_activated);

	// Initial run, enter all initial states
	if (_time == 0) {
		bool entered = false;
		if ( ! _nodes.empty()) {
			for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n) {
				
				if ((*n)->is_active())
					(*n)->exit(sink, 0);
				
				if ((*n)->is_initial()) {
					if (enter_node(sink, (*n)))
						entered = true;
				}

			}
		}
		if (!entered) {
			_is_finished = true;
			return 0;
		}
	}
	
	BeatCount this_time = 0;

	while (true) {

		SharedPtr<Node> earliest = earliest_node();

		// No more active states, machine is finished
		if (!earliest) {
#ifndef NDEBUG
			for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n)
				assert( ! (*n)->is_active());
#endif
			_is_finished = true;
			break;

		// Earliest active state ends this cycle
		} else if (time.beats_to_ticks(earliest->exit_time())
				<= cycle_end_ticks) {
			this_time += earliest->exit_time() - _time;
			_time = time.ticks_to_beats(
					time.beats_to_ticks(earliest->exit_time()));
			exit_node(sink, earliest);

		// Earliest active state ends in the future, done this cycle
		} else {
			_time = cycle_end_beats;
			this_time = time.length_beats(); // ran the entire cycle
			break;
		}

	}

	assert(this_time <= time.length_beats());
	return this_time;
}


/** Push a node onto the learn stack.
 *
 * NOT realtime (actions are allocated here).
 */
void
Machine::learn(SharedPtr<LearnRequest> learn)
{
	_pending_learn = learn;
}


void
Machine::write_state(Redland::Model& model)
{
	using namespace Raul;

	model.world().add_prefix("machina", "http://drobilla.net/ns/machina#");

	model.add_statement(model.base_uri(),
			Redland::Node(model.world(), Redland::Node::RESOURCE, "rdf:type"),
			Redland::Node(model.world(), Redland::Node::RESOURCE, "machina:Machine"));

	size_t count = 0;

	for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n) {
	
		(*n)->write_state(model);

		if ((*n)->is_initial()) {
			model.add_statement(model.base_uri(),
					Redland::Node(model.world(), Redland::Node::RESOURCE, "machina:initialNode"),
					(*n)->id());
		} else {
			model.add_statement(model.base_uri(),
					Redland::Node(model.world(), Redland::Node::RESOURCE, "machina:node"),
					(*n)->id());
		}
	}

	count = 0;

	for (Nodes::const_iterator n = _nodes.begin(); n != _nodes.end(); ++n) {
		
		for (Node::Edges::const_iterator e = (*n)->outgoing_edges().begin();
			e != (*n)->outgoing_edges().end(); ++e) {
			
			(*e)->write_state(model);
		
			model.add_statement(model.base_uri(),
				Redland::Node(model.world(), Redland::Node::RESOURCE, "machina:edge"),
				(*e)->id());
		}

	}
}


} // namespace Machina

