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

#ifndef RAUL_SRSW_QUEUE_HPP
#define RAUL_SRSW_QUEUE_HPP

#include <cassert>
#include <cstdlib>
#include <boost/utility.hpp>
#include <raul/AtomicInt.hpp>

namespace Raul {


/** Realtime-safe single-reader single-writer queue (aka lock-free ringbuffer)
 *
 * Implemented as a dequeue in a fixed array.  This is read/write thread-safe, 
 * pushing and popping may occur simultaneously by seperate threads, but
 * the push and pop operations themselves are not thread-safe (ie. there can
 * be at most 1 read and at most 1 writer thread).
 *
 * \ingroup raul
 */
template <typename T>
class SRSWQueue : boost::noncopyable
{
public:
	SRSWQueue(size_t size);
	~SRSWQueue();
	
	// Any thread:
	
	inline size_t capacity() const { return _size-1; }

	
	// Write thread(s):

	inline bool full() const;
	inline bool push(const T& obj);
	

	// Read thread:
	
	inline bool empty() const;
	inline T&   front() const;
	inline void pop();
	
private:
	volatile size_t _front;   ///< Index to front of queue (circular)
	volatile size_t _back;    ///< Index to back of queue (one past last element) (circular)
	const    size_t _size;    ///< Size of @ref _objects (you can store _size-1 objects)
	T* const        _objects; ///< Fixed array containing queued elements
};


template<typename T>
SRSWQueue<T>::SRSWQueue(size_t size)
: _front(0),
  _back(0),
  _size(size+1),
  _objects((T*)calloc(_size, sizeof(T)))
{
	assert(size > 1);
}


template <typename T>
SRSWQueue<T>::~SRSWQueue()
{
	free(_objects);
}


/** Return whether or not the queue is empty.
 */
template <typename T>
inline bool
SRSWQueue<T>::empty() const
{
	return (_back == _front);
}


/** Return whether or not the queue is full.
 */
template <typename T>
inline bool
SRSWQueue<T>::full() const
{
	// FIXME: uses both _front and _back - thread safe?
	return ( ((_front - _back + _size) % _size) == 1 );
}


/** Return the element at the front of the queue without removing it
 */
template <typename T>
inline T&
SRSWQueue<T>::front() const
{
	return _objects[_front];
}


/** Push an item onto the back of the SRSWQueue - realtime-safe, not thread-safe.
 *
 * @returns true if @a elem was successfully pushed onto the queue,
 * false otherwise (queue is full).
 */
template <typename T>
inline bool
SRSWQueue<T>::push(const T& elem)
{
	if (full()) {
		return false;
	} else {
		_objects[_back] = elem;
		_back = (_back + 1) % (_size);
		return true;
	}
}


/** Pop an item off the front of the queue - realtime-safe, not thread-safe.
 *
 * It is a fatal error to call pop() when the queue is empty.
 *
 * @returns the element popped.
 */
template <typename T>
inline void
SRSWQueue<T>::pop()
{
	assert(!empty());
	assert(_size > 0);

	_front = (_front + 1) % (_size);
}


} // namespace Raul

#endif // RAUL_SRSW_QUEUE_HPP
