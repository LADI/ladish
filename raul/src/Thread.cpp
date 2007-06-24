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

#include "raul/Thread.h"

namespace Raul {

/* Thread-specific data key (once-only initialized) */
pthread_once_t Thread::_thread_key_once = PTHREAD_ONCE_INIT;
pthread_key_t  Thread::_thread_key;


Thread::Thread(const std::string& name)
	: _context(0)
	, _name(name)
	, _pthread_exists(false)
{
	pthread_once(&_thread_key_once, thread_key_alloc);
	pthread_setspecific(_thread_key, this);
}


/** Must be called from thread */
Thread::Thread(pthread_t thread, const std::string& name)
	: _context(0)
	, _name(name)
	, _pthread_exists(true)
	, _pthread(thread)
{
	pthread_once(&_thread_key_once, thread_key_alloc);
	pthread_setspecific(_thread_key, this);
}


/** Return the calling thread.
 * The return value of this should NOT be cached unless the thread is
 * explicitly user created with create().
 */
Thread&
Thread::get()
{
	Thread* this_thread = reinterpret_cast<Thread*>(pthread_getspecific(_thread_key));
	if (!this_thread)
		this_thread = new Thread(); // sets thread-specific data

	return *this_thread;
}

/** Launch and start the thread. */
void
Thread::start()
{
	std::cout << "[" << _name << " Thread] Starting." << std::endl;

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 1500000);

	pthread_create(&_pthread, &attr, _static_run, this);
	_pthread_exists = true;
}

/** Stop and terminate the thread. */
void
Thread::stop()
{
	if (_pthread_exists) {
		pthread_cancel(_pthread);
		pthread_join(_pthread, NULL);
		_pthread_exists = false;
		std::cout << "[" << _name << " Thread] Exiting." << std::endl;
	}
}

void
Thread::set_scheduling(int policy, unsigned int priority)
{
	sched_param sp;
	sp.sched_priority = priority;
	int result = pthread_setschedparam(_pthread, SCHED_FIFO, &sp);
	if (!result) {
		std::cout << "[" << _name << "] Set scheduling policy to ";
		switch (policy) {
			case SCHED_FIFO:  std::cout << "SCHED_FIFO";  break;
			case SCHED_RR:    std::cout << "SCHED_RR";    break;
			case SCHED_OTHER: std::cout << "SCHED_OTHER"; break;
			default:          std::cout << "UNKNOWN";     break;
		}
		std::cout << ", priority " << sp.sched_priority << std::endl;
	} else {
		std::cout << "[" << _name << "] Unable to set scheduling policy ("
			<< strerror(result) << ")" << std::endl;
	}
}


} // namespace Raul

