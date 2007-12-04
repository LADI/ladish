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

#ifndef RAUL_COMMAND_HPP
#define RAUL_COMMAND_HPP

#include <raul/Semaphore.hpp>
#include <boost/utility.hpp>

namespace Raul {


/** A blocking command to be executed in the audio thread.
 *
 * This is useful for calling simple parameterless commands from another thread
 * (OSC, GUI, etc) and waiting on the result.  Works well for coarsely timed
 * events.  Realtime safe on the commend executing side.
 *
 * \ingroup raul
 */
class Command : boost::noncopyable {
public:

	inline Command() : _sem(0) {}

	/** Caller context */
	inline void operator()() { _sem.wait(); }

	/** Execution context */
	inline bool pending() { return _sem.has_waiter(); }
	inline void finish() { _sem.post(); }

private:
	Semaphore _sem;
};


} // namespace Raul

#endif // RAUL_COMMAND_HPP
