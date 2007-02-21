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

#include <iostream>
#include <raul/SharedPtr.h>
#include "machina/MidiAction.hpp"
#include "machina/MidiDriver.hpp"

namespace Machina {

WeakPtr<MidiDriver> MidiAction::_driver;


/** Create a MIDI action.
 *
 * Creating a NULL MIDIAction is okay, pass event=NULL and
 * the action will simply do nothing until a set_event (for MIDI learning).
 *
 * Memory management of @event is the caller's responsibility
 * (ownership is not taken).
 */
MidiAction::MidiAction(size_t      size,
                       const byte* event)
	: _size(0)
	, _max_size(size)
{
	_event = new byte[_max_size];
	set_event(size, event);
}


MidiAction::~MidiAction()
{
	if (_event.get())
		delete _event.get();
}


/** Set the MIDI driver to be used for executing MIDI actions.
 *
 * MIDI actions will silently do nothing unless this call is passed an
 * existing MidiDriver.
 */
void
MidiAction::set_driver(SharedPtr<MidiDriver> driver)
{
	_driver = driver;
}


/** Set the MIDI event to be emitted when the action executes.
 *
 * Returns pointer to old event (caller's responsibility to free if non-NULL).
 * Safe to call concurrently with execute.
 *
 * Returns true on success.
 */
bool
MidiAction::set_event(size_t size, const byte* new_event)
{
	byte* const event = _event.get();
	if (size <= _max_size) {
		_event = NULL;
		if (size > 0 && new_event)
			memcpy(event, new_event, size);
		_size = size;
		_event = event;
		return true;
	} else {
		return false;
	}
}


/** Execute the action.
 *
 * Safe to call concurrently with set_event.
 */
void
MidiAction::execute(Raul::BeatTime time)
{
	const byte* const event = _event.get();

	if (event) {
		SharedPtr<MidiDriver> driver = _driver.lock();
		if (driver)
			driver->write_event(time, _size, event);
	} else {
		std::cerr << "NULL MIDI ACTION";
	}
}


} // namespace Machina


