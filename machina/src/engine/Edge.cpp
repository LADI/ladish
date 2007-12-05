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

#include <raul/Atom.hpp>
#include <redlandmm/World.hpp>
#include <redlandmm/Model.hpp>
#include "machina/Node.hpp"
#include "machina/Edge.hpp"

namespace Machina {


void
Edge::write_state(Redland::Model& model)
{
	using namespace Raul;

	if (!_id)
		set_id(model.world().blank_id());

	model.add_statement(_id,
			"rdf:type",
			Redland::Node(model.world(), Redland::Node::RESOURCE, "machina:Edge"));

	SharedPtr<Node> tail = _tail.lock();
	SharedPtr<Node> head = _head;

	if (!tail || !head)
		return;

	assert(tail->id() && head->id());

	model.add_statement(_id,
			"machina:tail",
			tail->id());

	model.add_statement(_id,
			"machina:head",
			head->id());
	
	model.add_statement(_id,
			"machina:probability",
			Atom(_probability.get()).to_rdf_node(model.world()));
}


} // namespace Machina
