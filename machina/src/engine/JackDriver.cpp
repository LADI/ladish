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

#include <iostream>
#include "machina/JackDriver.hpp"
#include "machina/MidiAction.hpp"

namespace Machina {


JackDriver::JackDriver()
	: _input_port(NULL)
	, _output_port(NULL)
	, _current_cycle_start(0)
	, _current_cycle_nframes(0)
{
}


void
JackDriver::attach(const std::string& client_name)
{
	Raul::JackDriver::attach(client_name, "debug");

	if (jack_client()) {
		_input_port = jack_port_register(jack_client(),
			"out",
			JACK_DEFAULT_MIDI_TYPE, JackPortIsInput,
			0);

		_output_port = jack_port_register(jack_client(),
			"out",
			JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput,
			0);
	}
}


void
JackDriver::detach()
{
	jack_port_unregister(jack_client(), _input_port);
	jack_port_unregister(jack_client(), _output_port);
	_input_port = NULL;
	_output_port = NULL;

	Raul::JackDriver::detach();
}


Timestamp
JackDriver::stamp_to_offset(Timestamp stamp)
{
	assert(stamp >= _current_cycle_start);

	Timestamp ret = stamp - _current_cycle_start + _current_cycle_offset;
	assert(ret < _current_cycle_offset + _current_cycle_nframes);
	return ret;
}


void
JackDriver::learn(SharedPtr<Node> node)
{
	_learn_enter_action = SharedPtr<MidiAction>(new MidiAction(shared_from_this(), 4, NULL));
	_learn_exit_action = SharedPtr<MidiAction>(new MidiAction(shared_from_this(), 4, NULL));
	_learn_node = node;
}


void
JackDriver::process_input(jack_nframes_t nframes)
{
	//if (_learn_node) {
		void*                jack_buffer = jack_port_get_buffer(_input_port, nframes);
		const jack_nframes_t event_count = jack_midi_get_event_count(jack_buffer, nframes);

		for (jack_nframes_t i=0; i < event_count; ++i) {
			jack_midi_event_t ev;
			jack_midi_event_get(&ev, jack_buffer, i, nframes);

			std::cerr << "EVENT: " << (char)ev.buffer[0] << "\n";

		}
	//}
}


void
JackDriver::write_event(Timestamp   time,
                        size_t      size,
                        const byte* event)
{
	const FrameCount nframes = _current_cycle_nframes;
	const FrameCount offset  = stamp_to_offset(time);

	jack_midi_event_write(
		jack_port_get_buffer(_output_port, nframes), offset,
		event, size, nframes);
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

		if (machine_done && _machine->is_finished())
			return;

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
