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

#ifndef MACHINA_MIDIDRIVER_HPP
#define MACHINA_MIDIDRIVER_HPP

#include <raul/TimeSlice.h>
#include "machina/types.hpp"

namespace Machina {

class Node;


class MidiDriver : public Raul::MIDISink {
public:
	virtual ~MidiDriver() {}

	/** Emit a MIDI event at the given time */
	/*virtual void write_event(Raul::BeatTime       time,
	                         size_t               size,
	                         const unsigned char* event) = 0;
	*/

	/** Beginning of current cycle in absolute time.
	 */
	//virtual Raul::TickTime cycle_start() = 0;

	/** Length of current cycle in ticks.
	 *
	 * A "tick" is the smallest recognizable unit of (discrete) time.
	 * Ticks could be single audio frames, MIDI clock at a certain ppqn, etc.
	 */
	//virtual Raul::TickCount cycle_length() = 0;

};


} // namespace Machina

#endif // MACHINA_MIDIDRIVER_HPP

