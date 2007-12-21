/* lv2_events.h - C header file for the LV2 events extension.
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


/** An LV2 event.
 *
 * LV2 events are generic time-stamped containers for any type of event.
 * The type field defines the format of a given event's contents.
 *
 * This struct defined the header of an LV2 event.  An LV2 event is a single
 * chunk of POD (plain old data), usually contained in a flat buffer
 * (see LV2_EventBuffer below).  Unless a required feature says otherwise,
 * hosts may assume a deep copy of an LV2 event can be created safely
 * using a simple:
 *
 * memcpy(ev_copy, ev, sizeof(LV2_Event) + ev->size);  (or equivalent)
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

	/* size bytes of data follow here */

} LV2_Event;



/** A buffer of LV2 events.
 *
 * Like events (which this contains) an event buffer is a single chunk of POD:
 * the entire buffer (including contents) can be copied with a single memcpy.
 * The first contained event begins sizeof(LV2_EventBuffer) bytes after
 * the start of this struct.
 *
 * After this header, the buffer contains an event header (defined by struct
 * LV2_Event), followed by that event's contents (padded to 64 bits), followed by
 * another header, etc:
 *
 * |       |       |       |       |       |       |
 * | | | | | | | | | | | | | | | | | | | | | | | | |
 * |FRAMES |SUBFRMS|TYP|LEN|DATA..DATA..PAD|FRAMES | ...
 */
typedef struct {

	/** The number of events in this buffer.
	 * INPUT PORTS: The host must set this field to the number of events
	 *     contained in the data buffer before calling run().
	 *     The plugin must not change this field.
	 * OUTPUT PORTS: The plugin must set this field to the number of events it
	 *     has written to the buffer before returning from run().
	 *     Any initial value should be ignored by the plugin.
	 */
	uint32_t event_count;

	/** The size of the data buffer in bytes.
	 * This is set by the host and must not be changed by the plugin.
	 * The host is allowed to change this between run() calls.
	 */
	uint32_t capacity;

	/** The size of the initial portion of the data buffer containing data.
	 *  INPUT PORTS: The host must set this field to the number of bytes used
	 *      by all events it has written to the buffer (including headers)
	 *      before calling the plugin's run().
	 *      The plugin must not change this field.
	 *  OUTPUT PORTS: The plugin must set this field to the number of bytes
	 *      used by all events it has written to the buffer (including headers)
	 *      before returning from run().
	 *      Any initial value should be ignored by the plugin.
	 */
	uint32_t size;

} LV2_EventBuffer;


#endif // LV2_EVENTS_H

