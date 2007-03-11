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

#ifndef MACHINA_MIDIACTION_HPP
#define MACHINA_MIDIACTION_HPP

#include <raul/Maid.h>
#include <raul/WeakPtr.h>
#include <raul/AtomicPtr.h>
#include <raul/TimeSlice.h>
#include "types.hpp"
#include "Action.hpp"

namespace Raul { class MIDISink; }

namespace Machina {


class MidiAction : public Action {
public:
	~MidiAction();
	
	MidiAction(size_t               size,
	           const unsigned char* event);

	static SharedPtr<MidiAction>
	create(SharedPtr<Raul::Maid> maid,
	       size_t size, const unsigned char* event)
	{
		SharedPtr<MidiAction> ret(new MidiAction(size, event));
		maid->manage(ret);
		return ret;
	}

	size_t event_size() { return _size; }
	byte*  event()      { return _event.get(); }

	bool set_event(size_t size, const byte* event);

	void execute(SharedPtr<Raul::MIDISink> driver, Raul::BeatTime time);
	
	virtual void write_state(Raul::RDFWriter& writer);

private:


	size_t                 _size;
	const size_t           _max_size;
	Raul::AtomicPtr<byte>  _event;
};


} // namespace Machina

#endif // MACHINA_MIDIACTION_HPP
