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
#include <raul/midi_events.h>
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
	
	Raul::SMFReader reader;
	
	if (!reader.open(filename)) {
		cerr << "Unable to open MIDI file " << filename << endl;
		return SharedPtr<Machine>();
	}

	if (track > reader.num_tracks())
		return SharedPtr<Machine>();
	else
		learn_track(m, reader, track, q, max_duration);

	if (m->nodes().size() > 1)
		return m;
	else
		return SharedPtr<Machine>();
}


/** Learn all tracks from a MIDI file into a single machine.
 *
 * This will result in a disjoint subgraph in the machine, one for each track.
 */
SharedPtr<Machine>
SMFDriver::learn(const string& filename, double q, Raul::BeatTime max_duration)
{
	SharedPtr<Machine> m(new Machine());
	
	Raul::SMFReader reader;
	if (!reader.open(filename)) {
		cerr << "Unable to open MIDI file " << filename << endl;
		return SharedPtr<Machine>();
	}

	for (unsigned t=1; t <= reader.num_tracks(); ++t) {
		learn_track(m, reader, t, q, max_duration);
	}

	if (m->nodes().size() > 1)
		return m;
	else
		return SharedPtr<Machine>();
}


void
SMFDriver::learn_track(SharedPtr<Machine> m,
                       Raul::SMFReader&   reader,
                       unsigned           track,
                       double             q,
                       Raul::BeatTime     max_duration)
{
	const bool found_track = reader.seek_to_track(track);
	if (!found_track)
		return;

	list<SharedPtr<Node> > active_nodes;
	
	SharedPtr<Node> initial_node(new Node());
	initial_node->set_initial(true);
	//m->add_node(initial_node);

	SharedPtr<Node> connect_node = initial_node;
	Raul::BeatTime  connect_node_end_time = 0;

	unsigned added_nodes = 0;

	Raul::BeatTime unquantized_t = 0;
	Raul::BeatTime t = 0;
	unsigned char  buf[4];
	uint32_t       ev_size;
	uint32_t       ev_time;
	while (reader.read_event(4, buf, &ev_size, &ev_time) >= 0) {
		unquantized_t += ev_time / (double)reader.ppqn();
		t = Raul::Quantizer::quantize(q, unquantized_t);

		if (max_duration != 0 && t > max_duration)
			break;

		//cerr << "t = " << t << endl;
		if (ev_size > 0) {
			if ((buf[0] & 0xF0) == MIDI_CMD_NOTE_ON) {
				//cerr << "NOTE ON: " << (int)buf[1] << ", channel = " << (int)(buf[0] & 0x0F) << endl;
				SharedPtr<Node> node(new Node());
				node->add_enter_action(SharedPtr<Action>(new MidiAction(ev_size, buf)));
				assert(connect_node_end_time <= t);
				
				if (t == connect_node_end_time) {
					connect_node->add_outgoing_edge(SharedPtr<Edge>(new Edge(connect_node, node)));
				} else {
					SharedPtr<Node> delay_node(new Node());
					delay_node->set_duration(t - connect_node_end_time);
					connect_node->add_outgoing_edge(SharedPtr<Edge>(new Edge(connect_node, delay_node)));
					delay_node->add_outgoing_edge(SharedPtr<Edge>(new Edge(delay_node, node)));
					m->add_node(delay_node);
					++added_nodes;
					connect_node = delay_node;
					connect_node_end_time = t;
				}

				node->enter(SharedPtr<Raul::MIDISink>(), t);
				active_nodes.push_back(node);
			} else if ((buf[0] & 0xF0) == MIDI_CMD_NOTE_OFF) {
				//cerr << "NOTE OFF: " << (int)buf[1] << endl;
				for (list<SharedPtr<Node> >::iterator i = active_nodes.begin();
						i != active_nodes.end(); ++i) {
					SharedPtr<MidiAction> action = PtrCast<MidiAction>((*i)->enter_action());
					if (!action)
						continue;

					const size_t         ev_size = action->event_size();
					const unsigned char* ev      = action->event();
					if (ev_size == 3 && (ev[0] & 0xF0) == MIDI_CMD_NOTE_ON
							&& (ev[0] & 0x0F) == (buf[0] & 0x0F) // same channel
							&& ev[1] == buf[1]) // same note
					{
						//cerr << "FOUND MATCHING NOTE ON!\n";
						(*i)->add_exit_action(SharedPtr<Action>(new MidiAction(ev_size, buf)));
						(*i)->set_duration(t - (*i)->enter_time());
						(*i)->exit(SharedPtr<Raul::MIDISink>(), t);
						m->add_node((*i));
						++added_nodes;
						if (active_nodes.size() == 1) {
							connect_node = (*i);
							connect_node_end_time = t;
						}
						active_nodes.erase(i);
						break;
					}
				}
			}
		}
	}
	
	// Resolve any stuck notes when the rest of the machine is finished
	if ( ! active_nodes.empty()) {
		for (list<SharedPtr<Node> >::iterator i = active_nodes.begin(); i != active_nodes.end(); ++i) {
			cerr << "WARNING:  Resolving stuck note from MIDI file." << endl;
			SharedPtr<MidiAction> action = PtrCast<MidiAction>((*i)->enter_action());
			if (!action)
				continue;

			const size_t         ev_size = action->event_size();
			const unsigned char* ev      = action->event();
			if (ev_size == 3 && (ev[0] & 0xF0) == MIDI_CMD_NOTE_ON) {
				unsigned char note_off[3] = { ((MIDI_CMD_NOTE_OFF & 0xF0) | (ev[0] & 0x0F)), ev[1], 0x40 };
				(*i)->add_exit_action(SharedPtr<Action>(new MidiAction(3, note_off)));
				(*i)->set_duration(t - (*i)->enter_time());
				(*i)->exit(SharedPtr<Raul::MIDISink>(), t);
				m->add_node((*i));
				++added_nodes;
			}
		}
		active_nodes.clear();
	}
	
	if (added_nodes > 0)
		if (initial_node->outgoing_edges().size() == 1)
			(*initial_node->outgoing_edges().begin())->dst()->set_initial(true);
		else
			m->add_node(initial_node);
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
