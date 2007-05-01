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
#include <raul/RDFWorld.h>
#include <raul/RDFModel.h>
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


void
Node::set_selector(bool yn)
{
	_is_selector = yn;

	if (yn) {
		double prob_sum = 0;
		for (Edges::iterator i = _outgoing_edges.begin(); i != _outgoing_edges.end(); ++i)
			prob_sum += (*i)->probability();
	
		for (Edges::iterator i = _outgoing_edges.begin(); i != _outgoing_edges.end(); ++i)
			(*i)->set_probability((*i)->probability() / prob_sum);
	}
}


void
Node::set_enter_action(SharedPtr<Action> action)
{
	_enter_action = action;
}


void
Node::remove_enter_action()
{
	_enter_action.reset();
}



void
Node::set_exit_action(SharedPtr<Action> action)
{
	_exit_action = action;
}


void
Node::remove_exit_action()
{
	_exit_action.reset();
}


void
Node::enter(SharedPtr<Raul::MIDISink> sink, BeatTime time)
{
	assert(!_is_active);
	
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
	_is_active = false;
	_enter_time = 0;
}


void
Node::add_outgoing_edge(SharedPtr<Edge> edge)
{
	assert(edge->tail().lock().get() == this);
	
	_outgoing_edges.push_back(edge);
}


void
Node::remove_outgoing_edge(SharedPtr<Edge> edge)
{
	_outgoing_edges.erase(_outgoing_edges.find(edge));
}


void
Node::remove_outgoing_edges_to(SharedPtr<Node> node)
{
	for (Edges::iterator i = _outgoing_edges.begin(); i != _outgoing_edges.end() ; ) {
		Edges::iterator next = i;
		++next;

		if ((*i)->head() == node)
			_outgoing_edges.erase(i);

		i = next;
	}
}


void
Node::write_state(Raul::RDF::Model& model)
{
	using namespace Raul;
	
	if (!_id)
		set_id(model.world().blank_id());

	if (_is_selector)
		model.add_statement(_id,
				"rdf:type",
				RDF::Node(model.world(), RDF::Node::RESOURCE, "machina:SelectorNode"));
	else
		model.add_statement(_id,
				"rdf:type",
				RDF::Node(model.world(), RDF::Node::RESOURCE, "machina:Node"));

	model.add_statement(_id,
			"machina:duration",
			Raul::Atom((float)_duration));

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

