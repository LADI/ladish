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

#ifndef RAUL_TIME_SLICE_HPP
#define RAUL_TIME_SLICE_HPP

#include <cassert>
#include <cmath>
#include <boost/utility.hpp>
#include <raul/TimeStamp.hpp>
#include <raul/lv2_event.h>

namespace Raul {


/* FIXME: all the conversion here is wrong now */

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
	TimeSlice(uint32_t rate, double bpm)
		: _tick_rate(rate)
		, _beat_rate(60.0/bpm)
		, _start_ticks(Raul::TimeUnit(Raul::TimeUnit::FRAMES, rate), 0, 0)
		, _length_ticks(TimeUnit(TimeUnit::FRAMES, rate), 0, 0)
		, _start_beats(TimeUnit(TimeUnit::BEATS, LV2_EVENT_PPQN), 0, 0)
		, _length_beats(TimeUnit(TimeUnit::BEATS, LV2_EVENT_PPQN), 0, 0)
		, _offset_ticks(TimeUnit(TimeUnit::FRAMES, rate), 0, 0)
	{}


	/** Set the start and length of the slice.
	 *
	 * Note that external offset is not affected by this, don't forget to reset
	 * the offset each cycle!
	 */
	void set_window(TimeStamp start, TimeDuration length) {
		_start_ticks = start;
		_length_ticks = length;
		update_beat_time();
	}

	void set_start(TimeStamp time) { _start_ticks = time; update_beat_time(); }

	void set_length(TimeDuration length) { _length_ticks = length; update_beat_time(); }

	bool contains(TimeStamp time) {
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
	
	// FIXME
	
	inline TimeStamp beats_to_seconds(TimeStamp beats) const {
		//return (beats * _beat_rate);
		throw;
	}

	inline TimeStamp beats_to_ticks(TimeStamp beats) const {
		//return static_cast<TimeStamp>(floor(beats_to_seconds(beats) / _tick_rate)); 
		throw;
	}
		
	inline TimeStamp ticks_to_seconds(TimeStamp ticks) const {
		//return (ticks * _tick_rate);
		throw;
	}

	inline TimeStamp ticks_to_beats(TimeStamp ticks) const {
		//return ticks_to_seconds(ticks) / _beat_rate;
		throw;
	}
	
	/** Start of current sub-cycle in ticks */
	inline TimeStamp start_ticks() const { return _start_ticks; }

	/** Length of current sub-cycle in ticks */
	inline TimeDuration length_ticks() const { return _length_ticks; }

	/** Start of current sub-cycle in beats */
	inline TimeStamp start_beats() const { return _start_beats; }

	/** Length of current sub-cycle in beats */
	inline TimeDuration length_beats() const { return _length_beats; }

	/** Set the offset between real-time and timeslice-time. */
	inline void set_offset(TimeDuration offset) { _offset_ticks = offset; }

	/** Offset relative to external (e.g Jack) time */
	inline TimeDuration offset_ticks() const { return _offset_ticks; }

private:

	inline void update_beat_time() {
		_start_beats  = ticks_to_beats(_start_ticks);
		_length_beats = ticks_to_beats(_length_ticks);
	}

	// Rate/Tempo
	double _tick_rate; ///< Tick rate in Hz (e.g. sample rate)
	double _beat_rate; ///< Beat rate in Hz

	// Current time
	TimeStamp    _start_ticks;  ///< Current window start in ticks
	TimeDuration _length_ticks; ///< Current window length in ticks
	TimeStamp    _start_beats;  ///< Current window start in beats
	TimeDuration _length_beats; ///< Current window length in beats
	
	TimeDuration _offset_ticks; ///< Offset to global time (ie Jack sub-cycle offset)
};


} // namespace Raul

#endif // RAUL_TIME_SLICE_HPP
