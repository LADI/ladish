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

#include <list>
#include <iostream>
#include <glibmm/convert.h>
#include <raul/Quantizer.h>
#include <raul/SharedPtr.h>
#include <raul/SMFWriter.h>
#include <raul/SMFReader.h>
#include "machina/Machine.hpp"
#include "machina/Edge.hpp"
#include "machina/SMFDriver.hpp"

using namespace std;

namespace Machina {


SMFDriver::SMFDriver(SharedPtr<Machine> machine)
	: Driver(machine)
{
	_writer = SharedPtr<Raul::SMFWriter>(new Raul::SMFWriter());
}


/** Learn a single track from the MIDI file at @a uri
 *
 * @track selects which track of the MIDI file to import, starting from 1.
 *
 * Currently only file:// URIs are supported.
 * @return the resulting machine.
 */
SharedPtr<Machine>
SMFDriver::learn(const string& filename, unsigned track, double q, Raul::BeatTime max_duration)
{
	SharedPtr<Machine> m(new Machine());
	SharedPtr<MachineBuilder> builder = SharedPtr<MachineBuilder>(new MachineBuilder(m));
	Raul::SMFReader reader;
	
	if (!reader.open(filename)) {
		cerr << "Unable to open MIDI file " << filename << endl;
		return SharedPtr<Machine>();
	}

	if (track > reader.num_tracks())
		return SharedPtr<Machine>();
	else
		learn_track(builder, reader, track, q, max_duration);

	m->reset();

	if (m->nodes().size() > 1)
		return m;
	else
		return SharedPtr<Machine>();
}


/** Learn all tracks from a MIDI file into a single machine.
 *
 * This will result in one disjoint subgraph in the machine for each track.
 */
SharedPtr<Machine>
SMFDriver::learn(const string& filename, double q, Raul::BeatTime max_duration)
{
	SharedPtr<Machine> m(new Machine());
	SharedPtr<MachineBuilder> builder = SharedPtr<MachineBuilder>(new MachineBuilder(m));
	Raul::SMFReader reader;
	
	if (!reader.open(filename)) {
		cerr << "Unable to open MIDI file " << filename << endl;
		return SharedPtr<Machine>();
	}

	for (unsigned t=1; t <= reader.num_tracks(); ++t) {
		builder->reset();
		learn_track(builder, reader, t, q, max_duration);
	}

	m->reset();

	if (m->nodes().size() > 1)
		return m;
	else
		return SharedPtr<Machine>();
}


void
SMFDriver::learn_track(SharedPtr<MachineBuilder> builder,
                       Raul::SMFReader&          reader,
                       unsigned                  track,
                       double                    q,
                       Raul::BeatTime            max_duration)
{
	const bool found_track = reader.seek_to_track(track);
	if (!found_track)
		return;

	Raul::BeatTime unquantized_t = 0;
	Raul::BeatTime t = 0;
	unsigned char  buf[4];
	uint32_t       ev_size;
	uint32_t       ev_time;
	while (reader.read_event(4, buf, &ev_size, &ev_time) >= 0) {
		unquantized_t += ev_time / (double)reader.ppqn();
		t = Raul::Quantizer::quantize(q, unquantized_t);

		builder->set_time(t);

		if (max_duration != 0 && t > max_duration)
			break;

		if (ev_size > 0)
			builder->event(0, ev_size, buf);
	}

	builder->resolve();
}


void
SMFDriver::run(SharedPtr<Machine> machine, Raul::BeatTime max_time)
{
	Raul::TimeSlice time(1.0/(double)_writer->ppqn(), 120);
	time.set_length(time.beats_to_ticks(max_time));
	machine->set_sink(shared_from_this());
	machine->run(time);
}


} // namespace Machina
