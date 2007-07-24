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

#include <raul/RDFWorld.hpp>
#include <raul/RDFModel.hpp>
#include "machina/Action.hpp"

namespace Machina {

void
Action::write_state(Raul::RDF::Model& model)
{
	using namespace Raul;
	
	if (!_id)
		set_id(model.world().blank_id());

	model.add_statement(_id,
			RDF::Node(model.world(), RDF::Node::RESOURCE, "rdf:type"),
			RDF::Node(model.world(), RDF::Node::RESOURCE, "machina:Action"));
}


} // namespace Machina

