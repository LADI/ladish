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
#include "config.h"
#include "jack_compat.h"

using namespace Raul;
using namespace std;

namespace Machina {


JackDriver::JackDriver(SharedPtr<Machine> machine)
	: Driver(machine)
	, _machine_changed(0)
	, _input_port(NULL)
	, _output_port(NULL)
	, _cycle_time(1/48000.0, 120.0)
	, _bpm(120.0)
	, _quantization(machine->time().unit())
	, _record_time(machine->time().unit())
	, _recording(0)
{
}


JackDriver::~JackDriver()
{
	detach();
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

		if (!_machine)
			_machine = SharedPtr<Machine>(new Machine(
					TimeUnit::frames(jack_get_sample_rate(jack_client()))));

		_machine->activate();
	}
}


void
JackDriver::detach()
{
	_machine->deactivate();

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
JackDriver::set_machine(SharedPtr<Machine> machine)
{
	if (machine == _machine)
		return;

	cout << "DRIVER MACHINE: " << machine.get() << endl;
	SharedPtr<Machine> last_machine = _last_machine; // Keep a reference 
	_machine_changed.reset(0);
	assert(!last_machine.unique());
	_machine = machine;
	if (is_activated())
		_machine_changed.wait();
	assert(_machine == machine);
	last_machine.reset();
}


void
JackDriver::process_input(SharedPtr<Machine> machine, const TimeSlice& time)
{
	// We only actually read Jack input at the beginning of a cycle
	assert(time.offset_ticks().is_zero());
	assert(_input_port);

	if (_recording.get()) {

		const jack_nframes_t nframes     = time.length_ticks().ticks();
		void*                jack_buffer = jack_port_get_buffer(_input_port, nframes);
		const jack_nframes_t event_count = jack_midi_get_event_count(jack_buffer);

		for (jack_nframes_t i=0; i < event_count; ++i) {
			jack_midi_event_t ev;
			jack_midi_event_get(&ev, jack_buffer, i);
		
			_recorder->write(_record_time + TimeStamp(_record_time.unit(), ev.time, 0),
					ev.size, ev.buffer);
		}

		if (event_count > 0)
			_recorder->whip();

	} else {

		const jack_nframes_t nframes     = time.length_ticks().ticks();
		void*                jack_buffer = jack_port_get_buffer(_input_port, nframes);
		const jack_nframes_t event_count = jack_midi_get_event_count(jack_buffer);

		for (jack_nframes_t i=0; i < event_count; ++i) {
			jack_midi_event_t ev;
			jack_midi_event_get(&ev, jack_buffer, i);

			if (ev.buffer[0] == 0x90) {

				const SharedPtr<LearnRequest> learn = machine->pending_learn();
				if (learn) {
					learn->enter_action()->set_event(ev.size, ev.buffer);
					learn->start(_quantization.get(),
							TimeStamp(TimeUnit::frames(sample_rate()),
								jack_last_frame_time(_client) + ev.time, 0));
				}

			} else if (ev.buffer[0] == 0x80) {
				
				const SharedPtr<LearnRequest> learn = machine->pending_learn();
				
				if (learn) {
					if (learn->started()) {
						learn->exit_action()->set_event(ev.size, ev.buffer);
						learn->finish(
							TimeStamp(TimeUnit::frames(sample_rate()),
								jack_last_frame_time(_client) + ev.time, 0));
						machine->clear_pending_learn();
					}
				}
			}
		}

	}
}


void
JackDriver::write_event(Raul::TimeStamp time,
                        size_t          size,
                        const byte*     event) throw (std::logic_error)
{
	if (!_output_port)
		return;

	if (_cycle_time.beats_to_ticks(time) + _cycle_time.offset_ticks() < _cycle_time.start_ticks()) {
		std::cerr << "ERROR: Missed event by " 
			<< _cycle_time.start_ticks()
				- (_cycle_time.beats_to_ticks(time) + _cycle_time.offset_ticks())
			<< " ticks"
			<< "\n\tbpm: " << _cycle_time.bpm()
			<< "\n\tev time: " << _cycle_time.beats_to_ticks(time)
			<< "\n\tcycle_start: " << _cycle_time.start_ticks()
			<< "\n\tcycle_end: " << _cycle_time.start_ticks() + _cycle_time.length_ticks()
			<< "\n\tcycle_length: " << _cycle_time.length_ticks() << std::endl << std::endl;
		return;
	}

	const TickCount nframes = _cycle_time.length_ticks();
	const TickCount offset  = _cycle_time.beats_to_ticks(time)
		+ _cycle_time.offset_ticks() - _cycle_time.start_ticks();

	if ( ! (offset < _cycle_time.offset_ticks() + nframes)) {
		std::cerr << "ERROR: Event offset " << offset << " outside cycle "
			<< "\n\tbpm: " << _cycle_time.bpm()
			<< "\n\tev time: " << _cycle_time.beats_to_ticks(time)
			<< "\n\tcycle_start: " << _cycle_time.start_ticks()
			<< "\n\tcycle_end: " << _cycle_time.start_ticks() + _cycle_time.length_ticks()
			<< "\n\tcycle_length: " << _cycle_time.length_ticks() << std::endl;
	} else {
#ifdef JACK_MIDI_NEEDS_NFRAMES
		jack_midi_event_write(
				jack_port_get_buffer(_output_port, nframes), offset,
				event, size, nframes);
#else
		jack_midi_event_write(
				jack_port_get_buffer(_output_port, nframes), offset,
				event, size);
#endif
	}
}


void
JackDriver::on_process(jack_nframes_t nframes)
{
	_cycle_time.set_bpm(_bpm.get());

	// (N.B. start time set at end of previous cycle)
	_cycle_time.set_offset(0);
	_cycle_time.set_length(nframes);

	assert(_output_port);
#ifdef JACK_MIDI_NEEDS_NFRAMES
	jack_midi_clear_buffer(jack_port_get_buffer(_output_port, nframes), nframes);
#else
	jack_midi_clear_buffer(jack_port_get_buffer(_output_port, nframes));
#endif
	
	/* Take a reference to machine here and use only it during the process
	 * cycle so _machine can be switched with set_machine during a cycle. */
	SharedPtr<Machine> machine = _machine;

	// Machine was switched since last cycle, finalize old machine.
	if (machine != _last_machine) {
		if (_last_machine) {
			assert(!_last_machine.unique()); // Realtime, can't delete
			_last_machine->set_sink(shared_from_this());
			_last_machine->reset(_last_machine->time()); // Exit all active states
			_last_machine.reset(); // Cut our reference
		}
		_cycle_time.set_start(0);
		_machine_changed.post(); // Signal we're done with it
	}
	
	if (!machine) {
		_last_machine = machine;
		return;
	}
	
	machine->set_sink(shared_from_this());
	
	if (_recording.get())
		_record_time += nframes;

	if (_stop.pending())
		machine->reset(_cycle_time.start_beats());

	process_input(machine, _cycle_time);

	if (machine->is_empty() || !machine->is_activated())
		goto end;

	while (true) {
	
		const BeatCount run_dur_beats = machine->run(_cycle_time);
		const TickCount run_dur_ticks = _cycle_time.beats_to_ticks(run_dur_beats);

		// Machine didn't run at all (empty, or no initial states)
		if (run_dur_beats == 0) {
			machine->reset(machine->time()); // Try again next cycle
			_cycle_time.set_start(0);
			goto end;

		// Machine ran for portion of cycle (finished)
		} else if (run_dur_ticks < _cycle_time.length_ticks()) {
			const TickCount finish_offset = _cycle_time.offset_ticks() + run_dur_ticks;
			assert(finish_offset < nframes);
			
			machine->reset(machine->time());

			_cycle_time.set_start(0);
			_cycle_time.set_length(nframes - finish_offset);
			_cycle_time.set_offset(finish_offset);
		
		// Machine ran for entire cycle
		} else {
			if (machine->is_finished()) {
				machine->reset(machine->time());
				_cycle_time.set_start(0);
			} else {
				_cycle_time.set_start(
						_cycle_time.start_ticks() + _cycle_time.length_ticks());
			}

			break;
		}
	}

end:

	/* Remember the last machine run, in case a switch happens and
	 * we need to finalize it next cycle. */
	_last_machine = machine;
	
	if (_stop.pending()) {
		_cycle_time.set_start(0);
		_stop.finish();
	}
}


void
JackDriver::stop()
{
	if (recording())
		finish_record();

	_stop(); // waits
	_machine->deactivate();
}


void
JackDriver::start_record()
{
	// FIXME: Choose an appropriate maximum ringbuffer size
	_recorder = SharedPtr<Recorder>(new Recorder(
				1024, (1.0/(double)sample_rate()) * (_bpm.get() / 60.0), _quantization.get()));
	_recorder->start();
	_record_time = 0;
	_recording = 1;
}


void
JackDriver::finish_record()
{
	_recording = 0;
	SharedPtr<Machine> machine = _recorder->finish();
	_recorder.reset();
	machine->activate();
	_machine->nodes().append(machine->nodes());
}


} // namespace Machina
