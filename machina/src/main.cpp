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
#include "Machine.hpp"
#include "Node.hpp"
#include "Action.hpp"
#include "Edge.hpp"
#include "Loader.hpp"

using namespace std;
using namespace Machina;


int
main(int argc, char** argv)
{
	if (argc != 2)
		return -1;

	Loader l;
	SharedPtr<Machine> m = l.load(argv[1]);

	m->activate();

	/*
	Machine m(1);

	Node* n1 = create_debug_node("1", 1);
	Node* n2 = create_debug_node("2", 10);

	m.initial_node()->add_outgoing_edge(new Edge(n1));
	n1->add_outgoing_edge(new Edge(n2));
	n2->add_outgoing_edge(new Edge(m.initial_node()));
	*/

	Timestamp t = 0;

	while (t < 4000) {
		m->process(1000);
		t += 1000;
	}

	return 0;
}


