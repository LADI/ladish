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
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <iostream>
#include <ios>
#include "machina/Recorder.hpp"
#include "machina/MachineBuilder.hpp"

using namespace std;
using namespace Raul;

namespace Machina {


Recorder::Recorder(size_t buffer_size, double tick_rate, double q)
	: _tick_rate(tick_rate)
	, _record_buffer(buffer_size)
	, _builder(new MachineBuilder(SharedPtr<Machine>(new Machine()), q))
{
}


void
Recorder::_whipped()
{
	TickTime      t;
	size_t        size;
	unsigned char buf[4];

	while (_record_buffer.read(&t, &size, buf)) {
		_builder->set_time(t * _tick_rate);
		_builder->event(0, size, buf);
	}
} 


SharedPtr<Machine>
Recorder::finish()
{
	SharedPtr<Machine> machine = _builder->finish();
	_builder.reset();
	return machine;
}


}

