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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <iostream>
#include "JackActions.hpp"
#include "JackDriver.hpp"

namespace Machina {


/* NOTE ON */

JackNoteOnAction::JackNoteOnAction(WeakPtr<JackDriver> driver,
                                   unsigned char       note_num)
	: _driver(driver)
	, _note_num(note_num)
{
}


void
JackNoteOnAction::execute(Timestamp time)
{
	SharedPtr<JackDriver> driver = _driver.lock();
	if (!driver)
		return;
	
	const FrameCount nframes = driver->current_cycle_nframes();
	const FrameCount offset  = driver->stamp_to_offset(time);

	//std::cerr << offset << " \tNOTE ON:\t" << (int)_note_num << "\t@ " << time << std::endl;
	
	jack_midi_data_t ev[] = { 0x80, _note_num, 0x40 };

	jack_midi_event_write(
		jack_port_get_buffer(driver->output_port(), nframes),
		offset, ev, 3, nframes);
}



/* NOTE OFF */

JackNoteOffAction::JackNoteOffAction(WeakPtr<JackDriver> driver,
                                     unsigned char       note_num)
	: _driver(driver)
	, _note_num(note_num)
{
}


void
JackNoteOffAction::execute(Timestamp time)
{
	SharedPtr<JackDriver> driver = _driver.lock();
	if (!driver)
		return;
	
	const FrameCount nframes = driver->current_cycle_nframes();
	const FrameCount offset  = driver->stamp_to_offset(time);

	//std::cerr << offset << " \tNOTE OFF:\t" << (int)_note_num << "\t@ " << time << std::endl;
	
	jack_midi_data_t ev[] = { 0x90, _note_num, 0x40 };

	jack_midi_event_write(
		jack_port_get_buffer(driver->output_port(), nframes),
		offset, ev, 3, nframes);
}


} // namespace Machina

	
