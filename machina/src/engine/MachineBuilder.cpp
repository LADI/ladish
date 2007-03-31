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

#include <algorithm>
#include <raul/midi_events.h>
#include "machina/MachineBuilder.hpp"
#include "machina/Machine.hpp"
#include "machina/Node.hpp"
#include "machina/Edge.hpp"

using namespace std;

namespace Machina {


MachineBuilder::MachineBuilder(SharedPtr<Machine> machine)
	: _time(0)
	, _machine(machine)
	, _initial_node(new Node())
	, _connect_node(_initial_node)
	, _connect_node_end_time(0)
{
	_initial_node->set_initial(true);
}


void
MachineBuilder::reset()
{
	_initial_node = SharedPtr<Node>(new Node());
	_initial_node->set_initial(true);
	_connect_node = _initial_node;
	_connect_node_end_time = 0;
	_time = 0;
}


bool
MachineBuilder::is_delay_node(SharedPtr<Node> node) const
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
MachineBuilder::connect_nodes(SharedPtr<Machine> m,
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
MachineBuilder::event(Raul::BeatTime time_offset,
                      size_t         ev_size,
                      unsigned char* buf)
{
	Raul::BeatTime t = _time + time_offset;

	cerr << "t = " << t << endl;
	
	if (ev_size > 0) {
		if ((buf[0] & 0xF0) == MIDI_CMD_NOTE_ON) {
			//cerr << "NOTE ON: " << (int)buf[1] << ", channel = " << (int)(buf[0] & 0x0F) << endl;
			SharedPtr<Node> node(new Node());

			node->set_enter_action(SharedPtr<Action>(new MidiAction(ev_size, buf)));

			SharedPtr<Node> delay_node = connect_nodes(_machine,
					_connect_node, _connect_node_end_time, node, t);

			if (delay_node) {
				_connect_node = delay_node;
				_connect_node_end_time = t;
			}

			node->enter(SharedPtr<Raul::MIDISink>(), t);
			_active_nodes.push_back(node);

		} else if ((buf[0] & 0xF0) == MIDI_CMD_NOTE_OFF) {
			//cerr << "NOTE OFF: " << (int)buf[1] << endl;
			for (ActiveList::iterator i = _active_nodes.begin(); i != _active_nodes.end(); ++i) {
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

					_connect_node_end_time = t;

					if (_active_nodes.size() == 1) {
						if (_poly_nodes.size() > 0) {

							_connect_node = SharedPtr<Node>(new Node());
							_machine->add_node(_connect_node);

							connect_nodes(_machine, resolved, t, _connect_node, t);

							for (PolyList::iterator j = _poly_nodes.begin();
									j != _poly_nodes.end(); ++j) {
								_machine->add_node(j->second);
								connect_nodes(_machine, j->second, j->first + j->second->duration(),
										_connect_node, t);
							}
							_poly_nodes.clear();

							_machine->add_node(resolved);

						} else {
							// Trim useless delay node, if possible
							// (these happen after polyphonic sections)
							if (is_delay_node(_connect_node) && _connect_node->duration() == 0
									&& _connect_node->outgoing_edges().size() == 1
									&& (*_connect_node->outgoing_edges().begin())->head() == resolved) {
								_connect_node->outgoing_edges().clear();
								assert(_connect_node->outgoing_edges().empty());
								_connect_node->set_enter_action(resolved->enter_action());
								_connect_node->set_exit_action(resolved->exit_action());
								resolved->remove_enter_action();
								resolved->remove_exit_action();
								_connect_node->set_duration(resolved->duration());
								resolved = _connect_node;
								if (_machine->nodes().find(_connect_node) == _machine->nodes().end())
									_machine->add_node(_connect_node);
							} else {
								_connect_node = resolved;
								_machine->add_node(resolved);
							}
						} 

					} else {
						_poly_nodes.push_back(make_pair(resolved->enter_time(), resolved));
					}

					if (resolved->is_active())
						resolved->exit(SharedPtr<Raul::MIDISink>(), t);

					_active_nodes.erase(i);

					break;
				}
			}
		}
	}
}


/** Resolve any stuck notes.
 */
void
MachineBuilder::resolve()
{
	if ( ! _active_nodes.empty()) {
		for (list<SharedPtr<Node> >::iterator i = _active_nodes.begin(); i != _active_nodes.end(); ++i) {
			cerr << "WARNING:  Resolving stuck note from MIDI file." << endl;
			SharedPtr<MidiAction> action = PtrCast<MidiAction>((*i)->enter_action());
			if (!action)
				continue;

			const size_t         ev_size = action->event_size();
			const unsigned char* ev      = action->event();
			if (ev_size == 3 && (ev[0] & 0xF0) == MIDI_CMD_NOTE_ON) {
				unsigned char note_off[3] = { ((MIDI_CMD_NOTE_OFF & 0xF0) | (ev[0] & 0x0F)), ev[1], 0x40 };
				(*i)->set_exit_action(SharedPtr<Action>(new MidiAction(3, note_off)));
				(*i)->set_duration(_time - (*i)->enter_time());
				(*i)->exit(SharedPtr<Raul::MIDISink>(), _time);
				_machine->add_node((*i));
			}
		}
		_active_nodes.clear();
	}
	
	if (_machine->nodes().size() > 0 
			&& _machine->nodes().find(_initial_node) == _machine->nodes().end())
		_machine->add_node(_initial_node);
}


SharedPtr<Machine>
MachineBuilder::finish()
{
	resolve();

	return _machine;
}


} // namespace Machina
