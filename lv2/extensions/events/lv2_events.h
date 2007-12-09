/* lv2_events.h - header file for the LV2 events extension.
 *
 * This header defines the code portion of the LV2 extension with URI
 * <http://drobilla.net/ns/lv2ext/events> (preferred prefix 'lv2ev').
 * 
 * Copyright (C) 2006-2007 Lars Luthman <lars.luthman@gmail.com>
 * Copyright (C) 2007 Dave Robillard <dave@drobilla.net>
 * 
 * This header is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This header is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this header; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307 USA
 */

#include <stdint.h>

#ifndef LV2_EVENTS_H
#define LV2_EVENTS_H

/** A buffer of LV2 events.
 *
 * This is a flat buffer, the header and all contained events are POD:
 * the entire buffer (12 + size bytes) can be copied with a single memcpy
 * (using sizeof(LV2_EventBuffer) anywhere is almost certainly an error).
 */
typedef struct {

	/** The number of events in the data buffer. 
	 * INPUT PORTS: The host must set this field to the number of events
	 * contained in the data buffer before calling run()
	 * OUTPUT PORTS: The plugin must set this field to the number of events it
	 * has written to the buffer before returning from the run().  Any initial
	 * value should be ignored by the plugin.
	 */
	uint32_t event_count;

	/** The size of the data buffer in bytes.
	 * This is set by the host and must not be changed by the plugin.
	 * The host is allowed to change this between run() calls.
	 */
	uint32_t capacity;

	/** The size of the initial portion of the data buffer containing data.
	 *  INPUT PORTS: It's the host's responsibility to set this field to the
	 *  number of bytes used by all MIDI events it has written to the buffer
	 *  (including timestamps and size fields) before calling the plugin's 
	 *  run() function. The plugin may not change this field.
	 *  OUTPUT PORTS: It's the plugin's responsibility to set this field to
	 *  the number of bytes used by all MIDI events it has written to the
	 *  buffer (including timestamps and size fields) before returning from
	 *  the run() function. Any initial value should be ignored by the plugin.
	 */
	uint32_t size;

	/** The data buffer where events are packed sequentially (size bytes).
	 * Take the address of this member (&buf.data) for a pointer to the first
	 * event contained in this buffer.
	 *
	 * Events are a generic container in which seveal types of specific events
	 * can be transported.  New event types can be created and used within
	 * this event framework, specified by the type field of the event header.
	 *
	 * This data buffer contains a header (defined by struct LV2_Event),
	 * followed by that event's contents (padding to 32 bits), followed by
	 * another header, etc:
	 *
	 * |       |       |       |       |       |       |
	 * | | | | | | | | | | | | | | | | | | | | | | | | |
	 * |FRAMES |SUBFRMS|TYP|LEN|DATA........PAD|FRAMES | ...
	 */
	uint8_t data;

} LV2_EventBuffer;


/** An LV2 event.
 *
 * Like LV2_EventBuffer above (which this is a portion of), this is a single
 * continuous chunk of memory: the entire event (12 + size bytes) can be
 * copied with a single memcpy
 * (using sizeof(LV2_EventHeader) anywhere is almost certainly an error).
 */
typedef struct {

	/** The frames portion of timestamp.  The units used here can optionally be
	 * set for a port (with the lv2ev:timeUnits property), otherwise this
	 * is audio frames, corresponding to the sample_count parameter of the
	 * LV2 run method (e.g. frame 0 is the first frame for that call to run).
	 */
	uint32_t frames;

	/** The sub-frames portion of timestamp.  The units used here can
	 * optionally be set for a port (with the lv2ev:timeUnits property),
	 * otherwise this is 1/(2^32) of an audio frame.
	 */
	uint32_t subframes;
	
	/** The type of this event, where this numeric value refers to some URI
	 * via the symbol table LV2 extension
	 * <http://drobilla.net/ns/lv2ext/symbols#>
	 */
	uint16_t type;

	/** The size of the data portion of this event in bytes, which immediately
	 * follows.  The header size (12 bytes) is not included in this value.
	 */
	uint16_t size;

	/** The data portion of this event (size bytes).
	 * Take the address of this member (&ev.data) for a pointer to the start
	 * of the events contained in this buffer.
	 */
	uint8_t data;

} LV2_Event;


#endif
