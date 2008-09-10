/* This file is part of Raul.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Raul is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Raul is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <raul/TimeSlice.hpp>

namespace Raul {


TimeSlice::TimeSlice(uint32_t tick_rate, double bpm)
	: _tick_rate(tick_rate)
	, _beat_rate(60.0/bpm)
	, _start_ticks(0)
	, _offset(0)
	, _length(0)
	, _start_beats(0)
{}
					 

/** Update beat time to match real (tick) time.
 */
void
TimeSlice::update_beat_time()
{
	_start_beats = (start_ticks() * _tick_rate) / _beat_rate;
}


TickTime
TimeSlice::beats_to_ticks(BeatTime beats)
{
	return static_cast<TickTime>(beats_to_seconds(beats) / _tick_rate); 
}

Seconds
TimeSlice::beats_to_seconds(BeatTime beats)
{
	return (beats * _beat_rate);
}

} // namespace Raul
