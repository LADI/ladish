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

#ifndef RAUL_SRMW_QUEUE_HPP
#define RAUL_SRMW_QUEUE_HPP

#include <cassert>
#include <cstdlib>
#include <cmath>
#include <boost/utility.hpp>
#include <raul/AtomicInt.hpp>

#include <iostream>
using namespace std;

namespace Raul {


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
 * If you only need a single writer, use SRSWQueue.  This is slightly more
 * computationally expensive, and allocates an additional size words of memory (ie
 * if you're using this for ints or pointers etc, SRMWQueue will be twice the size
 * of SRSWQueue for the same queue size.  Additionally, the size of this queue must
 * be a power of 2 (SRSWQueue does not have this limitation).
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
	
	unsigned       _front; ///< Circular index of element at front of queue (READER ONLY)
	AtomicInt      _back; ///< Circular index 1 past element at back of queue (WRITERS ONLY)
	AtomicInt      _write_space; ///< Remaining free space for new elements (all threads)
	const unsigned _size; ///< Size of @ref _objects (you can store _size-1 objects)
	
	T* const         _objects; ///< Fixed array containing queued elements
	AtomicInt* const _valid; ///< Parallel array to _objects, whether loc is written or not
};


template<typename T>
SRMWQueue<T>::SRMWQueue(size_t size)
	: _front(0)
	, _back(0)
	, _write_space(size)
	, _size(size+1)
	, _objects((T*)calloc(_size, sizeof(T)))
	, _valid((AtomicInt*)calloc(_size, sizeof(AtomicInt)))
{
	assert(log2(size) - (int)log2(size) == 0);
	assert(size > 1);
	assert(_size-1 == (unsigned)_write_space.get());

	for (unsigned i=0; i < _size; ++i) {
		assert(_valid[i].get() == 0);
	}
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
	return (_write_space.get() <= 0);
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
	const int old_write_space = _write_space.exchange_and_add(-1);
	const bool already_full   = ( old_write_space <= 0 );

	/* Technically right here pop could be called in the reader thread and
	 * make space available, but no harm in failing anyway - this queue
	 * really isn't designed to be filled... */

	if (already_full) {
		
		/* if multiple threads simultaneously get here, _write_space may be 0
		 * or negative.  The next call to pop() will set _write_space back to
		 * a sane value.  Note that _write_space is not exposed, so this is okay
		 * (... assuming this code is correct) */

		return false;

	} else {
		
		// Note: _size must be a power of 2 for this to not explode when _back overflows
		const unsigned write_index = (unsigned)_back.exchange_and_add(1) % _size;
		
		assert(_valid[write_index] == 0);
		_objects[write_index] = elem;
		++(_valid[write_index]);
	
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
	return (_valid[_front].get() == 0);
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
	assert(_valid[_front] == 1);
	--(_valid[_front]);

	_front = (_front + 1) % (_size);

	if (_write_space.get() < 0)
		_write_space = 1;
	else
		++_write_space;
}


} // namespace Raul

#endif // RAUL_SRMW_QUEUE_HPP
