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

#include "config.h"

#ifdef LASH_OLD_API

# include <stdio.h>
# include <inttypes.h>

# include "lash/protocol.h"

const char *
lash_protocol_string(lash_protocol_t protocol)
{
	static char str[32];

	sprintf(str, "%" PRIu32 ".%" PRIu32,
	        LASH_PROTOCOL_GET_MAJOR(protocol),
	        LASH_PROTOCOL_GET_MINOR(protocol));

	return str;
}

#endif /* LASH_OLD_API */

/* EOF */
