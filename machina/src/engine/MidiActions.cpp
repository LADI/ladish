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
#include "machina/MidiAction.hpp"
#include "machina/MidiDriver.hpp"

namespace Machina {


/* NOTE ON */

MidiAction(WeakPtr<JackDriver> driver,
           size_t              size,
           unsigned char*      event)
	: _driver(driver)
	, _size(size)
	, _event(event)
{
}


void
MidiAction::execute(Timestamp time)
{
	SharedPtr<MidiDriver> driver = _driver.lock();
	if (!driver)
		return;
	
	const FrameCount nframes = driver->current_cycle_nframes();
	const FrameCount offset  = driver->stamp_to_offset(time);

	//std::cerr << offset << " \tMIDI @ " << time << std::endl;
	
	//jack_midi_data_t ev[] = { 0x80, _note_num, 0x40 }; note on
	//jack_midi_data_t ev[] = { 0x90, _note_num, 0x40 }; note off

	jack_midi_event_write(
		jack_port_get_buffer(driver->output_port(), nframes),
		offset, ev, _size, _event);
}


} // namespace Machina

	
