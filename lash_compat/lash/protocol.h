/*
 *   LASH
 *
 *   Copyright (C) 2002 Robert Ham <rah@bash.sh>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __LASH_PROTOCOL_H__
#define __LASH_PROTOCOL_H__

#include <lash/types.h>

#define LASH_PROTOCOL_MAJOR 2
#define LASH_PROTOCOL_MINOR 0

/*
 * Protocol versions (release: protocol):
 *
 * 0.4: 2.0
 * 0.3: 1.1
 * 0.2: 1.0
 * 0.1: 0.0 (implied by the 0ing of the lash_event_t.event_data)
 */

#define LASH_PROTOCOL_MAJOR_MASK 0xFFFF0000
#define LASH_PROTOCOL_MINOR_MASK 0x0000FFFF

#define LASH_PROTOCOL(major, minor) \
  ((lash_protocol_t)( ((major << 16) & LASH_PROTOCOL_MAJOR_MASK) | \
                     (minor & LASH_PROTOCOL_MINOR_MASK)             ))

#define LASH_PROTOCOL_VERSION (LASH_PROTOCOL (LASH_PROTOCOL_MAJOR, LASH_PROTOCOL_MINOR))

#define LASH_PROTOCOL_GET_MAJOR(protocol)  ((lash_protocol_t) (((protocol & LASH_PROTOCOL_MAJOR_MASK) >> 16)))
#define LASH_PROTOCOL_GET_MINOR(protocol)  ((lash_protocol_t) (protocol & LASH_PROTOCOL_MINOR_MASK))

#define LASH_PROTOCOL_IS_VALID(proto) \
  ((LASH_PROTOCOL_GET_MAJOR (proto) == LASH_PROTOCOL_MAJOR) && \
   (LASH_PROTOCOL_GET_MINOR (proto) <= LASH_PROTOCOL_MINOR))

const char * lash_protocol_string (lash_protocol_t protocol);

#endif /* __LASH_PROTOCOL_H__ */
