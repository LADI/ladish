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


/** Learn the MIDI file at @a uri
 *
 * Currently only file:// URIs are supported.
 * @return the resulting machine.
 */
SharedPtr<Machine>
SMFDriver::learn(const Glib::ustring& uri, unsigned track)
{
	const string filename = Glib::filename_from_uri(uri);

	std::cerr << "Learn MIDI: " << filename << std::endl;
	
	SharedPtr<Machine> m(new Machine());

	list<SharedPtr<Node> > active_nodes;
	SharedPtr<Node>        connect_node(new Node());
	connect_node->set_initial(true);
	m->add_node(connect_node);

	Raul::SMFReader reader;
	reader.open(filename);

	if ( ! reader.seek_to_track(track) )
		return SharedPtr<Machine>();

	Raul::BeatTime t = 0;
	unsigned char  buf[4];
	uint32_t       ev_size;
	uint32_t       ev_time;
	while (reader.read_event(4, buf, &ev_size, &ev_time) >= 0) {
		t += ev_time / (double)reader.ppqn();
		cerr << "t = " << t << endl;
		if (ev_size > 0) {
			if ((buf[0] & 0xF0) == MIDI_CMD_NOTE_ON) {
				cerr << "NOTE ON: " << (int)buf[1] << endl;
				SharedPtr<Node> node(new Node());
				node->add_enter_action(SharedPtr<Action>(new MidiAction(ev_size, buf)));
				connect_node->add_outgoing_edge(SharedPtr<Edge>(new Edge(connect_node, node)));
				node->enter(SharedPtr<Raul::MIDISink>(), t);
				active_nodes.push_back(node);
			} else if ((buf[0] & 0xF0) == MIDI_CMD_NOTE_OFF) {
				cerr << "NOTE OFF: " << (int)buf[1] << endl;
				for (list<SharedPtr<Node> >::iterator i = active_nodes.begin();
						i != active_nodes.end(); ++i) {
					SharedPtr<MidiAction> action = PtrCast<MidiAction>((*i)->enter_action());
					if (!action)
						continue;

					const size_t         ev_size = action->event_size();
					const unsigned char* ev      = action->event();
					if (ev_size == 3 && (ev[0] & 0xF0) == MIDI_CMD_NOTE_ON
							&& ev[1] == buf[1]) {
						cerr << "FOUND MATCHING NOTE ON!\n";
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
		/*std::cerr << "Event, size = " << ev_size << ", time = " << ev_time << std::endl;
		cerr.flags(ios::hex);
		for (uint32_t i=0; i < ev_size; ++i) {
			cerr << "0x" << (int)buf[i] << " ";
		}
		cerr.flags(ios::dec);
		cerr << endl;*/
	}
	return m;
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
