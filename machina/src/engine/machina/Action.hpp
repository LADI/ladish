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

#ifndef MACHINA_ACTION_HPP
#define MACHINA_ACTION_HPP

#include <string>
#include <iostream>
#include <raul/Deletable.h>
#include "types.hpp"

namespace Machina {


/** An Action, executed on entering or exiting of a state.
 *
 * Actions do not have time as a property.
 */
struct Action : public Raul::Deletable {
	virtual void execute(Timestamp /*time*/) {}
};


class PrintAction : public Action {
public:
	PrintAction(const std::string& msg) : _msg(msg) {}

	void execute(Timestamp time) { std::cout << "t=" << time << ": " << _msg << std::endl; }

private:
	std::string _msg;
};


} // namespace Machina

#endif // MACHINA_ACTION_HPP
