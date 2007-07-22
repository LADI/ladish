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

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "lv2_osc.h"

#define lv2_osc_swap32(x) \
({ \
    uint32_t __x = (x); \
    ((uint32_t)( \
    (((uint32_t)(__x) & (uint32_t)0x000000ffUL) << 24) | \
    (((uint32_t)(__x) & (uint32_t)0x0000ff00UL) <<  8) | \
    (((uint32_t)(__x) & (uint32_t)0x00ff0000UL) >>  8) | \
    (((uint32_t)(__x) & (uint32_t)0xff000000UL) >> 24) )); \
})

#define lv2_osc_swap64(x) \
({ \
    uint64_t __x = (x); \
    ((uint64_t)( \
	(uint64_t)(((uint64_t)(__x) & (uint64_t)0x00000000000000ffULL) << 56) | \
	(uint64_t)(((uint64_t)(__x) & (uint64_t)0x000000000000ff00ULL) << 40) | \
	(uint64_t)(((uint64_t)(__x) & (uint64_t)0x0000000000ff0000ULL) << 24) | \
	(uint64_t)(((uint64_t)(__x) & (uint64_t)0x00000000ff000000ULL) <<  8) | \
	(uint64_t)(((uint64_t)(__x) & (uint64_t)0x000000ff00000000ULL) >>  8) | \
	(uint64_t)(((uint64_t)(__x) & (uint64_t)0x0000ff0000000000ULL) >> 24) | \
	(uint64_t)(((uint64_t)(__x) & (uint64_t)0x00ff000000000000ULL) >> 40) | \
	(uint64_t)(((uint64_t)(__x) & (uint64_t)0xff00000000000000ULL) >> 56) )); \
})


/** Pad a size to a multiple of 32 bits */
inline static uint32_t
lv2_osc_pad_size(uint32_t size)
{
	return size + 3 - ((size-1) % 4);
}


inline static uint32_t
lv2_osc_string_size(const char *s)
{
    return lv2_osc_pad_size(strlen(s) + 1);
}


static inline uint32_t
lv2_osc_blob_size(const void* blob)
{
    return sizeof(uint32_t) + lv2_osc_pad_size(*((uint32_t*)blob));
}


uint32_t
lv2_osc_arg_size(char type, const LV2Argument* arg)
{
    switch (type) {
	case 'c':
	case 'i':
	case 'f':
		return 4;
	
	case 'h':
	case 'd':
		return 8;

	case 's':
		return lv2_osc_string_size(&arg->s);
	
	case 'S':
		return lv2_osc_string_size(&arg->S);

	case 'b':
		return lv2_osc_blob_size(&arg->b);

	default:
		fprintf(stderr, "Warning: unknown OSC type '%c'.", type);
		return 0;
    }
}


void
lv2_osc_argument_swap_byte_order(char type, LV2Argument* arg)
{
    switch (type) {
	case 'i':
	case 'f':
	case 'b':
	case 'c':
		*(int32_t*)arg = lv2_osc_swap32(*(int32_t*)arg);
		break;

	case 'h':
	case 'd':
		*(int64_t*)arg = lv2_osc_swap64(*(int64_t*)arg);
		break;
	}
}


/** Convert a message from network byte order to host byte order. */
void
lv2_osc_message_swap_byte_order(LV2Message* msg)
{
	const char* const types = lv2_message_get_types(msg);

	for (uint32_t i=0; i < msg->argument_count; ++i)
		lv2_osc_argument_swap_byte_order(types[i], lv2_message_get_argument(msg, i));
}


void
lv2_osc_argument_print(char type, const LV2Argument* arg)
{
	int32_t blob_size;

    switch (type) {
	case 'c':
		printf("%c", arg->c); break;
	case 'i':
		printf("%d", arg->i); break;
	case 'f':
		printf("%f", arg->f); break;
	case 'h':
		printf("%ld", arg->h); break;
	case 'd':
		printf("%f", arg->d); break;
	case 's':
		printf("%s", &arg->s); break;
	case 'S':
		printf("%s", &arg->S); break;
	case 'b':
		blob_size = *((int32_t*)arg);
		printf("{ ");
		for (int32_t i=0; i < blob_size; ++i)
			printf("%X, ", (&arg->b)[i+4]);
		printf(" }");
		break;
	default:
		printf("?");
	}
}

void
lv2_osc_message_print(LV2Message* msg)
{
	const char* const types = lv2_message_get_types(msg);

	printf("%s (%s) ", lv2_message_get_path(msg), types);
	for (uint32_t i=0; i < msg->argument_count; ++i) {
		lv2_osc_argument_print(types[i], lv2_message_get_argument(msg, i));
		printf(" ");
	}
	printf("\n");
}


/** Allocate a new LV2OSCBuffer.
 *
 * This function is NOT realtime safe.
 */
LV2OSCBuffer*
lv2_osc_buffer_new(uint32_t capacity)
{
	LV2OSCBuffer* buf = (LV2OSCBuffer*)malloc((sizeof(uint32_t) * 3) + capacity);
	buf->capacity = capacity;
	buf->size = 0;
	buf->message_count = 0;
	memset(&buf->data, 0, capacity);
	return buf;
}


int
lv2_osc_buffer_append_raw_message(LV2OSCBuffer* buf, double time, uint32_t size, void* raw_msg)
{
	const uint32_t message_header_size = sizeof(double) + (sizeof(uint32_t) * 4);

	if (buf->capacity - buf->size < message_header_size + size)
		return ENOBUFS;

	LV2Message* write_loc = (LV2Message*)(&buf->data + buf->size);
	write_loc->time = time;
	write_loc->data_size = size;
	
	const uint32_t path_size = lv2_osc_string_size((char*)raw_msg);
	const uint32_t types_len = strlen((char*)(raw_msg + path_size + 1));
	write_loc->argument_count = types_len;
	
	uint32_t index_size = write_loc->argument_count * sizeof(uint32_t);
	
	// Copy raw message
	memcpy(&write_loc->data + index_size, raw_msg, size);

	write_loc->types_index = index_size + path_size + 1;
	const char* const types = lv2_message_get_types(write_loc);
	
	// Calculate/Write index
	uint32_t args_base_offset = write_loc->types_index + lv2_osc_string_size(types) - 1;
	uint32_t arg_offset = 0;
	
	for (uint32_t i=0; i < write_loc->argument_count; ++i) {
		((uint32_t*)&write_loc->data)[i] = args_base_offset + arg_offset;
		const LV2Argument* const arg = (LV2Argument*)(&write_loc->data + args_base_offset + arg_offset);
		if (types[i] == 'b') // special case because size is still big-endian
			arg_offset += GINT32_FROM_BE(*((int32_t*)arg));
		else
			arg_offset += lv2_osc_arg_size(types[i], arg);
	}
	
	/*printf("Index:\n");
	for (uint32_t i=0; i < write_loc->argument_count; ++i) {
		printf("%u ", ((uint32_t*)&write_loc->data)[i]);
	}
	printf("\n");
	
	printf("Data:\n");
	for (uint32_t i=0; i < (write_loc->argument_count * 4) + size; ++i) {
		printf("%3u", i % 10);
	}
	printf("\n");
	for (uint32_t i=0; i < (write_loc->argument_count * 4) + size; ++i) {
		char c = *(((char*)&write_loc->data) + i);
		if (c >= 32 && c <= 126)
			printf("%3c", c);
		else
			printf("%3d", (int)c);
	}
	printf("\n");*/

	// Swap to host byte order if necessary (must do now so blob size is read correctly)
	if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
		lv2_osc_message_swap_byte_order(write_loc);

	printf("Appended message:\n");
	lv2_osc_message_print(write_loc);

	buf->capacity -= message_header_size + size;

	return 0;
}

