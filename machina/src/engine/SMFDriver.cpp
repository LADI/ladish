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
#include <raul/midi_events.h>
#include <raul/SMFWriter.h>
#include <raul/SMFReader.h>
#include "machina/Machine.hpp"
#include "machina/Edge.hpp"
#include "machina/SMFDriver.hpp"

using namespace std;

namespace Machina {


/** Learn a single track from the MIDI file at @a uri
 *
 * @track selects which track of the MIDI file to import, starting from 1.
 *
 * Currently only file:// URIs are supported.
 * @return the resulting machine.
 */
SharedPtr<Machine>
SMFDriver::learn(const Glib::ustring& uri, unsigned track)
{
	const string filename = Glib::filename_from_uri(uri);

	SharedPtr<Machine> m(new Machine());
	
	Raul::SMFReader reader;
	reader.open(filename);

	if (track > reader.num_tracks())
		return SharedPtr<Machine>();
	else
		learn_track(m, reader, track);

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
SMFDriver::learn(const Glib::ustring& uri)
{
	const string filename = Glib::filename_from_uri(uri);

	SharedPtr<Machine> m(new Machine());
	
	Raul::SMFReader reader;
	reader.open(filename);

	for (unsigned t=1; t <= reader.num_tracks(); ++t) {
		learn_track(m, reader, t);
	}

	if (m->nodes().size() > 1)
		return m;
	else
		return SharedPtr<Machine>();
}


void
SMFDriver::learn_track(SharedPtr<Machine> m,
	                   Raul::SMFReader&   reader,
	                   unsigned           track)
{
	const bool found_track = reader.seek_to_track(track);
	assert(found_track);

	list<SharedPtr<Node> > active_nodes;
	
	SharedPtr<Node> initial_node(new Node());
	m->add_node(initial_node);

	SharedPtr<Node> connect_node = initial_node;

	Raul::BeatTime t = 0;
	unsigned char  buf[4];
	uint32_t       ev_size;
	uint32_t       ev_time;
	while (reader.read_event(4, buf, &ev_size, &ev_time) >= 0) {
		t += ev_time / (double)reader.ppqn();
		//cerr << "t = " << t << endl;
		if (ev_size > 0) {
			if ((buf[0] & 0xF0) == MIDI_CMD_NOTE_ON) {
				//cerr << "NOTE ON: " << (int)buf[1] << endl;
				SharedPtr<Node> node(new Node());
				node->add_enter_action(SharedPtr<Action>(new MidiAction(ev_size, buf)));
				connect_node->add_outgoing_edge(SharedPtr<Edge>(new Edge(connect_node, node)));
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
							&& ev[1] == buf[1]) {
						//cerr << "FOUND MATCHING NOTE ON!\n";
						(*i)->add_exit_action(SharedPtr<Action>(new MidiAction(ev_size, buf)));
						(*i)->set_duration(t - (*i)->enter_time());
						(*i)->exit(SharedPtr<Raul::MIDISink>(), t);
						m->add_node((*i));
						if (active_nodes.size() == 1)
							connect_node = (*i);
						active_nodes.erase(i);
						break;
					}
				}
			}
		}
	}
	
	initial_node->set_initial(true);
}


void
SMFDriver::run(SharedPtr<Machine> machine, Raul::BeatTime max_time)
{
	Raul::TimeSlice time(1.0/(double)_ppqn, 120);
	time.set_length(time.beats_to_ticks(max_time));
	machine->set_sink(shared_from_this());
	machine->run(time);
}


} // namespace Machina
