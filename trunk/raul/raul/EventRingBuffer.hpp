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

#ifndef RAUL_EVENT_RING_BUFFER_HPP
#define RAUL_EVENT_RING_BUFFER_HPP

#include <cassert>
#include <algorithm>
#include <glib.h>
#include "raul/RingBuffer.hpp"
#include "raul/TimeStamp.hpp"

namespace Raul {


/** A RingBuffer of events (generic time-stamped binary "blobs").
 *
 * This packs a timestamp, size, and size bytes of data flat into the buffer.
 * Useful for MIDI events, OSC messages, etc.
 */
class EventRingBuffer : private Raul::RingBuffer<uint8_t> {
public:

	/** @param capacity Ringbuffer capacity in bytes.
	 */
	EventRingBuffer(size_t capacity)
		: RingBuffer<uint8_t>(capacity)
	{}

	size_t capacity() const { return _size; }

	size_t write(TimeStamp time, size_t size, const uint8_t* buf);
	bool   read(TimeStamp* time, size_t* size, uint8_t* buf);
};


inline bool
EventRingBuffer::read(TimeStamp* time, size_t* size, uint8_t* buf)
{
	bool success = RingBuffer<uint8_t>::full_read(sizeof(TimeStamp), (uint8_t*)time);
	if (success)
		success = RingBuffer<uint8_t>::full_read(sizeof(size_t), (uint8_t*)size);
	if (success)
		success = RingBuffer<uint8_t>::full_read(*size, buf);

	return success;
}


inline size_t
EventRingBuffer::write(TimeStamp time, size_t size, const uint8_t* buf)
{
	assert(size > 0);

	if (write_space() < (sizeof(TimeStamp) + sizeof(size_t) + size)) {
		return 0;
	} else {
		RingBuffer<uint8_t>::write(sizeof(TimeStamp), (uint8_t*)&time);
		RingBuffer<uint8_t>::write(sizeof(size_t), (uint8_t*)&size);
		RingBuffer<uint8_t>::write(size, buf);
		return size;
	}
}


} // namespace Raul

#endif // RAUL_EVENT_RING_BUFFER_HPP

