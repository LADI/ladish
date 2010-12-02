/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009, 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the prototypes of the escape helper functions
 **************************************************************************
 *
 * LADI Session Handler is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * LADI Session Handler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LADI Session Handler. If not, see <http://www.gnu.org/licenses/>
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef ESCAPE_H__FABBE484_1093_41C1_9FC9_62ED0106E542__INCLUDED
#define ESCAPE_H__FABBE484_1093_41C1_9FC9_62ED0106E542__INCLUDED

#include "common.h"
#include <limits.h>             /* UINT_MAX */

#define LADISH_ESCAPE_FLAG_XML_ATTR ((unsigned int)1 << 0)
#define LADISH_ESCAPE_FLAG_OTHER    ((unsigned int)1 << 1)
#define LADISH_ESCAPE_FLAG_ALL      UINT_MAX

void escape(const char ** src_ptr, char ** dst_ptr, unsigned int flags);
void escape_simple(const char * src_ptr, char * dst_ptr, unsigned int flags);
size_t unescape(const char * src, size_t src_len, char * dst);
void unescape_simple(char * buffer);
char * unescape_dup(const char * src);

/* encode each char in three bytes (percent encoding) */
#define max_escaped_length(unescaped_length) ((unescaped_length) * 3)

#endif /* #ifndef ESCAPE_H__FABBE484_1093_41C1_9FC9_62ED0106E542__INCLUDED */
