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

#ifndef RAUL_SEMAPHORE_HPP
#define RAUL_SEMAPHORE_HPP

#include <semaphore.h>
#include <boost/utility.hpp>

namespace Raul {


/** Trivial wrapper around POSIX semaphores (zero memory overhead).
 *
 * \ingroup raul
 */
class Semaphore : boost::noncopyable {
public:
	inline Semaphore(unsigned int initial) { sem_init(&_sem, 0, initial); }
	
	inline ~Semaphore() { sem_destroy(&_sem); }

	inline void reset(unsigned int initial) {
		sem_destroy(&_sem);
		sem_init(&_sem, 0, initial);
	}

	inline bool has_waiter() {
		int val;
		sem_getvalue(&_sem, &val);
		return (val <= 0);
	}

	/** Increment (and signal any waiters).
	 * 
	 * Realtime safe.
	 */
	inline void post() { sem_post(&_sem); }

	/** Wait until count is > 0, then decrement.
	 *
	 * Note that sem_wait always returns 0 in practise.  It returns nonzero
	 * when run in GDB, so the while is necessary to allow debugging.
	 *
	 * Obviously not realtime safe.
	 */
	inline void wait() { while (sem_wait(&_sem) != 0) ; }
	
	/** Non-blocking version of wait().
	 *
	 * \return true if decrement was successful (lock was acquired).
	 *
	 * Realtime safe?
	 */
	inline bool try_wait() { return (sem_trywait(&_sem) == 0); }

private:
	sem_t _sem;
};


} // namespace Raul

#endif // RAUL_SEMAPHORE_HPP
