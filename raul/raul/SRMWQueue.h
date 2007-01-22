/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
 * 
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef RAUL_SRMW_QUEUE_H
#define RAUL_SRMW_QUEUE_H

#include <cassert>
#include <cstdlib>
#include <boost/utility.hpp>
#include "raul/AtomicInt.h"

#include <iostream>
using namespace std;


/** Realtime-safe single-reader multi-writer queue (aka lock-free ringbuffer)
 *
 * Implemented as a dequeue in a fixed array.  Both push and pop are realtime
 * safe, but only push is threadsafe.  In other words, multiple threads can push
 * data into this queue for a single thread to consume.
 *
 * The interface is intentionally as similar to std::queue as possible, but
 * note the additional thread restrictions imposed (e.g. empty() is only 
 * legal to call in the read thread).
 *
 * Obey the threading restrictions documented here, or horrible nasty (possibly
 * undetected) errors will occur.
 *
 * This is slightly more expensive than the SRMWSRMWQueue.  Use that if you don't
 * require multiple writers.
 *
 * \ingroup raul
 */
template <typename T>
class SRMWQueue : boost::noncopyable
{
public:
	SRMWQueue(size_t size);
	~SRMWQueue();
	

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

	// Note that _front doesn't need to be an AtomicInt since it's only accessed
	// by the (single) reader thread
	
	int       _front;   ///< Circular index of element at front of queue (READER ONLY)
	AtomicInt _back;    ///< Circular index 1 past element at back of queue (WRITERS ONLY)
	AtomicInt _space;   ///< Remaining free space for new elements (all threads)
	const int _size;    ///< Size of @ref _objects (you can store _size-1 objects)
	T* const  _objects; ///< Fixed array containing queued elements
};


template<typename T>
SRMWQueue<T>::SRMWQueue(size_t size)
	: _front(0)
	, _back(0)
	, _space(size)
	, _size(size+1)
	, _objects((T*)calloc(_size, sizeof(T)))
{
	assert(size > 1);
	assert(_size-1 == _space.get());
}


template <typename T>
SRMWQueue<T>::~SRMWQueue()
{
	free(_objects);
}


/** Return whether the queue is full.
 *
 * Write thread(s) only.
 */
template <typename T>
inline bool
SRMWQueue<T>::full() const
{
	return ( _space.get() <= 0 );
}


/** Push an item onto the back of the SRMWQueue - realtime-safe, not thread-safe.
 *
 * Write thread(s) only.
 *
 * @returns true if @a elem was successfully pushed onto the queue,
 * false otherwise (queue is full).
 */
template <typename T>
inline bool
SRMWQueue<T>::push(const T& elem)
{
	const int old_space = _space.exchange_and_add(-1);

	const bool already_full = ( old_space <= 0 );

	/* Technically right here pop could be called in the reader thread and
	 * make space available, but no harm in failing anyway - this queue
	 * really isn't designed to be filled... */

	if (already_full) {
		
		/* if multiple threads simultaneously get here, _space may be 0
		 * or negative.  The next call to pop() will set _space back to
		 * a sane value.  Note that _space is not exposed, so this is okay
		 * (... assuming this code is correct) */

		return false;

	} else {
		
		const int write_index = _back.exchange_and_add(1) % _size;
		_objects[write_index] = elem;
	
		return true;

	}
}


/** Return whether the queue is empty.
 *
 * Read thread only.
 */
template <typename T>
inline bool
SRMWQueue<T>::empty() const
{
	return ( _space == capacity() );
}


/** Return the element at the front of the queue without removing it.
 *
 * It is a fatal error to call front() when the queue is empty.
 * Read thread only.
 */
template <typename T>
inline T&
SRMWQueue<T>::front() const
{
	return _objects[_front];
}


/** Pop an item off the front of the queue - realtime-safe, NOT thread-safe.
 *
 * It is a fatal error to call pop() if the queue is empty.
 * Read thread only.
 *
 * @return true if queue is now empty, otherwise false.
 */
template <typename T>
inline void
SRMWQueue<T>::pop()
{
	_front = (_front + 1) % (_size);

	if (_space.get() < 0)
		_space = 1;
	else
		++_space;
}


#endif // RAUL_SRMW_QUEUE_H
