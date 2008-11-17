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
	SharedPtr<Node> node(new Node(machine.time().unit()));
	node->set_selector(true);
	
	SharedPtr<Node> note_node = machine.random_node();
	if (!note_node)
		return;

	uint8_t note = rand() % 128;

	SharedPtr<MidiAction> enter_action = PtrCast<MidiAction>(note_node->enter_action());
	if (enter_action)
		note = enter_action->event()[1];

	node->set_enter_action(ActionFactory::note_on(note));
	node->set_exit_action(ActionFactory::note_off(note));
	machine.add_node(node);
	
	// Insert after some node
	SharedPtr<Node> tail = machine.random_node();
	if (tail && tail != node/* && !node->connected_to(tail)*/)
		tail->add_edge(boost::shared_ptr<Edge>(new Edge(tail, node)));
	
	// Insert before some other node
	SharedPtr<Node> head = machine.random_node();
	if (head && head != node/* && !head->connected_to(node)*/)
		node->add_edge(boost::shared_ptr<Edge>(new Edge(node, head)));
}


void
RemoveNode::mutate(Machine& machine)
{
	//cout << "REMOVE NODE" << endl;

	SharedPtr<Node> node = machine.random_node();
	if (node && !node->is_initial())
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
SwapNodes::mutate(Machine& machine)
{
	//cout << "SWAP NODE" << endl;
	
	if (machine.nodes().size() <= 1)
		return;

	SharedPtr<Node> a = machine.random_node();
	SharedPtr<Node> b = machine.random_node();
	while (b == a)
		b = machine.random_node();

	SharedPtr<MidiAction> a_enter = PtrCast<MidiAction>(a->enter_action());
	SharedPtr<MidiAction> a_exit  = PtrCast<MidiAction>(a->exit_action());
	SharedPtr<MidiAction> b_enter = PtrCast<MidiAction>(b->enter_action());
	SharedPtr<MidiAction> b_exit  = PtrCast<MidiAction>(b->exit_action());

	uint8_t note_a = a_enter->event()[1];
	uint8_t note_b = b_enter->event()[1];

	a_enter->event()[1] = note_b;
	a_exit->event()[1] = note_b;
	b_enter->event()[1] = note_a;
	b_exit->event()[1] = note_a;
}


void
AddEdge::mutate(Machine& machine)
{
	//cout << "ADJUST EDGE" << endl;

	SharedPtr<Node> tail = machine.random_node();
	SharedPtr<Node> head = machine.random_node();

	if (tail && head && tail != head/* && !tail->connected_to(head) && !head->connected_to(tail)*/) {
		SharedPtr<Edge> edge(new Edge(tail, head));
		edge->set_probability(rand() / (float)RAND_MAX);
		tail->add_edge(boost::shared_ptr<Edge>(new Edge(tail, head)));
	}
}


void
RemoveEdge::mutate(Machine& machine)
{
	//cout << "REMOVE EDGE" << endl;

	SharedPtr<Node> tail = machine.random_node();
	if (tail && !(tail->is_initial() && tail->edges().size() == 1))
		tail->remove_edge(tail->random_edge());
}

	
void
AdjustEdge::mutate(Machine& machine)
{
	//cout << "ADJUST EDGE" << endl;

	SharedPtr<Edge> edge = machine.random_edge();
	if (edge) {
		edge->set_probability(rand() / (float)RAND_MAX);
		edge->tail().lock()->edges_changed();
	}
}


} // namespace Mutation
} // namespace Machina

