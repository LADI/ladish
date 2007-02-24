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

#ifndef RAUL_DOUBLE_BUFFER_H
#define RAUL_DOUBLE_BUFFER_H

#include <boost/utility.hpp>
#include <raul/AtomicInt.h>
#include <raul/AtomicPtr.h>

namespace Raul {

/** Double buffer.
 *
 * Can be thought of as a wrapper class to make a non-atomic type atomically
 * settable (with no locking).
 *
 * Read/Write realtime safe, many writers safe - but set calls may fail.
 *
 * Space:  2*sizeof(T) + sizeof(int) + sizeof(void*)
 */
template<typename T>
class DoubleBuffer : public boost::noncopyable {
public:
	
	inline DoubleBuffer(T val)
		: _state(READ_WRITE)
	{
		_vals[0] = val;
		_read_val = &_vals[0];
	}

	inline T& get()
	{
		return *_read_val.get();
	}

	inline bool set(T new_val)
	{
		if (_state.compare_and_exchange(READ_WRITE, READ_LOCK)) {

			// locked _vals[1] for write
			_vals[1] = new_val;
			_read_val = &_vals[1];
			_state = WRITE_READ;
			return true;

			// concurrent calls here are fine.  good, actually - caught
			// the WRITE_READ state immediately after it was set above

		} else if (_state.compare_and_exchange(WRITE_READ, LOCK_READ)) {

			// locked _vals[0] for write
			_vals[0] = new_val;
			_read_val = &_vals[0];
			_state = READ_WRITE;
			return true;

		} else {

			return false;

		}
	}

private:
	enum States {
		// vals[0] state _ vals[1] state
		READ_WRITE = 0,
		READ_LOCK,
		WRITE_READ,
		LOCK_READ
	};

	AtomicInt    _state;
	AtomicPtr<T> _read_val;
	T            _vals[2];
};


} // namespace Raul

#endif // RAUL_DOUBLE_BUFFER_H
