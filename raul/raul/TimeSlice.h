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

#ifndef RAUL_TIMESLICE_H
#define RAUL_TIMESLICE_H

#include <cassert>
#include <cmath>
#include <boost/utility.hpp>
#include <raul/types.h>

namespace Raul {


/** A duration of time, with conversion between tick time and beat time.
 *
 * This is a slice along a single timeline (ie t=0 in ticks and t=0 in beats
 * are equal).  Relation to an external time base (e.g. Jack frame time) is
 * represented by frame_offset (the idea is that this holds all the information
 * necessary for passing to run() methods so they know the current state of
 * things WRT time).
 *
 * This class handles conversion between two units of time: musical
 * (beat) time, and real (tick) time.  Real time is discrete, the smallest
 * unit of time is the 'tick' (usually audio frames or MIDI ticks).  Beat time
 * is stored as a double (to be independent of any rates or timer precision).
 *
 * This caches as many values as possible to make calls less expensive, pass it
 * around by reference, not value.
 *
 * \ingroup raul
 */
class TimeSlice : public boost::noncopyable {
public:
	TimeSlice(double tick_rate, double bpm)
		: _tick_rate(tick_rate)
		, _beat_rate(60.0/bpm)
		, _start_ticks(0)
		, _length_ticks(0)
		, _start_beats(0)
		, _length_beats(0)
		, _offset_ticks(0)
	{}


	/** Set the start and length of the slice.
	 *
	 * Note that external offset is not affected by this, don't forget to reset
	 * the offset each cycle!
	 */
	void set_window(TickTime start, TickCount length) {
		_start_ticks = start;
		_length_ticks = length;
		update_beat_time();
	}

	void set_start(TickTime time) { _start_ticks = time; update_beat_time(); }

	void set_length(TickCount length) { _length_ticks = length; update_beat_time(); }

	bool contains(TickTime time) {
		return (time >= start_ticks() && time < start_ticks() + length_ticks());
	}

	double tick_rate() { return _tick_rate; }
	double beat_rate() { return _beat_rate; }
	double bpm()       { return 60/_beat_rate; }

	void set_tick_rate(double tick_rate) {
		_tick_rate = tick_rate;
		update_beat_time();
	}

	void set_bpm(double bpm) {
		_beat_rate = 60.0/bpm;
		update_beat_time();
	}
	
	inline Seconds beats_to_seconds(BeatTime beats) const {
		return (beats * _beat_rate);
	}

	inline TickTime beats_to_ticks(BeatTime beats) const {
		return static_cast<TickTime>(floor(beats_to_seconds(beats) / _tick_rate)); 
	}
		
	inline Seconds ticks_to_seconds(TickTime ticks) const {
		return (ticks * _tick_rate);
	}

	inline BeatTime ticks_to_beats(TickTime ticks) const {
		return ticks_to_seconds(ticks) / _beat_rate;
	}
	
	/** Start of current sub-cycle in ticks */
	inline TickTime start_ticks() const { return _start_ticks; }

	/** Length of current sub-cycle in ticks */
	inline TickCount length_ticks() const { return _length_ticks; }

	/** Start of current sub-cycle in beats */
	inline BeatTime start_beats() const { return _start_beats; }

	/** Length of current sub-cycle in beats */
	inline BeatCount length_beats() const { return _length_beats; }


	// Real-time conversion
/*
	TickCount ticks_to_offset(TickTime time) {
		assert(time >= _start_ticks);
		TickCount ret = time - _start_ticks + _offset_ticks;
		assert(ret < _offset_ticks + _length_ticks);
		return ret;
	}
*/
	/** Set the offset between real-time and timeslice-time. */
	inline void set_offset(TickCount offset) { _offset_ticks = offset; }
	/** Offset relative to external (e.g Jack) time */
	inline TickCount offset_ticks() const { return _offset_ticks; }

private:

	inline void update_beat_time() {
		_start_beats  = ticks_to_beats(_start_ticks);
		_length_beats = ticks_to_beats(_length_ticks);
	}

	// Rate/Tempo
	double _tick_rate; ///< Tick rate in Hz (e.g. sample rate)
	double _beat_rate; ///< Beat rate in Hz

	// Current time
	TickTime  _start_ticks;  ///< Current window start in ticks
	TickCount _length_ticks; ///< Current window length in ticks
	BeatTime  _start_beats;  ///< Current window start in beats
	BeatCount _length_beats; ///< Current window length in beats
	
	TickCount _offset_ticks; ///< Offset to global time (ie Jack sub-cycle offset)
};


} // namespace Raul

#endif // RAUL_TIMESLICE_H
