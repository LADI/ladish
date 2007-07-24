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

#ifndef RAUL_STAMPED_CHUNK_RING_BUFFER_HPP
#define RAUL_STAMPED_CHUNK_RING_BUFFER_HPP

#include <cassert>
#include <algorithm>
#include <glib.h>
#include <raul/types.hpp>
#include <raul/RingBuffer.hpp>

namespace Raul {


/** A RingBuffer of timestamped binary "chunks".
 *
 * This packs a timestamp, size, and size bytes of data flat into the buffer.
 * Useful for MIDI events, OSC messages, etc.
 */
class StampedChunkRingBuffer : private Raul::RingBuffer<Byte> {
public:

	/** @param size Size in bytes.
	 */
	StampedChunkRingBuffer(size_t size)
		: RingBuffer<Byte>(size)
	{}

	size_t capacity() const { return _size; }

	size_t write(TickTime time, size_t size, const Byte* buf);
	bool   read(TickTime* time, size_t* size, Byte* buf);
};


inline bool
StampedChunkRingBuffer::read(TickTime* time, size_t* size, Byte* buf)
{
	bool success = RingBuffer<Byte>::full_read(sizeof(TickTime), (Byte*)time);
	if (success)
		success = RingBuffer<Byte>::full_read(sizeof(size_t), (Byte*)size);
	if (success)
		success = RingBuffer<Byte>::full_read(*size, buf);

	return success;
}


inline size_t
StampedChunkRingBuffer::write(TickTime time, size_t size, const Byte* buf)
{
	assert(size > 0);

	if (write_space() < (sizeof(TickTime) + sizeof(size_t) + size)) {
		return 0;
	} else {
		RingBuffer<Byte>::write(sizeof(TickTime), (Byte*)&time);
		RingBuffer<Byte>::write(sizeof(size_t), (Byte*)&size);
		RingBuffer<Byte>::write(size, buf);
		return size;
	}
}


} // namespace Raul

#endif // RAUL_STAMPED_CHUNK_RING_BUFFER_HPP

