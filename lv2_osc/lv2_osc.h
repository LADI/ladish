/* LV2 OSC Messages Extension
 * Copyright (C) 2007 Dave Robillard
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>

/** @file
 * This is an LV2 message port specification, which uses (raw) OSC messages
 * and a buffer format which contains a sequence of timestamped messages.
 * Additional (ie beyond raw OSC) indexing information is stored in the buffer
 * for performance, so that accessors for messages and arguments are very fast:
 * O(1) and realtime safe, unless otherwise noted.
 */


/** Argument (in a message).
 *
 * The name of the element in this union directly corresponds to the OSC
 * type tag character in LV2Message::types.
 */
typedef union {
	int32_t       c; ///< Standard C, 8 bit char, as a 32-bit int.
	int32_t       i; ///< 32 bit signed integer.
	float         f; ///< 32 bit IEEE-754 float.
	int64_t       h; ///< 64 bit signed integer.
	double        d; ///< 64 bit IEEE-754 double.
	char          s; ///< Standard C, NULL terminated string.
	char          S; ///< Standard C, NULL terminated symbol.
	unsigned char b; ///< Blob (int32 size, then size bytes padded to 32 bits)
} LV2Argument;



/** Message.
 *
 * This is an OSC message at heart, but with some additional cache information
 * to allow fast access to parameters.
 */
typedef struct {
	double    time;           ///< Time stamp of message, in frames
	uint32_t  data_size;      ///< Total size of data, in bytes
	uint32_t  argument_count; ///< Number of arguments in data
	uint32_t  types_index;    ///< Index to start of types in data

	/** Take the address of this member to get a pointer to the remaining data.
	 * 
	 * Contents:
	 * uint32_t argument_index[argument_count]
	 * char     path[path_length]     (padded OSC string)
	 * char     types[argument_count] (padded OSC string)
	 * void     data[data_size]
	 */
	char data;

} LV2Message;

inline static uint32_t lv2_message_get_size(LV2Message* msg)
	{ return sizeof(double) + (sizeof(uint32_t) * 3) + msg->data_size; }

inline static uint32_t lv2_message_get_osc_message_size(LV2Message* msg)
	{ return (msg->argument_count * sizeof(char) + 1) + msg->data_size; }

inline static const void* lv2_message_get_osc_message(LV2Message* msg)
	{ return (const void*)(&msg->data + (sizeof(uint32_t) * msg->argument_count)); }

inline static const char* lv2_message_get_path(LV2Message* msg)
	{ return (const char*)(&msg->data + (sizeof(uint32_t) * msg->argument_count)); }

inline static const char* lv2_message_get_types(LV2Message* msg)
	{ return (const char*)(&msg->data + msg->types_index); }

inline static LV2Argument* lv2_message_get_argument(LV2Message* msg, uint32_t i)
	{ return (LV2Argument*)(&msg->data + ((uint32_t*)&msg->data)[i]); }


/** OSC Message Buffer.
 *
 * The port buffer for an LV2 port of the type
 * <http://drobilla.net/ns/lv2ext/osc> should be a pointer 
 * to an instance of this struct.
 *
 * This buffer and all contained messages are flat packed, an OSC buffer can
 * be copied using memcpy with the size returned by lv2_osc_buffer_get_size.
 */
typedef struct {
	uint32_t  capacity;      ///< Total allocated size of data
	uint32_t  size;          ///< Used size of data
	uint32_t  message_count; ///< Number of messages in data

	/** Take the address of this member to get a pointer to the remaining data.
	 *
	 * Contents:
	 * uint32_t message_index[message_count]
	 * void     data[data_size]
	 */
	char data;

} LV2OSCBuffer;

LV2OSCBuffer* lv2_osc_buffer_new(uint32_t capacity);

inline static uint32_t lv2_osc_buffer_get_size(LV2Message* msg)
	{ return sizeof(double) + (sizeof(uint32_t) * 3) + msg->data_size; }

inline static const LV2Message* lv2_osc_buffer_get_message(LV2OSCBuffer* buf, uint32_t i)
	{ return (const LV2Message*)(&buf->data + ((uint32_t*)&buf->data)[i]); }

int lv2_osc_buffer_append_raw_message(LV2OSCBuffer* buf, double time, uint32_t size, void* msg);

