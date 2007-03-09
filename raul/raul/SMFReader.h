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

#ifndef RAUL_SMFREADER_H
#define RAUL_SMFREADER_H

#include <stdexcept>

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

	uint16_t ppqn() const { return _ppqn; }

	size_t num_tracks() throw (std::logic_error);

	int read_event(size_t buf_len, char* buf, size_t* ev_size, uint64_t* ev_time);
	
	void close();

protected:
	//static const uint32_t VAR_LEN_MAX = 0x0FFFFFFF;
	
	/** size of SMF header, including MTrk chunk header */
	static const uint32_t HEADER_SIZE = 22;

	uint32_t read_var_len() const;

	std::string    _filename;
	FILE*          _fd;
	uint16_t       _ppqn;
	uint64_t       _last_ev_time;
	uint32_t       _track_size;
/*	Raul::BeatTime _start_time;
	Raul::BeatTime _last_ev_time; ///< Time last event was written relative to _start_time
	*/
};


} // namespace Raul

#endif // RAUL_SMFREADER_H

