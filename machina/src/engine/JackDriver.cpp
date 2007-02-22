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

using namespace Raul;

namespace Machina {


JackDriver::JackDriver()
	: _input_port(NULL)
	, _output_port(NULL)
	, _cycle_time(1/48000.0, 120.0)
	, _bpm(120.0)
	, _quantization(120.0)
{
}


void
JackDriver::attach(const std::string& client_name)
{
	Raul::JackDriver::attach(client_name);

	if (jack_client()) {

		_cycle_time.set_tick_rate(1/(double)sample_rate());
		
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
	if (_input_port) {
		jack_port_unregister(jack_client(), _input_port);
		_input_port = NULL;
	}
	
	if (_output_port) {
		jack_port_unregister(jack_client(), _output_port);
		_output_port = NULL;
	}
	Raul::JackDriver::detach();
}


void
JackDriver::process_input(const TimeSlice& time)
{
	// We only actually read Jack input at the beginning of a cycle
	assert(time.offset_ticks() == 0);

	using namespace std;

	//if (_learn_node) {
		const jack_nframes_t nframes     = time.length_ticks();
		void*                jack_buffer = jack_port_get_buffer(_input_port, nframes);
		const jack_nframes_t event_count = jack_midi_get_event_count(jack_buffer, nframes);

		for (jack_nframes_t i=0; i < event_count; ++i) {
			jack_midi_event_t ev;
			jack_midi_event_get(&ev, jack_buffer, i, nframes);

			if (ev.buffer[0] == 0x90) {

				const SharedPtr<LearnRequest> learn = _machine->pending_learn();
				if (learn) {
					learn->enter_action()->set_event(ev.size, ev.buffer);
					cerr << "LEARN START\n";
					learn->start(_quantization.get(),
						time.ticks_to_beats(jack_last_frame_time(_client) + ev.time));
					//LearnRecord learn = _machine->pop_learn();
				}

			} else if (ev.buffer[0] == 0x80) {
				
				const SharedPtr<LearnRequest> learn = _machine->pending_learn();
				
				if (learn) {
					if (learn->started()) {
						learn->exit_action()->set_event(ev.size, ev.buffer);
						learn->finish(
							time.ticks_to_beats(jack_last_frame_time(_client) + ev.time));
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
JackDriver::write_event(Raul::BeatTime time,
                        size_t         size,
                        const byte*    event)
{
	const TickCount nframes = _cycle_time.length_ticks();
	const TickCount offset  = _cycle_time.beats_to_ticks(time)
		+ _cycle_time.offset_ticks() - _cycle_time.start_ticks();

	assert(_output_port);
	
	if ( ! (offset < _cycle_time.offset_ticks() + nframes)) {
		std::cerr << "ERROR: Event offset " << offset << " outside cycle "
			<< "\n\tbpm: " << _cycle_time.bpm()
			<< "\n\tev time: " << _cycle_time.beats_to_ticks(time)
			<< "\n\tcycle_start: " << _cycle_time.start_ticks()
			<< "\n\tcycle_end: " << _cycle_time.start_ticks() + _cycle_time.length_ticks()
			<< "\n\tcycle_length: " << _cycle_time.length_ticks() << std::endl;
	} else {
		jack_midi_event_write(
				jack_port_get_buffer(_output_port, nframes), offset,
				event, size, nframes);
	}
}


void
JackDriver::on_process(jack_nframes_t nframes)
{
	using namespace std;
	//std::cerr << "> ======================================================\n";

	_cycle_time.set_bpm(_bpm.get());

	// Start time set at end of previous cycle
	_cycle_time.set_offset(0);
	_cycle_time.set_length(nframes);

	assert(_output_port);
	jack_midi_clear_buffer(jack_port_get_buffer(_output_port, nframes), nframes);

	process_input(_cycle_time);

	if (_machine->is_empty()) {
		//cerr << "EMPTY\n";
		return;
	}

	while (true) {
	
		const BeatCount run_dur_beats = _machine->run(_cycle_time);
		const TickCount run_dur_ticks = _cycle_time.beats_to_ticks(run_dur_beats);

		// Machine didn't run at all (empty, or no initial states)
		if (run_dur_ticks == 0) {
			_machine->reset(); // Try again next cycle
			_cycle_time.set_start(0);
			return;

		// Machine ran for portion of cycle (finished)
		} else if (run_dur_ticks < _cycle_time.length_ticks()) {
			const TickCount finish_offset = _cycle_time.offset_ticks() + run_dur_ticks;
			assert(finish_offset < nframes);
			
			_machine->reset();

			_cycle_time.set_start(0);
			_cycle_time.set_length(nframes - finish_offset);
			_cycle_time.set_offset(finish_offset);
		
		// Machine ran for entire cycle
		} else {
			if (_machine->is_finished()) {
				_machine->reset();
				_cycle_time.set_start(0);
			} else {
				_cycle_time.set_start(
						_cycle_time.start_ticks() + _cycle_time.length_ticks());
			}

			break;
		}
	}
	
	//std::cerr << "< ======================================================\n";
}


} // namespace Machina
