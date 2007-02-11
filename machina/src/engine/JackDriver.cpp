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
#include <iomanip>
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
	Raul::JackDriver::attach(client_name);

	if (jack_client()) {
		_input_port = jack_port_register(jack_client(),
			"in",
			JACK_DEFAULT_MIDI_TYPE, JackPortIsInput,
			0);
		
		if (!_input_port)
			std:: cerr << "WARNING: Failed to create MIDI input port." << std::endl;

		_output_port = jack_port_register(jack_client(),
			"out",
			JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput,
			0);
		
		if (!_output_port)
			std::cerr << "WARNING: Failed to create MIDI output port." << std::endl;
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
JackDriver::process_input(jack_nframes_t nframes)
{
	using namespace std;

	//if (_learn_node) {
		void*                jack_buffer = jack_port_get_buffer(_input_port, nframes);
		const jack_nframes_t event_count = jack_midi_get_event_count(jack_buffer, nframes);

		for (jack_nframes_t i=0; i < event_count; ++i) {
			jack_midi_event_t ev;
			jack_midi_event_get(&ev, jack_buffer, i, nframes);

			if (ev.buffer[0] == 0x90) {
				cerr << "NOTE ON\n";

				const SharedPtr<LearnRequest> learn = _machine->pending_learn();
				if (learn) {
					learn->enter_action()->set_event(ev.size, ev.buffer);
					cerr << "LEARN START\n";
					learn->start(jack_last_frame_time(_client) +  ev.time);
					//LearnRecord learn = _machine->pop_learn();
				}

			} else if (ev.buffer[0] == 0x80) {
				cerr << "NOTE OFF\n";
				
				const SharedPtr<LearnRequest> learn = _machine->pending_learn();
				
				if (learn) {
					if (learn->started()) {
						learn->exit_action()->set_event(ev.size, ev.buffer);
						learn->finish(jack_last_frame_time(_client) + ev.time);
						_machine->clear_pending_learn();
						cerr << "LEARNED!\n";
					}
				}
			}

			//std::cerr << "EVENT: " << std::hex << (int)ev.buffer[0] << "\n";

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

	assert(_output_port);
	jack_midi_event_write(
		jack_port_get_buffer(_output_port, nframes), offset,
		event, size, nframes);
}


void
JackDriver::on_process(jack_nframes_t nframes)
{
	using namespace std;
	//std::cerr << "> ======================================================\n";

	_current_cycle_offset  = 0;
	_current_cycle_nframes = nframes;

	assert(_output_port);
	jack_midi_clear_buffer(jack_port_get_buffer(_output_port, nframes), nframes);

	process_input(nframes);

	if (_machine->is_empty()) {
		//cerr << "EMPTY\n";
		return;
	}

	while (true) {
	
		const FrameCount run_duration = _machine->run(_current_cycle_nframes);

		// Machine didn't run at all (empty, or no initial states)
		if (run_duration == 0) {
			_machine->reset(); // Try again next cycle
			_current_cycle_start = 0;
			return;
		}

		// Machine ran for portion of cycle
		else if (run_duration < _current_cycle_nframes) {
			const Timestamp  finish_time   = _machine->time();
			const FrameCount finish_offset = stamp_to_offset(finish_time);
			
			if (finish_offset >= _current_cycle_nframes)
				break;

			_current_cycle_offset = stamp_to_offset(finish_time);
			_current_cycle_nframes -= _current_cycle_offset;
			_current_cycle_start = 0;
			_machine->reset();
		
		// Machine ran for entire cycle
		} else {
			if (_machine->is_finished())
				_machine->reset();

			_current_cycle_start += _current_cycle_nframes;
			break;
		}
	}
	
	//std::cerr << "< ======================================================\n";
}


} // namespace Machina
