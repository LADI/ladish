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
#include <cassert>
#include <iostream>
#include <glibmm/miscutils.h>
#include "raul/SMFReader.h"
#include "raul/midi_events.h"

using namespace std;

namespace Raul {


/** Return the size of the given event NOT including the status byte,
 * or -1 if unknown (eg sysex)
 */
static int
midi_event_size(unsigned char status)
{
	if (status >= 0x80 && status <= 0xE0) {
		status &= 0xF0; // mask off the channel
	}

	switch (status) {
		case MIDI_CMD_NOTE_OFF:
		case MIDI_CMD_NOTE_ON:
		case MIDI_CMD_NOTE_PRESSURE:
		case MIDI_CMD_CONTROL:
		case MIDI_CMD_BENDER:
		case MIDI_CMD_COMMON_SONG_POS:
			return 2;

		case MIDI_CMD_PGM_CHANGE:
		case MIDI_CMD_CHANNEL_PRESSURE:
		case MIDI_CMD_COMMON_MTC_QUARTER:
		case MIDI_CMD_COMMON_SONG_SELECT:
			return 1;

		case MIDI_CMD_COMMON_TUNE_REQUEST:
		case MIDI_CMD_COMMON_SYSEX_END:
		case MIDI_CMD_COMMON_CLOCK:
		case MIDI_CMD_COMMON_START:
		case MIDI_CMD_COMMON_CONTINUE:
		case MIDI_CMD_COMMON_STOP:
		case MIDI_CMD_COMMON_SENSING:
		case MIDI_CMD_COMMON_RESET:
			return 0;

		case MIDI_CMD_COMMON_SYSEX:
			return -1;
	}

	return -1;
}


SMFReader::SMFReader()
	: _fd(NULL)
	, _ppqn(0)
	, _last_ev_time(0)
	, _track_size(0)
{
}


SMFReader::~SMFReader()
{
	if (_fd)
		close();
}


bool
SMFReader::open(const string&  filename)
{
	if (_fd)
		throw logic_error("Attempt to start new read while write in progress.");

	cerr << "Opening SMF file " << filename << " for reading." << endl;

	_fd = fopen(filename.c_str(), "r+");

	if (_fd) {
		// Read PPQN
		// FIXME: broken for time-based divisions
		fseek(_fd, 12, SEEK_SET);
		uint16_t ppqn_be = 0;
		fread(&ppqn_be, 2, 1, _fd);
		_ppqn = GUINT16_FROM_BE(ppqn_be);

		// Read Track size
		fseek(_fd, 18, SEEK_SET);
		uint32_t track_size_be = 0;
		fread(&track_size_be, 4, 1, _fd);
		_track_size = GUINT32_FROM_BE(track_size_be);
		std::cerr << "SMF - read track size " << _track_size << std::endl;

		_last_ev_time = 0;
		return true;
	} else {
		return false;
	}
}

	
size_t
SMFReader::num_tracks() throw (logic_error)
{
	if (!_fd)
		throw logic_error("num_tracks called without an open SMF file.");

	return 1;
}


/** Read an event from the current position in file.
 *
 * File position MUST be at the beginning of a delta time, or this will die very messily.
 * ev.buffer must be of size ev.size, and large enough for the event.  The returned event
 * will have it's time field set to it's delta time (so it's the caller's responsibility
 * to calculate a real time for the event).
 *
 * Returns event length (including status byte) on success, 0 if event was
 * skipped (eg a meta event), or -1 on EOF (or end of track).
 *
 * If @a buf is not large enough to hold the event, 0 will be returned, but ev_size
 * set to the actual size of the event.
 */
int
SMFReader::read_event(size_t buf_len, char* buf, size_t* ev_size, uint64_t* ev_time)
{
	// - 4 is for the EOT event, which we don't actually want to read
	//if (feof(_fd) || ftell(_fd) >= HEADER_SIZE + _track_size - 4) {
	if (feof(_fd)) {
		return -1;
	}

	assert(buf_len > 0);
	assert(buf);
	assert(ev_size);
	assert(ev_time);

	uint32_t delta_time = read_var_len();
	int status = fgetc(_fd);
	assert(status != EOF); // FIXME die gracefully
	if (status == 0xFF) {
		assert(!feof(_fd));
		int type = fgetc(_fd);
		if ((unsigned char)type == 0x2F) {
			//cerr << "SMF - hit EOT" << endl;
			return -1; // we hit the logical EOF anyway...
		} else {
			ev_size = 0;
			*ev_time = _last_ev_time + delta_time; // this is needed regardless
			_last_ev_time = *ev_time;
			return 0;
		}
	}

	buf[0] = (unsigned char)status;
	*ev_size = midi_event_size(buf[0]) + 1;
	if (*ev_size > buf_len)
		return 0;
	fread(buf+1, 1, *ev_size - 1, _fd);
	*ev_time = _last_ev_time + delta_time;
	_last_ev_time = *ev_time;

	/*printf("SMF - read event, delta = %u, size = %zu, data = ",
		delta_time, ev.size);
	for (size_t i=0; i < ev.size; ++i) {
		printf("%X ", ev.buffer[i]);
	}
	printf("\n");*/
	
	return *ev_size;
}


void
SMFReader::close()
{
	if (_fd)
		fclose(_fd);

	_fd = NULL;
}


uint32_t
SMFReader::read_var_len() const
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


} // namespace Raul

