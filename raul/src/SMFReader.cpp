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
#include <raul/SMFReader.hpp>
#include <raul/midi_events.h>

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
	, _track_size(0)
{
}


SMFReader::~SMFReader()
{
	if (_fd)
		close();
}


bool
SMFReader::open(const string& filename)
{
	if (_fd)
		throw logic_error("Attempt to start new read while write in progress.");

	cerr << "Opening SMF file " << filename << " for reading." << endl;

	_fd = fopen(filename.c_str(), "r+");

	if (_fd) {
		// Read type (bytes 8..9)
		fseek(_fd, 0, SEEK_SET);
		char mthd[5];
		mthd[4] = '\0';
		fread(mthd, 1, 4, _fd);
		if (strcmp(mthd, "MThd")) {
			cerr << "File is not an SMF file, aborting." << endl;
			fclose(_fd);
			_fd = NULL;
			return false;
		}

		
		// Read type (bytes 8..9)
		fseek(_fd, 8, SEEK_SET);
		uint16_t type_be = 0;
		fread(&type_be, 2, 1, _fd);
		_type = GUINT16_FROM_BE(type_be);

		// Read number of tracks (bytes 10..11)
		uint16_t num_tracks_be = 0;
		fread(&num_tracks_be, 2, 1, _fd);
		_num_tracks = GUINT16_FROM_BE(num_tracks_be);

		// Read PPQN (bytes 12..13)
		// FIXME: broken for time-based divisions
		uint16_t ppqn_be = 0;
		fread(&ppqn_be, 2, 1, _fd);
		_ppqn = GUINT16_FROM_BE(ppqn_be);

		seek_to_track(1);
		// Read Track size (skip bytes 14..17 'Mtrk')
		// FIXME: first track read only
		/*fseek(_fd, 18, SEEK_SET);
		uint32_t track_size_be = 0;
		fread(&track_size_be, 4, 1, _fd);
		_track_size = GUINT32_FROM_BE(track_size_be);
		std::cerr << "SMF - read track size " << _track_size << std::endl;
		*/
		
		return true;
	} else {
		return false;
	}
}

	
/** Seek to the start of a given track, starting from 1.
 * Returns true if specified track was found.
 */
bool
SMFReader::seek_to_track(unsigned track)
{
	assert(track > 0);

	if (!_fd)
		throw logic_error("Attempt to seek to track on unopen SMF file.");

	unsigned track_pos = 0;

	fseek(_fd, 14, SEEK_SET);
	char     id[5];
	id[4] = '\0';
	uint32_t chunk_size = 0;

	while (!feof(_fd)) {
		fread(id, 1, 4, _fd);

		if (!strcmp(id, "MTrk")) {
			++track_pos;
			//std::cerr << "Found track " << track_pos << endl;
		} else {
			std::cerr << "Unknown chunk ID " << id << endl;
		}

		uint32_t chunk_size_be;
		fread(&chunk_size_be, 4, 1, _fd);
		chunk_size = GUINT32_FROM_BE(chunk_size_be);

		if (track_pos == track)
			break;

		fseek(_fd, chunk_size, SEEK_CUR);
	}

	if (!feof(_fd) && track_pos == track) {
		//_track = track;
		_track_size = chunk_size;
		return true;
	} else {
		return false;
	}
}

	
/** Read an event from the current position in file.
 *
 * File position MUST be at the beginning of a delta time, or this will die very messily.
 * ev.buffer must be of size ev.size, and large enough for the event.  The returned event
 * will have it's time field set to it's delta time (so it's the caller's responsibility
 * to keep track of delta time, even for ignored events).
 *
 * Returns event length (including status byte) on success, 0 if event was
 * skipped (eg a meta event), or -1 on EOF (or end of track).
 *
 * If @a buf is not large enough to hold the event, 0 will be returned, but ev_size
 * set to the actual size of the event.
 */
int
SMFReader::read_event(size_t buf_len, unsigned char* buf, uint32_t* ev_size, uint32_t* delta_time)
{
	// - 4 is for the EOT event, which we don't actually want to read
	//if (feof(_fd) || ftell(_fd) >= HEADER_SIZE + _track_size - 4) {
	if (feof(_fd)) {
		return -1;
	}

	assert(buf_len > 0);
	assert(buf);
	assert(ev_size);
	assert(delta_time);

	//cerr.flags(ios::hex);
	//cerr << "SMF - Reading event at offset 0x" << ftell(_fd) << endl;
	//cerr.flags(ios::dec);

	// Running status state
	static unsigned char last_status = 0;
	static uint32_t      last_size   = 1234567;

	*delta_time = read_var_len();
	int status = fgetc(_fd);
	assert(status != EOF); // FIXME die gracefully
	assert(status <= 0xFF);

	if (status < 0x80) {
		status = last_status;
		*ev_size = last_size;
		fseek(_fd, -1, SEEK_CUR);
		//cerr << "RUNNING STATUS, size = " << *ev_size << endl;
	} else {
		last_status = status;
		*ev_size = midi_event_size(status) + 1;
		last_size = *ev_size;
		//cerr << "NORMAL STATUS, size = " << *ev_size << endl;
	}

	buf[0] = (unsigned char)status;

	if (status == 0xFF) {
		*ev_size = 0;
		assert(!feof(_fd));
		unsigned char type = fgetc(_fd);
		const uint32_t size = read_var_len();
		/*cerr.flags(ios::hex);
		cerr << "SMF - meta 0x" << (int)type << ", size = ";
		cerr.flags(ios::dec);
		cerr << size << endl;*/

		if ((unsigned char)type == 0x2F) {
			//cerr << "SMF - hit EOT" << endl;
			return -1; // we hit the logical EOF anyway...
		} else {
			fseek(_fd, size, SEEK_CUR);
			return 0;
		}
	}

	if (*ev_size > buf_len) {
		//cerr << "Skipping event" << endl;
		// Skip event, return 0
		fseek(_fd, *ev_size - 1, SEEK_CUR);
		return 0;
	} else {
		// Read event, return size
		fread(buf+1, 1, *ev_size - 1, _fd);
	
		if ((buf[0] & 0xF0) == 0x90 && buf[2] == 0) {
			buf[0] = (0x80 | (buf[0] & 0x0F));
			buf[2] = 0x40;
		}
		
		return *ev_size;
	}
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

