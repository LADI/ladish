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

#ifndef RAUL_SMFWRITER_H
#define RAUL_SMFWRITER_H

#include <stdexcept>
#include <raul/MIDISink.h>

namespace Raul {


/** Standard Midi File (Type 0) Writer
 */
class SMFWriter : public Raul::MIDISink {
public:
	SMFWriter(unsigned short ppqn=1920);
	~SMFWriter();

	bool start(const std::string& filename,
	           BeatTime           start_time=0) throw (std::logic_error);

	void write_event(BeatTime             time,
	                 size_t               ev_size,
	                 const unsigned char* ev) throw (std::logic_error);

	void flush();

	void finish() throw (std::logic_error);

protected:
	static const uint32_t VAR_LEN_MAX = 0x0FFFFFFF;

	void write_header();
	void write_footer();

	void     write_chunk_header(char id[4], uint32_t length);
	void     write_chunk(char id[4], uint32_t length, void* data);
	size_t   write_var_len(uint32_t val);
	//uint32_t read_var_len() const;
	//int      read_event(MidiEvent& ev) const;

	std::string    _filename;
	FILE*          _fd;
	uint16_t       _ppqn;
	Raul::BeatTime _start_time;
	Raul::BeatTime _last_ev_time; ///< Time last event was written relative to _start_time
	uint32_t       _track_size;
	uint32_t       _header_size; ///< size of SMF header, including MTrk chunk header
};


} // namespace Raul

#endif // RAUL_SMFWRITER_H

