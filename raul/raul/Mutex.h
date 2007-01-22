/* This file is part of Raul.  Copyright (C) 2007 Dave Robillard.
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

#ifndef RAUL_MUTEX_H
#define RAUL_MUTEX_H

#include <pthread.h>
#include <boost/utility.hpp>

namespace Raul {


/** Trivial (zero overhead) wrapper around POSIX Mutexes.
 *
 * \ingroup raul
 */
class Mutex : boost::noncopyable {
public:
	inline Mutex() { pthread_mutex_init(&_mutex, NULL); }
	
	inline ~Mutex() { pthread_mutex_destroy(&_mutex); }

	inline bool try_lock() { return (pthread_mutex_trylock(&_mutex) == 0); }
	inline void lock()     { pthread_mutex_lock(&_mutex); }
	inline void unlock()   { pthread_mutex_unlock(&_mutex); }

private:
	friend class Condition;
	pthread_mutex_t _mutex;
};


} // namespace Raul

#endif // RAUL_MUTEX_H
