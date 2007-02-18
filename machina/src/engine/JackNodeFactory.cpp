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

#include "machina/JackNodeFactory.hpp"
#include "machina/MidiAction.hpp"
#include "machina/Node.hpp"
#include "machina/JackDriver.hpp"

namespace Machina {


SharedPtr<Node>
JackNodeFactory::create_node(Node::ID, byte /*note*/, FrameCount duration)
{
	// FIXME: obviously leaks like a sieve

	//size_t event_size = 3;
	//static const byte note_on[3] = { 0x80, note, 0x40 };
	
	Node* n = new Node(duration);
	/*SharedPtr<MidiAction> a_enter = MidiAction::create(event_size, note_on);
	
	
	static const byte note_off[3] = { 0x90, note, 0x40 };
	SharedPtr<MidiAction> a_exit = MidiAction::create(event_size, note_off);

	n->add_enter_action(a_enter);
	n->add_exit_action(a_exit);
*/
	return SharedPtr<Node>(n);
}


} // namespace Machina

