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

#include <iostream>
#include <cstdlib>
#include "machina/Edge.hpp"
#include "machina/Machine.hpp"
#include "machina/Mutation.hpp"
#include "machina/ActionFactory.hpp"

using namespace std;

namespace Machina {
namespace Mutation {


void
Compress::mutate(Machine& machine)
{
	//cout << "COMPRESS" << endl;

	// Trim disconnected nodes
	for (Machine::Nodes::iterator i = machine.nodes().begin(); i != machine.nodes().end() ;) {
		Machine::Nodes::iterator next = i;
		++next;
		
		if ((*i)->edges().empty())
			machine.remove_node(*i);

		i = next;
	}
}


void
AddNode::mutate(Machine& machine)
{
	//cout << "ADD NODE" << endl;

	// Create random node
	SharedPtr<Node> node(new Node(1.0));
	uint8_t note = rand() % 128;
	node->set_enter_action(ActionFactory::note_on(note));
	node->set_exit_action(ActionFactory::note_off(note));
	machine.add_node(node);
	
	// Add as a successor to some other random node
	SharedPtr<Node> tail = machine.random_node();
	if (tail && tail != node)
		tail->add_edge(boost::shared_ptr<Edge>(new Edge(tail, node)));
}


void
RemoveNode::mutate(Machine& machine)
{
	//cout << "REMOVE NODE" << endl;

	SharedPtr<Node> node = machine.random_node();
	machine.remove_node(node);
}

	
void
AdjustNode::mutate(Machine& machine)
{
	//cout << "ADJUST NODE" << endl;

	SharedPtr<Node> node = machine.random_node();
	if (node) {
		SharedPtr<MidiAction> enter_action = PtrCast<MidiAction>(node->enter_action());
		SharedPtr<MidiAction> exit_action  = PtrCast<MidiAction>(node->exit_action());
		if (enter_action && exit_action) {
			const uint8_t note = rand() % 128;
			enter_action->event()[1] = note;
			exit_action->event()[1] = note;
		}
		node->set_changed();
	}
}


void
AddEdge::mutate(Machine& machine)
{
	//cout << "ADJUST EDGE" << endl;

	SharedPtr<Node> tail = machine.random_node();
	SharedPtr<Node> head = machine.random_node();

	if (tail && head && tail != head && !tail->connected_to(head))
		tail->add_edge(boost::shared_ptr<Edge>(new Edge(tail, head)));
}


void
RemoveEdge::mutate(Machine& machine)
{
	//cout << "REMOVE EDGE" << endl;

	SharedPtr<Node> tail = machine.random_node();
	if (tail)
		tail->remove_edge(tail->random_edge());
}

	
void
AdjustEdge::mutate(Machine& machine)
{
	//cout << "ADJUST EDGE" << endl;

	SharedPtr<Edge> edge = machine.random_edge();
	if (edge)
		edge->set_probability(rand() / (float)RAND_MAX);
}


} // namespace Mutation
} // namespace Machina

