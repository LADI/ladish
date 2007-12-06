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

#include <iostream>
#include <machina/Problem.hpp>
#include <machina/Machine.hpp>
#include <raul/SMFReader.hpp>
#include <raul/midi_events.h>

using namespace std;

namespace Machina {

	
Problem::Problem(const std::string& target_midi)
	: _target(*this)
{
	Raul::SMFReader smf;
	const bool opened = smf.open(target_midi);
	assert(opened);
	
	smf.seek_to_track(2); // FIXME: kluge
	
	uint8_t  buf[4];
	uint32_t ev_size;
	uint32_t delta_time;
	while (smf.read_event(4, buf, &ev_size, &delta_time) >= 0) {
		if ((buf[0] & 0xF0) == MIDI_CMD_NOTE_ON) {
			const uint8_t note = buf[1];
			++_target._note_frequency[note];
			++_target._n_notes;
		}
	}

	_target.compute();
}


float
Problem::fitness(Machine& machine)
{
	SharedPtr<Evaluator> eval(new Evaluator(*this));

	machine.reset(0.0f);
	machine.set_sink(eval);

	// FIXME: timing stuff here isn't right at all...
	
	unsigned ppqn = 19200;
	Raul::TimeSlice time(ppqn, 120);
	time.set_start(0);
	time.set_length(ppqn);

	while (time.start_ticks() < _target._n_notes * ppqn) {
		machine.run(time);
		time.set_start(time.start_ticks() + ppqn);
	}

	eval->compute();

	float f = 0;

	// Punish for frequency differences.
	// Optimal fitness = 0 (max)
	for (uint8_t i=0; i < 128; ++i)
		f -= fabs(_target._note_frequency[i] - eval->_note_frequency[i]);

	return f;
}
	

void
Problem::Evaluator::write_event(Raul::BeatTime time,
                                size_t         ev_size,
                                const uint8_t* ev) throw (std::logic_error)
{
	if ((ev[0] & 0xF0) == MIDI_CMD_NOTE_ON) {
		const uint8_t note = ev[1];
		++_note_frequency[note];
		++_n_notes;
	}
}


void
Problem::Evaluator::compute()
{
	for (uint8_t i=0; i < 128; ++i) {
		if (_note_frequency[i] > 0) {
			_note_frequency[i] /= (float)_n_notes;
			//cout << (int)i << ":\t" << _note_frequency[i] << endl;
		}
	}
}


} // namespace Machina

