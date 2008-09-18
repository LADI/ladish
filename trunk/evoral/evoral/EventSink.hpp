/* This file is part of Evoral.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
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

#ifndef EVORAL_EVENT_SINK_HPP
#define EVORAL_EVENT_SINK_HPP

#include <stdexcept>

namespace Evoral {


/** Pure virtual base for anything you can write events to.
 */
class EventSink {
public:
	virtual void write(timestamp_t    time,
	                   uint32_t       size,
	                   const uint8_t* buf) throw (std::logic_error) = 0;
};


} // namespace Raul

#endif // EVORAL_EVENT_SINK_HPP

