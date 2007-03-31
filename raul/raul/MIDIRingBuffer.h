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

namespace Raul {


/** A MIDI RingBuffer
 */
class MIDIRingBuffer : private Raul::RingBuffer<Byte> {
public:

	/** @param size Size in bytes.
	 */
	MIDIRingBuffer(size_t size)
		: RingBuffer<Byte>(size)
	{}

	size_t capacity() const { return _size; }

	size_t write(TickTime time, size_t size, const Byte* buf);
	bool   read(TickTime* time, size_t* size, Byte* buf);
};


inline bool
MIDIRingBuffer::read(TickTime* time, size_t* size, Byte* buf)
{
	bool success = RingBuffer<Byte>::full_read(sizeof(TickTime), (Byte*)time);
	if (success)
		success = RingBuffer<Byte>::full_read(sizeof(size_t), (Byte*)size);
	if (success)
		success = RingBuffer<Byte>::full_read(*size, buf);

	return success;
}


inline size_t
MIDIRingBuffer::write(TickTime time, size_t size, const Byte* buf)
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

#endif // RAUL_MIDI_RING_BUFFER_H

