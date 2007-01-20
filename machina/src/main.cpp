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

#include <iostream>
#include "MetaState.hpp"
#include "Node.hpp"
#include "Action.hpp"
#include "Edge.hpp"

using namespace std;
using namespace Machina;


Node* create_debug_node(const string& name, FrameCount duration)
{
	// leaks like a sieve, obviously
	
	Node* n = new Node(duration);
	PrintAction* a_enter = new PrintAction(string("> ") + name);
	PrintAction* a_exit = new PrintAction(string("< ")/* + name*/);

	n->add_enter_action(a_enter);
	n->add_exit_action(a_exit);

	return n;
}

	
int
main()//int argc, char** argv)
{
	MetaState ms(1);

	Node* n1 = create_debug_node("1", 1);
	Node* n2 = create_debug_node("2", 10);

	ms.initial_node()->add_outgoing_edge(new Edge(n1));
	n1->add_outgoing_edge(new Edge(n2));
	n2->add_outgoing_edge(new Edge(ms.initial_node()));

	Timestamp t = 0;

	while (t < 80) {
		ms.process(10);
		t += 10;
	}

	return 0;
}


