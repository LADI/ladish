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

#ifndef RAUL_SMF_READER_HPP
#define RAUL_SMF_READER_HPP

#include <stdexcept>
#include <string>
#include <inttypes.h>

namespace Raul {


/** Standard Midi File (Type 0) Reader
 *
 * Currently this only reads SMF files with tempo-based timing.
 */
class SMFReader {
public:
	SMFReader();
	~SMFReader();

	bool open(const std::string& filename);

	bool seek_to_track(unsigned track) throw (std::logic_error);

	uint16_t type() const { return _type; }
	uint16_t ppqn() const { return _ppqn; }
	size_t   num_tracks() { return _num_tracks; }

	int read_event(size_t    buf_len,
	               uint8_t*  buf,
	               uint32_t* ev_size,
	               uint32_t* ev_delta_time) throw (std::logic_error);
	
	void close();

protected:
	/** size of SMF header, including MTrk chunk header */
	static const uint32_t HEADER_SIZE = 22;

	uint32_t read_var_len() const;

	std::string    _filename;
	FILE*          _fd;
	uint16_t       _type;
	uint16_t       _ppqn;
	uint16_t       _num_tracks;
	uint32_t       _track;
	uint32_t       _track_size;
};


} // namespace Raul

#endif // RAUL_SMF_READER_HPP

