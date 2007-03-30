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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <raul/RDFWriter.h>
#include "machina/Node.hpp"
#include "machina/Edge.hpp"

namespace Machina {


void
Edge::write_state(Raul::RDFWriter& writer)
{
	using Raul::RdfId;

	if (!_id)
		set_id(writer.blank_id());

	writer.write(_id,
			RdfId(RdfId::RESOURCE, "rdf:type"),
			RdfId(RdfId::RESOURCE, "machina:Edge"));

	SharedPtr<Node> tail = _tail.lock();
	SharedPtr<Node> head = _head;

	if (!tail || !head)
		return;

	assert(tail->id() && head->id());

	writer.write(_id,
			RdfId(RdfId::RESOURCE, "machina:tail"),
			tail->id());

	writer.write(_id,
			RdfId(RdfId::RESOURCE, "machina:head"),
			head->id());
	
	writer.write(_id,
			RdfId(RdfId::RESOURCE, "machina:probability"),
			_probability.get());
}


} // namespace Machina
