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
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef MACHINA_LEARNREQUEST_HPP
#define MACHINA_LEARNREQUEST_HPP

#include <raul/Maid.h>
#include <raul/SharedPtr.h>
#include "types.hpp"
#include "Node.hpp"
#include "MidiAction.hpp"

namespace Machina {

class Node;
class MidiAction;


/** A request to MIDI learn a certain node.
 */
class LearnRequest : public Raul::Deletable {
public:

	static SharedPtr<LearnRequest>
	create(SharedPtr<Raul::Maid> maid, SharedPtr<Node> node)
	{
		SharedPtr<LearnRequest> ret(new LearnRequest(maid, node));
		maid->manage(ret);
		return ret;
	}

	void start(double q, BeatTime time)
		{ _started = true; _start_time = time; _quantization = q; }
	
	void finish(BeatTime time);
	
	bool started() { return _started; }

	const SharedPtr<Node>&       node()         { return _node; }
	const SharedPtr<MidiAction>& enter_action() { return _enter_action; }
	const SharedPtr<MidiAction>& exit_action()  { return _exit_action; }

private:
	LearnRequest(SharedPtr<Raul::Maid> maid, SharedPtr<Node> node)
		: _started(false)
		, _node(node)
		, _enter_action(MidiAction::create(maid, 4, NULL))
		, _exit_action(MidiAction::create(maid, 4, NULL))
	{
	}

	bool                  _started;
	BeatTime              _start_time;
	double                _quantization;
	SharedPtr<Node>       _node;
	SharedPtr<MidiAction> _enter_action;
	SharedPtr<MidiAction> _exit_action;
};


} // namespace Machina

#endif // MACHINA_LEARNREQUEST_HPP
