/* This file is part of Raul.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Raul is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Raul is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef STATEFUL_H
#define STATEFUL_H

#include "redlandmm/Model.hpp"

namespace Raul {


class Stateful {
public:
	virtual ~Stateful() {}

	virtual void write_state(Redland::Model& model) = 0;
	
	Redland::Node id() const                      { return _id; }
	void          set_id(const Redland::Node& id) { _id = id; }

protected:
	Redland::Node _id;
};
	

} // namespace Raul

#endif // STATEFUL_H
