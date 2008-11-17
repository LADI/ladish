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
#include "raul/Quantizer.hpp"
#include "raul/SharedPtr.hpp"
#include "raul/SMFWriter.hpp"
#include "raul/SMFReader.hpp"
#include "machina/Machine.hpp"
#include "machina/Edge.hpp"
#include "machina/SMFDriver.hpp"

using namespace std;

namespace Machina {


SMFDriver::SMFDriver(SharedPtr<Machine> machine)
	: Driver(machine)
{
	_writer = SharedPtr<Raul::SMFWriter>(new Raul::SMFWriter(machine->time().unit()));
}


/** Learn a single track from the MIDI file at @a uri
 *
 * @track selects which track of the MIDI file to import, starting from 1.
 *
 * Currently only file:// URIs are supported.
 * @return the resulting machine.
 */
SharedPtr<Machine>
SMFDriver::learn(const string& filename, unsigned track, double q, Raul::TimeDuration max_duration)
{
	//assert(q.unit() == max_duration.unit());
	SharedPtr<Machine> m(new Machine(max_duration.unit()));
	SharedPtr<MachineBuilder> builder = SharedPtr<MachineBuilder>(new MachineBuilder(m, q));
	Raul::SMFReader reader;
	
	if (!reader.open(filename)) {
		cerr << "Unable to open MIDI file " << filename << endl;
		return SharedPtr<Machine>();
	}

	if (track > reader.num_tracks())
		return SharedPtr<Machine>();
	else
		learn_track(builder, reader, track, q, max_duration);

	m->reset(m->time());

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
SMFDriver::learn(const string& filename, double q, Raul::TimeStamp max_duration)
{
#if 0
	SharedPtr<Machine> m(new Machine(q.unit()));
	SharedPtr<MachineBuilder> builder = SharedPtr<MachineBuilder>(new MachineBuilder(m, q));
	Raul::SMFReader reader;
	
	if (!reader.open(filename)) {
		cerr << "Unable to open MIDI file " << filename << endl;
		return SharedPtr<Machine>();
	}

	for (unsigned t=1; t <= reader.num_tracks(); ++t) {
		builder->reset();
		learn_track(builder, reader, t, q, max_duration);
	}

	m->reset(m->time());

	if (m->nodes().size() > 1)
		return m;
	else
#endif
		return SharedPtr<Machine>();
}


void
SMFDriver::learn_track(SharedPtr<MachineBuilder> builder,
                       Raul::SMFReader&          reader,
                       unsigned                  track,
                       double                    q,
                       Raul::TimeDuration        max_duration)
{
	const bool found_track = reader.seek_to_track(track);
	if (!found_track)
		return;
#if 0
	uint8_5  buf[4];
	uint32_t ev_size;
	uint32_t ev_delta_time;
	
	uint64_t t = 0;
	uint64_t unquantized_t = 0;

	while (reader.read_event(4, buf, &ev_size, &ev_delta_time) >= 0) {
		unquantized_t += ev_delta_time;
		t = Raul::Quantizer::quantize(q, unquantized_t);

		builder->set_time(t);

		if ((!max_duration.is_zero()) && t > max_duration)
			break;

		if (ev_size > 0)
			builder->event(TimeStamp(t.unit(), 0, 0), ev_size, buf);
	}
#endif

	builder->resolve();
}


void
SMFDriver::run(SharedPtr<Machine> machine, Raul::TimeStamp max_time)
{
	// FIXME: unit kludge (tempo only)
	Raul::TimeSlice time(1.0/(double)_writer->unit().ppt(), 120);
	time.set_length(time.beats_to_ticks(max_time));
	machine->set_sink(shared_from_this());
	machine->run(time);
}


} // namespace Machina
