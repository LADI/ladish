/* This file is part of Machina.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Machina is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Machina is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MACHINA_JACKDRIVER_HPP
#define MACHINA_JACKDRIVER_HPP

#include <raul/JackDriver.h>
#include <raul/SharedPtr.h>
#include <jack/midiport.h>
#include "Machine.hpp"
#include "MidiDriver.hpp"
#include <boost/enable_shared_from_this.hpp>

namespace Machina {

class MidiAction;
class Node;


/** Realtime JACK Driver.
 *
 * "Ticks" are individual frames when running under this driver, and all code
 * in the processing context must be realtime safe (non-blocking).
 */
class JackDriver : public Raul::JackDriver, public Machina::MidiDriver,
                   public boost::enable_shared_from_this<JackDriver> {
public:
	JackDriver();

	void attach(const std::string& client_name);
	void detach();

	void set_machine(SharedPtr<Machine> machine) { _machine = machine; }
	
	// Audio context
	Timestamp    cycle_start()  { return _current_cycle_start; }
	FrameCount   cycle_length() { return _current_cycle_nframes; }
	
	void write_event(Timestamp            time,
	                 size_t               size,
	                 const unsigned char* event);
	
private:
	// Audio context
	Timestamp    subcycle_offset() { return _current_cycle_offset; }
	Timestamp    stamp_to_offset(Timestamp stamp);

	void         process_input(jack_nframes_t nframes);
	virtual void on_process(jack_nframes_t nframes);

	SharedPtr<Machine> _machine;

	jack_port_t* _input_port;
	jack_port_t* _output_port;
	Timestamp    _current_cycle_start; ///< in machine relative time
	Timestamp    _current_cycle_offset; ///< for split cycles
	FrameCount   _current_cycle_nframes;
};


} // namespace Machina

#endif // MACHINA_JACKDRIVER_HPP
