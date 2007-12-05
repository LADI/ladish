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

#include <cassert>
#include <raul/Atom.hpp>
#include <redlandmm/World.hpp>
#include <redlandmm/Model.hpp>
#include <machina/Node.hpp>
#include <machina/Edge.hpp>

namespace Machina {


Node::Node(BeatCount duration, bool initial)
	: _is_initial(initial)
	, _is_selector(false)
	, _is_active(false)
	, _enter_time(0)
	, _duration(duration)
{
}


/** Always returns an edge, unless there are none */
SharedPtr<Edge>
Node::random_edge()
{
	SharedPtr<Edge> ret;
	if (_edges.empty())
		return ret;
	
	size_t i = rand() % _edges.size();

	// FIXME: O(n) worst case :(
	for (Edges::const_iterator e = _edges.begin(); e != _edges.end(); ++e, --i) {
		if (i == 0) {
			ret = *e;
			break;
		}
	}

	return ret;
}


void
Node::set_selector(bool yn)
{
	_is_selector = yn;

	if (yn) {
		double prob_sum = 0;
		for (Edges::iterator i = _edges.begin(); i != _edges.end(); ++i)
			prob_sum += (*i)->probability();
	
		for (Edges::iterator i = _edges.begin(); i != _edges.end(); ++i)
			(*i)->set_probability((*i)->probability() / prob_sum);
	}
	_changed = true;
}


void
Node::set_enter_action(SharedPtr<Action> action)
{
	_enter_action = action;
	_changed = true;
}


void
Node::set_exit_action(SharedPtr<Action> action)
{
	_exit_action = action;
	_changed = true;
}


void
Node::enter(SharedPtr<Raul::MIDISink> sink, BeatTime time)
{
	assert(!_is_active);
	
	_changed = true;
	_is_active = true;
	_enter_time = time;
	
	if (sink && _enter_action)
		_enter_action->execute(sink, time);
}


void
Node::exit(SharedPtr<Raul::MIDISink> sink, BeatTime time)
{
	assert(_is_active);
	
	if (sink && _exit_action)
		_exit_action->execute(sink, time);
	
	_changed = true;
	_is_active = false;
	_enter_time = 0;
}


void
Node::add_outgoing_edge(SharedPtr<Edge> edge)
{
	assert(edge->tail().lock().get() == this);
	
	_edges.push_back(edge);
}


void
Node::remove_outgoing_edge(SharedPtr<Edge> edge)
{
	_edges.erase(_edges.find(edge));
}


void
Node::remove_edges_to(SharedPtr<Node> node)
{
	for (Edges::iterator i = _edges.begin(); i != _edges.end() ; ) {
		Edges::iterator next = i;
		++next;

		if ((*i)->head() == node)
			_edges.erase(i);

		i = next;
	}
}


void
Node::write_state(Redland::Model& model)
{
	using namespace Raul;
	
	if (!_id)
		set_id(model.world().blank_id());

	if (_is_selector)
		model.add_statement(_id,
				"rdf:type",
				Redland::Node(model.world(), Redland::Node::RESOURCE, "machina:SelectorNode"));
	else
		model.add_statement(_id,
				"rdf:type",
				Redland::Node(model.world(), Redland::Node::RESOURCE, "machina:Node"));

	model.add_statement(_id,
			"machina:duration",
			Raul::Atom((float)_duration).to_rdf_node(model.world()));

	if (_enter_action) {
		_enter_action->write_state(model);
		
		model.add_statement(_id,
				"machina:enterAction",
				_enter_action->id());
	}

	if (_exit_action) {
		_exit_action->write_state(model);

		model.add_statement(_id,
				"machina:exitAction",
				_exit_action->id());
	}
}


} // namespace Machina

