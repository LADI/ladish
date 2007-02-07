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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "JackDriver.hpp"

#include <iostream>

namespace Machina {


JackDriver::JackDriver()
	: _output_port(NULL)
	, _current_cycle_start(0)
	, _current_cycle_nframes(0)
{
}


void
JackDriver::attach(const std::string& client_name)
{
	Raul::JackDriver::attach(client_name);

	if (jack_client()) {
		_output_port = jack_port_register(jack_client(),
			"out",
			JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput,
			0);
	}
}


void
JackDriver::detach()
{
	jack_port_unregister(jack_client(), _output_port);
	_output_port = NULL;

	Raul::JackDriver::detach();
}


Timestamp
JackDriver::stamp_to_offset(Timestamp stamp)
{
	Timestamp ret = stamp - _current_cycle_start + _current_cycle_offset;
	assert(ret < _current_cycle_offset + _current_cycle_nframes);
	return ret;
}


void
JackDriver::on_process(jack_nframes_t nframes)
{
	//std::cerr << "======================================================\n";

	_current_cycle_offset  = 0;
	_current_cycle_nframes = nframes;

	jack_midi_clear_buffer(jack_port_get_buffer(_output_port, nframes), nframes);

	while (true) {
	
		bool machine_done = ! _machine->run(_current_cycle_nframes);

		if (!machine_done) {
			_current_cycle_start += _current_cycle_nframes;
			break;

		} else {
			const Timestamp  finish_time   = _machine->time();
			const FrameCount finish_offset = stamp_to_offset(finish_time);
			
			if (finish_offset >= _current_cycle_nframes)
				break;

			_current_cycle_offset = stamp_to_offset(finish_time);
			_current_cycle_nframes -= _current_cycle_offset;
			_current_cycle_start = 0;
			_machine->reset();
		}
	}
	
	//std::cerr << "======================================================\n";
}


} // namespace Machina
