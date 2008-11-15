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

#ifndef RAUL_THREAD_HPP
#define RAUL_THREAD_HPP

#include <string>
#include <iostream>
#include <pthread.h>
#include <boost/utility.hpp>

namespace Raul {


/** Abstract base class for a thread.
 *
 * Extend this and override the _run method to easily create a thread
 * to perform some task.
 *
 * The current Thread can be accessed using the get() method.
 * 
 * \ingroup raul
 */
class Thread : boost::noncopyable
{
public:
	virtual ~Thread() {
		stop();
	}

	static Thread* create(const std::string& name="")
		{ return new Thread(name); }
	
	/** Must be called from thread */
	static Thread* create_for_this_thread(const std::string& name="")
		{ return new Thread(pthread_self(), name); }
	
	static Thread& get();

	virtual void start();
	virtual void stop();

	void set_scheduling(int policy, unsigned int priority);
	
	const std::string& name() const { return _name; }
	void set_name(const std::string& name) { _name = name; }
	
	unsigned context() const               { return _context; }
	void     set_context(unsigned context) { _context = context; }

protected:
	Thread(const std::string& name="");
	Thread(pthread_t thread, const std::string& name="");

	/** Thread function to execute.
	 *
	 * This is called once on start, and terminated on stop.
	 * Implementations likely want to put some infinite loop here.
	 *
	 * When _exit_flag becomes true (via a call to stop()) the loop
	 * should exit.
	 */
	virtual void _run() {}
	
	bool _exit_flag;

private:

	inline static void* _static_run(void* me) {
		pthread_setspecific(_thread_key, me);
		Thread* myself = (Thread*)me;
		myself->_run();
		myself->_pthread_exists = false;
		return NULL; // and I
	}

	/** Allocate thread-specific data key */
	static void thread_key_alloc() {
		pthread_key_create(&_thread_key, NULL);
	}

	/* Key for the thread-specific buffer */
	static pthread_key_t _thread_key;
	
	/* Once-only initialisation of the key */
	static pthread_once_t _thread_key_once;

	unsigned    _context;
	std::string _name;
	bool        _pthread_exists;
	pthread_t   _pthread;
};


} // namespace Raul

#endif // RAUL_THREAD_HPP
