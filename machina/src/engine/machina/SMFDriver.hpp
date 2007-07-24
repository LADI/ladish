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

#ifndef MACHINA_SMFDRIVER_HPP
#define MACHINA_SMFDRIVER_HPP

#include <boost/enable_shared_from_this.hpp>
#include <glibmm/ustring.h>
#include <raul/SharedPtr.hpp>
#include <raul/SMFWriter.hpp>
#include <raul/SMFReader.hpp>
#include "machina/types.hpp"
#include "machina/Driver.hpp"
#include "machina/MachineBuilder.hpp"

namespace Machina {

class Node;
class Machine;


class SMFDriver : public Driver,
                  public boost::enable_shared_from_this<SMFDriver> {
public:
	SMFDriver(SharedPtr<Machine> machine = SharedPtr<Machine>());

	SharedPtr<Machine> learn(const std::string& filename,
	                         double             q=0.0,
	                         Raul::BeatTime     max_duration=0);

	SharedPtr<Machine> learn(const std::string& filename,
	                         unsigned           track,
	                         double             q=0.0,
	                         Raul::BeatTime     max_duration=0);

	void run(SharedPtr<Machine> machine, Raul::BeatTime max_time);
	
	void write_event(Raul::BeatTime       time,
	                 size_t               ev_size,
	                 const unsigned char* ev) throw (std::logic_error)
	{ _writer->write_event(time, ev_size, ev); }
	
	void set_bpm(double /*bpm*/)                   { }
	void set_quantization(double /*quantization*/) { }

	SharedPtr<Raul::SMFWriter> writer() { return _writer; }

private:
	SharedPtr<Raul::SMFWriter> _writer;
	
	void learn_track(SharedPtr<MachineBuilder> builder,
	                 Raul::SMFReader&          reader,
	                 unsigned                  track,
	                 double                    q,
	                 Raul::BeatTime            max_duration=0);
};


} // namespace Machina

#endif // MACHINA_SMFDRIVER_HPP

