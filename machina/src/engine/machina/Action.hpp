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

#ifndef MACHINA_ACTION_HPP
#define MACHINA_ACTION_HPP

#include <string>
#include <iostream>
#include <raul/MIDISink.hpp>
#include <raul/TimeSlice.hpp>
#include <raul/Stateful.hpp>
#include <raul/SharedPtr.hpp>
#include "types.hpp"

namespace Machina {


/** An Action, executed on entering or exiting of a state.
 */
struct Action : public Raul::Deletable, public Raul::Stateful {
	virtual void execute(SharedPtr<Raul::MIDISink> sink, Raul::BeatTime time) = 0;

	virtual void write_state(Raul::RDF::Model& model);
};


class PrintAction : public Action {
public:
	PrintAction(const std::string& msg) : _msg(msg) {}

	void execute(SharedPtr<Raul::MIDISink>, Raul::BeatTime time)
	{ std::cout << "t=" << time << ": " << _msg << std::endl; }

private:
	std::string _msg;
};


} // namespace Machina

#endif // MACHINA_ACTION_HPP
