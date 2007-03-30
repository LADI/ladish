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
	
	Raul::SMFReader reader;
	if (!reader.open(filename)) {
		cerr << "Unable to open MIDI file " << filename << endl;
		return SharedPtr<Machine>();
	}

	for (unsigned t=1; t <= reader.num_tracks(); ++t) {
		learn_track(m, reader, t, q, max_duration);
	}

	m->reset();

	if (m->nodes().size() > 1)
		return m;
	else
		return SharedPtr<Machine>();
}


bool
SMFDriver::is_delay_node(SharedPtr<Node> node) const
{
	if (node->enter_action() || node->exit_action())
		return false;
	else
		return true;
}


/** Connect two nodes, inserting a delay node between them if necessary.
 *
 * If a delay node is added to the machine, it is returned.
 */
SharedPtr<Node>
SMFDriver::connect_nodes(SharedPtr<Machine> m,
                         SharedPtr<Node>    tail, Raul::BeatTime tail_end_time,
                         SharedPtr<Node>    head, Raul::BeatTime head_start_time)
{
	assert(tail != head);
	assert(head_start_time >= tail_end_time);

	SharedPtr<Node> delay_node;

	//cerr << "Connect nodes durations: " << tail->duration() << " .. " << head->duration() << endl;
	//cerr << "Connect nodes times: " << tail_end_time << " .. " << head_start_time << endl;

	if (head_start_time == tail_end_time) {
		// Connect directly
		tail->add_outgoing_edge(SharedPtr<Edge>(new Edge(tail, head)));
	} else if (is_delay_node(tail)) {
		// Tail is a delay node, just accumulate the time difference into it
		tail->set_duration(tail->duration() + head_start_time - tail_end_time);
		tail->add_outgoing_edge(SharedPtr<Edge>(new Edge(tail, head)));
	} else {
		// Need to actually create a delay node
		//cerr << "Adding delay node for " << tail_end_time << " .. " << head_start_time << endl;
		delay_node = SharedPtr<Node>(new Node());
		delay_node->set_duration(head_start_time - tail_end_time);
		tail->add_outgoing_edge(SharedPtr<Edge>(new Edge(tail, delay_node)));
		delay_node->add_outgoing_edge(SharedPtr<Edge>(new Edge(delay_node, head)));
		m->add_node(delay_node);
	}

	return delay_node;
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

	typedef list<SharedPtr<Node> > ActiveList;
	ActiveList active_nodes;
	
	typedef list<pair<Raul::BeatTime, SharedPtr<Node> > > PolyList;
	PolyList poly_nodes;
	
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
				
				node->set_enter_action(SharedPtr<Action>(new MidiAction(ev_size, buf)));

				SharedPtr<Node> delay_node = connect_nodes(m, connect_node, connect_node_end_time, node, t);
				if (delay_node) {
					connect_node = delay_node;
					connect_node_end_time = t;
				}

				node->enter(SharedPtr<Raul::MIDISink>(), t);
				active_nodes.push_back(node);

			} else if ((buf[0] & 0xF0) == MIDI_CMD_NOTE_OFF) {
				//cerr << "NOTE OFF: " << (int)buf[1] << endl;
				for (ActiveList::iterator i = active_nodes.begin(); i != active_nodes.end(); ++i) {
					SharedPtr<MidiAction> action = PtrCast<MidiAction>((*i)->enter_action());
					if (!action)
						continue;

					const size_t         ev_size = action->event_size();
					const unsigned char* ev      = action->event();
					if (ev_size == 3 && (ev[0] & 0xF0) == MIDI_CMD_NOTE_ON
							&& (ev[0] & 0x0F) == (buf[0] & 0x0F) // same channel
							&& ev[1] == buf[1]) // same note
					{
						//cerr << "FOUND MATCHING NOTE OFF!\n";

						SharedPtr<Node> resolved = *i;

						resolved->set_exit_action(SharedPtr<Action>(new MidiAction(ev_size, buf)));
						resolved->set_duration(t - resolved->enter_time());

						++added_nodes;
							
						connect_node_end_time = t;
						
						if (active_nodes.size() == 1) {
							if (poly_nodes.size() > 0) {
								
								connect_node = SharedPtr<Node>(new Node());
								m->add_node(connect_node);
									
								connect_nodes(m, resolved, t, connect_node, t);
								
								for (PolyList::iterator j = poly_nodes.begin();
										j != poly_nodes.end(); ++j) {
									m->add_node(j->second);
									connect_nodes(m, j->second, j->first + j->second->duration(),
											connect_node, t);
								}
								poly_nodes.clear();
							
								m->add_node(resolved);
								
							} else {
								// Trim useless delay node, if possible
								// (these happen after polyphonic sections)
								if (is_delay_node(connect_node) && connect_node->duration() == 0
										&& connect_node->outgoing_edges().size() == 1
										&& (*connect_node->outgoing_edges().begin())->head() == resolved) {
									connect_node->outgoing_edges().clear();
									assert(connect_node->outgoing_edges().empty());
									connect_node->set_enter_action(resolved->enter_action());
									connect_node->set_exit_action(resolved->exit_action());
									resolved->remove_enter_action();
									resolved->remove_exit_action();
									connect_node->set_duration(resolved->duration());
									resolved = connect_node;
									if (m->nodes().find(connect_node) == m->nodes().end())
										m->add_node(connect_node);
								} else {
									connect_node = resolved;
									m->add_node(resolved);
								}
							} 

						} else {
							poly_nodes.push_back(make_pair(resolved->enter_time(), resolved));
						}
						
						if (resolved->is_active())
							resolved->exit(SharedPtr<Raul::MIDISink>(), t);
						
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
				(*i)->set_exit_action(SharedPtr<Action>(new MidiAction(3, note_off)));
				(*i)->set_duration(t - (*i)->enter_time());
				(*i)->exit(SharedPtr<Raul::MIDISink>(), t);
				m->add_node((*i));
				++added_nodes;
			}
		}
		active_nodes.clear();
	}
	
	if (m->nodes().find(initial_node) == m->nodes().end())
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
