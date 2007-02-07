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

#include "JackNodeFactory.hpp"
#include "JackActions.hpp"
#include "Node.hpp"

namespace Machina {


SharedPtr<Node>
JackNodeFactory::create_node(Node::ID, unsigned char note, FrameCount duration)
{
	// FIXME: leaks like a sieve, obviously
	
	Node* n = new Node(duration);
	JackNoteOnAction* a_enter = new JackNoteOnAction(_driver, note);
	JackNoteOffAction* a_exit = new JackNoteOffAction(_driver, note);

	n->add_enter_action(a_enter);
	n->add_exit_action(a_exit);

	return SharedPtr<Node>(n);
}


} // namespace Machina

