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

#include <boost/enable_shared_from_this.hpp>
#include <jack/midiport.h>
#include "raul/JackDriver.hpp"
#include "raul/SharedPtr.hpp"
#include "raul/DoubleBuffer.hpp"
#include "raul/EventRingBuffer.hpp"
#include "raul/Semaphore.hpp"
#include "raul/Command.hpp"
#include "Machine.hpp"
#include "Driver.hpp"
#include "Recorder.hpp"

namespace Machina {

class MidiAction;
class Node;


/** Realtime JACK Driver.
 *
 * "Ticks" are individual frames when running under this driver, and all code
 * in the processing context must be realtime safe (non-blocking).
 */
class JackDriver : public Raul::JackDriver,
                   public Machina::Driver,
                   public boost::enable_shared_from_this<JackDriver> {
public:
	JackDriver(SharedPtr<Machine> machine = SharedPtr<Machine>());
	~JackDriver();

	void attach(const std::string& client_name);
	void detach();
	
	void activate()   { Raul::JackDriver::activate(); }
	void deactivate() { Raul::JackDriver::deactivate(); }

	void set_machine(SharedPtr<Machine> machine);
	
	void write_event(Raul::TimeStamp      time,
	                 size_t               size,
	                 const unsigned char* event) throw (std::logic_error);
	
	void set_bpm(double bpm)        { _bpm.set(bpm); }
	void set_quantization(double q) { _quantization.set(q); }

	void stop();

	bool recording() { return _recording.get(); }
	void start_record();
	void finish_record();

private:
	void process_input(SharedPtr<Machine>     machine,
	                   const Raul::TimeSlice& time);

	virtual void on_process(jack_nframes_t nframes);

	Raul::Semaphore    _machine_changed;
	SharedPtr<Machine> _last_machine;

	jack_port_t* _input_port;
	jack_port_t* _output_port;
	
	Raul::TimeUnit  _frames_unit;
	Raul::TimeUnit  _beats_unit;
	Raul::TimeSlice _cycle_time;

	Raul::DoubleBuffer<double> _bpm;
	Raul::DoubleBuffer<double> _quantization;

	Raul::Command _stop;

	Raul::TimeStamp     _record_time;
	Raul::AtomicInt     _recording;
	SharedPtr<Recorder> _recorder;
};


} // namespace Machina

#endif // MACHINA_JACKDRIVER_HPP
