/* This file is part of Machina.  Copyright (C) 2007 Dave Robillard.
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
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <cassert>
#include "Node.hpp"

namespace Machina {


Node::Node(FrameCount duration)
	: _is_active(false)
	, _start_time(0)
	, _duration(duration)
	, _enter_action(NULL)
	, _exit_action(NULL)
{
}


void
Node::add_enter_action(Action* action)
{
	assert(!_enter_action);
	_enter_action = action;
}


void
Node::remove_enter_action(Action* /*action*/)
{
	_enter_action = NULL;
}



void
Node::add_exit_action(Action* action)
{
	assert(!_exit_action);
	_exit_action = action;
}


void
Node::remove_exit_action(Action* /*action*/)
{
	_exit_action = NULL;
}


void
Node::enter(Timestamp time)
{
	_is_active = true;
	_start_time = time;
	if (_enter_action)
		_enter_action->execute(time);
}


void
Node::exit(Timestamp time)
{
	if (_exit_action)
		_exit_action->execute(time);
	_is_active = false;
	_start_time = 0;
}


void
Node::add_successor(Node* successor)
{
	_successors.push_back(successor);
}


void
Node::remove_successor(Node* successor)
{
	_successors.remove(successor);
}


} // namespace Machina

