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

#ifndef RAUL_MIDI_RING_BUFFER_H
#define RAUL_MIDI_RING_BUFFER_H

#include <cassert>
#include <algorithm>
#include <glib.h>
#include <raul/types.h>
#include <raul/RingBuffer.h>

#include <iostream>

namespace Raul {


/** A MIDI RingBuffer
 */
class MIDIRingBuffer : private Raul::RingBuffer<char> {
public:

	struct MidiEvent {
		TickTime       time;
		size_t         size;
		unsigned char* buf;
	};


	/** @param size Size in bytes.
	 */
	MIDIRingBuffer(size_t size)
		: RingBuffer<char>(size)
	{
	}

	size_t capacity() const { return _size; }

	/** Read one event and appends it to @a out. */
	//size_t read(MidiBuffer& out);
	
	/** Read events all events up to time @a end into @a out, leaving stamps intact.
	 * Any events before @a start will be dropped. */
	//size_t read(MidiBuffer& out, TickTime start, TickTime end);

	/** Write one event */
	//size_t write(const MidiEvent& in); // deep copies in

	size_t write(TickTime time, size_t size, const char* buf);
	bool   read(TickTime* time, size_t* size, char* buf);

};


bool
MIDIRingBuffer::read(TickTime* time, size_t* size, char* buf)
{
	bool success = RingBuffer<char>::full_read(sizeof(TickTime), (char*)time);
	if (success)
		success = RingBuffer<char>::full_read(sizeof(size_t), (char*)size);
	if (success)
		success = RingBuffer<char>::full_read(*size, buf);

	return success;
}


inline size_t
MIDIRingBuffer::write(TickTime time, size_t size, const char* buf)
{
	assert(size > 0);

	if (write_space() < (sizeof(TickTime) + sizeof(size_t) + size)) {
		return 0;
	} else {
		RingBuffer<char>::write(sizeof(TickTime), (char*)&time);
		RingBuffer<char>::write(sizeof(size_t), (char*)&size);
		RingBuffer<char>::write(size, buf);
		return size;
	}
}

#if 0
inline size_t
MIDIRingBuffer::read(MidiBuffer& dst, TickTime start, TickTime end)
{
	if (read_space() == 0)
		return 0;

	size_t         priv_read_ptr = g_atomic_int_get(&_read_ptr);
	TickTime time          = _ev_buf[priv_read_ptr].time;
	size_t         count         = 0;
	size_t         limit         = read_space();

	while (time <= end && limit > 0) {
		MidiEvent* const read_ev = &_ev_buf[priv_read_ptr];
		if(time >= start) {
			dst.push_back(*read_ev);
			//printf("MRB - read %#X %d %d with time %u at index %zu\n",
			//	read_ev->buffer[0], read_ev->buffer[1], read_ev->buffer[2], read_ev->time,
			//	priv_read_ptr);
		} else {
			printf("MRB - SKIPPING - %#X %d %d with time %u at index %zu\n",
				read_ev->buffer[0], read_ev->buffer[1], read_ev->buffer[2], read_ev->time,
				priv_read_ptr);
			break;
		}

		clear_event(priv_read_ptr);

		++count;
		--limit;
		
		priv_read_ptr =(priv_read_ptr + 1) % _size;
		
		assert(read_ev->time <= end);
		time = _ev_buf[priv_read_ptr].time;
	}
	
	g_atomic_int_set(&_read_ptr, priv_read_ptr);
	
	//printf("(R) read space: %zu\n", read_space());

	return count;
}

inline size_t
MIDIRingBuffer::write(const MidiBuffer& in, TickTime time)
{
	const size_t num_events = in.size();
	const size_t to_write = std::min(write_space(), num_events);

	// FIXME: double copy :/
	for (size_t i=0; i < to_write; ++i) {
		MidiEvent ev = in[i];
		ev.time += offset;
		write(ev);
	}

	return to_write;
}
#endif


} // namespace Raul

#endif // RAUL_MIDI_RING_BUFFER_H

