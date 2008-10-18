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

#include <stdint.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <glibmm/miscutils.h>
#include "raul/SMFWriter.hpp"

using namespace std;

namespace Raul {


/** Create a new SMF writer.
 *
 * @a unit must match the time stamp of ALL events passed to write, or
 * terrible things will happen.
 *
 * *** NOTE: Only beat time is implemented currently.
 */
SMFWriter::SMFWriter(TimeUnit unit)
	: _fd(NULL)
	, _unit(unit)
	, _start_time(unit, 0, 0)
	, _last_ev_time(unit, 0, 0)
	, _track_size(0)
	, _header_size(0)
{
	if (unit.type() == TimeUnit::BEATS)
		assert(unit.ppt() < std::numeric_limits<uint16_t>::max());
}


SMFWriter::~SMFWriter()
{
	if (_fd)
		finish();
}


/** Start a write to an SMF file.
 *
 * @a filename Filename to write to.
 * @a start_time Beat time corresponding to t=0 in the file (timestamps passed
 *    to write_event will have this value subtracted before writing).
 */
bool
SMFWriter::start(const string&   filename,
                 Raul::TimeStamp start_time) throw (logic_error)
{
	if (_fd)
		throw logic_error("Attempt to start new write while write in progress.");

	cerr << "Opening SMF file " << filename << " for writing." << endl;

	_fd = fopen(filename.c_str(), "w+");

	if (_fd) { 
		_track_size = 0;
		_filename = filename;
		_start_time = start_time;
		_last_ev_time = 0;
		// write a tentative header to pad file out so writing starts at the right offset
		write_header();
	}

	return (_fd == 0) ? -1 : 0;
}


/** Write an event at the end of the file.
 *
 * @a time is the absolute time of the event, relative to the start of the file
 *    (the start_time parameter to start).  Must be monotonically increasing on
 *    successive calls to this method.
 */
void
SMFWriter::write_event(Raul::TimeStamp      time,
                       size_t               ev_size,
                       const unsigned char* ev) throw (logic_error)
{
	if (time < _start_time)
		throw logic_error("Event time is before file start time");
	else if (time < _last_ev_time)
		throw logic_error("Event time not monotonically increasing");
	else if (time.unit() != _unit)
		throw logic_error("Event has unexpected time unit");

	Raul::TimeStamp delta_time = time;
	delta_time -= _start_time;

	fseek(_fd, 0, SEEK_END);
	
	uint64_t delta_ticks = delta_time.ticks() * _unit.ppt() + delta_time.subticks();
	size_t stamp_size = 0;

	/* If delta time is too long (i.e. overflows), write out empty
	 * "proprietary" events to reach the desired time.
	 * Any SMF reading application should interpret this correctly
	 * (by accumulating the delta time and ignoring the event) */
	while (delta_ticks > VAR_LEN_MAX) {
		static unsigned char null_event[] = { 0xFF, 0x7F, 0x0 };
		stamp_size = write_var_len(VAR_LEN_MAX);
		fwrite(null_event, 1, 3, _fd);
		_track_size += stamp_size + 3;
		delta_ticks -= VAR_LEN_MAX;
	}
	
	assert(delta_ticks <= VAR_LEN_MAX);
	stamp_size = write_var_len((uint32_t)delta_ticks);
	fwrite(ev, 1, ev_size, _fd);

	_last_ev_time = time;
	_track_size += stamp_size + ev_size;
}


void
SMFWriter::flush()
{
	if (_fd)
		fflush(_fd);
}


void
SMFWriter::finish() throw (logic_error)
{
	if (!_fd)
		throw logic_error("Attempt to finish write with no write in progress.");

	write_footer();
	fclose(_fd);
	_fd = NULL;
}


void
SMFWriter::write_header()
{
	cerr << "SMF Flushing header\n";

	assert(_fd);

	const uint16_t type     = GUINT16_TO_BE(0);           // SMF Type 0 (single track)
	const uint16_t ntracks  = GUINT16_TO_BE(1);           // Number of tracks (always 1 for Type 0)
	const uint16_t division = GUINT16_TO_BE(_unit.ppt()); // PPQN

	char data[6];
	memcpy(data, &type, 2);
	memcpy(data+2, &ntracks, 2);
	memcpy(data+4, &division, 2);
	//data[4] = _ppqn & 0xF0;
	//data[5] = _ppqn & 0x0F;

	_fd = freopen(_filename.c_str(), "r+", _fd);
	assert(_fd);
	fseek(_fd, 0, 0);
	write_chunk("MThd", 6, data);
	write_chunk_header("MTrk", _track_size); 
}


void
SMFWriter::write_footer()
{
	cerr << "SMF - Writing EOT\n";

	fseek(_fd, 0, SEEK_END);
	write_var_len(1); // whatever...
	static const unsigned char eot[4] = { 0xFF, 0x2F, 0x00 }; // end-of-track meta-event
	fwrite(eot, 1, 4, _fd);
}


void
SMFWriter::write_chunk_header(const char id[4], uint32_t length)
{
	const uint32_t length_be = GUINT32_TO_BE(length);

	fwrite(id, 1, 4, _fd);
	fwrite(&length_be, 4, 1, _fd);
}


void
SMFWriter::write_chunk(const char id[4], uint32_t length, void* data)
{
	write_chunk_header(id, length);
	
	fwrite(data, 1, length, _fd);
}


/** Write an SMF variable length value.
 *
 * @return size (in bytes) of the value written.
 */
size_t
SMFWriter::write_var_len(uint32_t value)
{
	size_t ret = 0;

	uint32_t buffer = value & 0x7F;

	while ( (value >>= 7) ) {
		buffer <<= 8;
		buffer |= ((value & 0x7F) | 0x80);
	}

	while (true) {
		//printf("Writing var len byte %X\n", (unsigned char)buffer);
		++ret;
		fputc(buffer, _fd);
		if (buffer & 0x80)
			buffer >>= 8;
		else
			break;
	}

	return ret;
}


} // namespace Raul

