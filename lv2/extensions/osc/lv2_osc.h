/* LV2 OSC Messages Extension
 * Copyright (C) 2007 Dave Robillard <dave@drobilla.net>
 * 
 * This header is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This header is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef LV2_OSC_H
#define LV2_OSC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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
	int32_t       c; /**< Standard C, 8 bit char, as a 32-bit int. */
	int32_t       i; /**< 32 bit signed integer. */
	float         f; /**< 32 bit IEEE-754 float. */
	int64_t       h; /**< 64 bit signed integer. */
	double        d; /**< 64 bit IEEE-754 double. */
	char          s; /**< Standard C, NULL terminated string. */
	char          S; /**< Standard C, NULL terminated symbol. */
	unsigned char b; /**< Blob (int32 size then size bytes padded to 32 bits) */
} LV2Argument;



/** Message.
 *
 * This is an OSC message at heart, but with some additional cache information
 * to allow fast access to parameters.
 */
typedef struct {
	double    time;           /**< Time stamp of message, in frames */
	uint32_t  data_size;      /**< Total size of data, in bytes */
	uint32_t  argument_count; /**< Number of arguments in data */
	uint32_t  types_offset;   /**< Offset of types string in data */

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

LV2Message* lv2_osc_message_new(double time, const char* path, const char* types, ...);

LV2Message* lv2_osc_message_from_raw(double time,
                                     uint32_t out_buf_size, void* out_buf,
                                     uint32_t raw_msg_size, void* raw_msg);

static inline uint32_t lv2_message_get_size(LV2Message* msg)
	{ return sizeof(double) + (sizeof(uint32_t) * 3) + msg->data_size; }

static inline uint32_t lv2_message_get_osc_message_size(const LV2Message* msg)
	{ return (msg->argument_count * sizeof(char) + 1) + msg->data_size; }

static inline const void* lv2_message_get_osc_message(const LV2Message* msg)
	{ return (const void*)(&msg->data + (sizeof(uint32_t) * msg->argument_count)); }

static inline const char* lv2_message_get_path(const LV2Message* msg)
	{ return (const char*)(&msg->data + (sizeof(uint32_t) * msg->argument_count)); }

static inline const char* lv2_message_get_types(const LV2Message* msg)
	{ return (const char*)(&msg->data + msg->types_offset); }

static inline LV2Argument* lv2_message_get_argument(const LV2Message* msg, uint32_t i)
	{ return (LV2Argument*)(&msg->data + ((uint32_t*)&msg->data)[i]); }


/** OSC Message Buffer.
 *
 * The port buffer for an LV2 port of the type
 * <http://drobilla.net/ns/lv2ext/osc> should be a pointer 
 * to an instance of this struct.
 *
 * This buffer and all contained messages are flat packed, an OSC buffer can
 * be copied using memcpy with the size returned by lv2_osc_buffer_get_size.
 *
 * The index is maintained at the end of the buffer's allocated space.
 * To reuse the unclaimed space, use lv2_osc_buffer_compact.
 */
typedef struct {
	uint32_t  capacity;      /**< Total allocated size of data */
	uint32_t  size;          /**< Used size of data */
	uint32_t  message_count; /**< Number of messages in data */

	/** Take the address of this member to get a pointer to the remaining data.
	 *
	 * Contents:
	 * char     data[data_size]
	 * char     padding[...]
	 * uint32_t message_index[message_count]
	 */
	char data;

} LV2OSCBuffer;

LV2OSCBuffer* lv2_osc_buffer_new(uint32_t capacity);

void lv2_osc_buffer_clear(LV2OSCBuffer* buf);

static inline uint32_t lv2_osc_buffer_get_size(LV2Message* msg)
	{ return sizeof(double) + (sizeof(uint32_t) * 3) + msg->data_size; }

static inline const LV2Message* lv2_osc_buffer_get_message(const LV2OSCBuffer* buf, uint32_t i)
{
	const uint32_t* const index_end = (uint32_t*)(&buf->data + buf->capacity - sizeof(uint32_t));
	return (const LV2Message*)(&buf->data + *(index_end - i));
}

int lv2_osc_buffer_append_message(LV2OSCBuffer* buf, LV2Message* msg);
int lv2_osc_buffer_append(LV2OSCBuffer* buf, double time, const char* path, const char* types, ...);

void lv2_osc_buffer_compact(LV2OSCBuffer* buf);

#ifdef __cplusplus
}
#endif

#endif /* LV2_OSC_H */
