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

#ifndef RAUL_SMF_WRITER_HPP
#define RAUL_SMF_WRITER_HPP

#include <stdexcept>
#include <raul/MIDISink.hpp>
#include <raul/TimeStamp.hpp>

namespace Raul {


/** Standard Midi File (Type 0) Writer
 */
class SMFWriter : public Raul::MIDISink {
public:
	SMFWriter(TimeUnit unit);
	~SMFWriter();

	bool start(const std::string& filename,
	           TimeStamp          start_time) throw (std::logic_error);

	TimeUnit unit() const { return _unit; }

	void write_event(TimeStamp            time,
	                 size_t               ev_size,
	                 const unsigned char* ev) throw (std::logic_error);

	void flush();

	void finish() throw (std::logic_error);

protected:
	static const uint32_t VAR_LEN_MAX = 0x0FFFFFFF;

	void write_header();
	void write_footer();

	void     write_chunk_header(const char id[4], uint32_t length);
	void     write_chunk(const char id[4], uint32_t length, void* data);
	size_t   write_var_len(uint32_t val);

	std::string     _filename;
	FILE*           _fd;
	TimeUnit        _unit;
	Raul::TimeStamp _start_time;
	Raul::TimeStamp _last_ev_time; ///< Time last event was written relative to _start_time
	uint32_t        _track_size;
	uint32_t        _header_size; ///< size of SMF header, including MTrk chunk header
};


} // namespace Raul

#endif // RAUL_SMF_WRITER_HPP

