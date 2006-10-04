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

#ifndef RAUL_CONDITION_H
#define RAUL_CONDITION_H

#include <pthread.h>


/** Trivial (but pretty) wrapper around POSIX Conditions (zero overhead).
 *
 * A semaphore that isn't a counter, is slow, and not realtime safe.  Yay.
 */
class Condition {
public:
	inline Condition() { pthread_cond_init(&_cond, NULL); }
	
	inline ~Condition() { pthread_cond_destroy(&_cond); }
	
	inline void signal()           { pthread_cond_signal(&_cond); }
	inline void wait(Mutex& mutex) { pthread_cond_wait(&_cond, &mutex._mutex); }

private:
	pthread_cond_t _cond;
};


#endif // RAUL_CONDITION_H

