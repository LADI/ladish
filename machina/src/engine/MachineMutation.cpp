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
#include "machina/MachineMutation.hpp"

using namespace std;

namespace Machina {
namespace Mutation {


void
AddEdge::mutate(Machine& machine)
{
	cout << "ADD" << endl;
}


void
RemoveEdge::mutate(Machine& machine)
{
	cout << "REMOVE" << endl;
}

	
void
AdjustEdge::mutate(Machine& machine)
{
	SharedPtr<Edge> edge = machine.random_edge();
	if (edge)
		edge->set_probability(rand() / (float)RAND_MAX);
}


} // namespace Mutation
} // namespace Machina

