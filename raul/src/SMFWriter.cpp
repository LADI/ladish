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

#include <cstdio>
#include <iostream>
#include <glibmm/miscutils.h>
#include "raul/SMFWriter.h"

using namespace std;

namespace Raul {


SMFWriter::SMFWriter(unsigned short ppqn)
	: _fd(NULL)
	, _ppqn(ppqn)
	, _last_ev_time(0)
	, _track_size(0)
	, _header_size(0)
{
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
SMFWriter::start(const string& filename,
                 Raul::BeatTime     start_time) throw (logic_error)
{
	if (_fd)
		throw logic_error("Attempt to start new write while write in progress.");

	cerr << "Opening SMF file " << filename << endl;

	_fd = fopen(filename.c_str(), "r+");

	// File already exists
	if (_fd) {
		return false;
		/*fseek(_fd, _header_size - 4, 0);
		uint32_t track_size_be = 0;
		fread(&track_size_be, 4, 1, _fd);
		_track_size = GUINT32_FROM_BE(track_size_be);
		cerr << "SMF - read track size " << _track_size;*/

	// Make a new file
	} else {
		_fd = fopen(filename.c_str(), "w+");

		if (_fd) { 
			_track_size = 0;
			_filename = filename;
			_start_time = start_time;
			_last_ev_time = 0;
			// write a tentative header to pad file out so writing starts at the right offset
			write_header();
		}
	}

	return (_fd == 0) ? -1 : 0;
}

#if 0
jack_nframes_t
SMFWriter::write_unlocked (MidiRingBuffer& src, jack_nframes_t cnt)
{
	//cerr << "SMF WRITE -- " << _length << "--" << cnt << endl;
	
	MidiBuffer buf(1024); // FIXME: allocation, size?
	src.read(buf, /*_length*/0, _length + cnt); // FIXME?

	fseek(_fd, 0, SEEK_END);

	// FIXME: start of source time?
	
	for (size_t i=0; i < buf.size(); ++i) {
		const MidiEvent& ev = buf[i];
		assert(ev.time >= _timeline_position);
		uint32_t delta_time = (ev.time - _timeline_position) - _last_ev_time;
		
		/*printf("SMF - writing event, delta = %u, size = %zu, data = ",
			delta_time, ev.size);
		for (size_t i=0; i < ev.size; ++i) {
			printf("%X ", ev.buffer[i]);
		}
		printf("\n");
		*/
		size_t stamp_size = write_var_len(delta_time);
		fwrite(ev.buffer, 1, ev.size, _fd);
		_last_ev_time += delta_time;
		_track_size += stamp_size + ev.size;
	}

	fflush(_fd);

	if (buf.size() > 0) {
		ViewDataRangeReady (_length, cnt); /* EMIT SIGNAL */
	}

	update_length(_length, cnt);
	return cnt;
}
#endif


/** Write an event at the end of the file.
 *
 * @a time is the absolute time of the event, relative to the start of the file
 *    (the start_time parameter to start).  Must be monotonically increasing on
 *    successive calls to this method.
 */
void
SMFWriter::write_event(Raul::BeatTime       time,
                       size_t               ev_size,
                       const unsigned char* ev) throw (logic_error)
{
	if (time < _start_time)
		throw logic_error("Event time is before file start time");
	else if (time < _last_ev_time)
		throw logic_error("Event time not monotonically increasing");

	time -= _start_time;

	fseek(_fd, 0, SEEK_END);
	
	double delta_time = (time - _last_ev_time) / (double)_ppqn;
	size_t stamp_size = 0;

	/* If delta time is too long (i.e. overflows), write out empty
	 * "proprietary" events to reach the desired time.
	 * Any SMF reading application should interpret this correctly
	 * (by accumulating the delta time and ignoring the event) */
	while (delta_time > VAR_LEN_MAX) {
		static unsigned char null_event[] = { 0xFF, 0x7F, 0x0 };
		stamp_size = write_var_len(VAR_LEN_MAX);
		fwrite(null_event, 1, 3, _fd);
		delta_time -= VAR_LEN_MAX;
	}
	
	assert(delta_time < VAR_LEN_MAX);
	stamp_size = write_var_len((uint32_t)delta_time);
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
SMFWriter::write_header ()
{
	cerr << "SMF Flushing header\n";

	assert(_fd);

	const uint16_t type     = GUINT16_TO_BE(0);    // SMF Type 0 (single track)
	const uint16_t ntracks  = GUINT16_TO_BE(1);    // Number of tracks (always 1 for Type 0)
	const uint16_t division = GUINT16_TO_BE(1920); // FIXME FIXME FIXME PPQN

	char data[6];
	memcpy(data, &type, 2);
	memcpy(data+2, &ntracks, 2);
	memcpy(data+4, &division, 2);

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
SMFWriter::write_chunk_header(char id[4], uint32_t length)
{
	const uint32_t length_be = GUINT32_TO_BE(length);

	fwrite(id, 1, 4, _fd);
	fwrite(&length_be, 4, 1, _fd);
}


void
SMFWriter::write_chunk(char id[4], uint32_t length, void* data)
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


#if 0
uint32_t
SMFWriter::read_var_len() const
{
	assert(!feof(_fd));

	//int offset = ftell(_fd);
	//cerr << "SMF - reading var len at " << offset << endl;

	uint32_t value;
	unsigned char c;

	if ( (value = getc(_fd)) & 0x80 ) {
		value &= 0x7F;
		do {
			assert(!feof(_fd));
			value = (value << 7) + ((c = getc(_fd)) & 0x7F);
		} while (c & 0x80);
	}

	return value;
}
#endif


} // namespace Raul

