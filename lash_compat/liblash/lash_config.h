/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
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

#ifndef __LIBLASH_CONFIG_H__
#define __LIBLASH_CONFIG_H__

#include "../config.h"

#include <stdbool.h>
#include <dbus/dbus.h>

#include "lash/types.h"

struct _lash_config_handle
{
	DBusMessageIter *iter;
	bool             is_read;
};


#ifdef LASH_OLD_API
# include <sys/types.h>
# include "common/klist.h"

struct _lash_config
{
	struct list_head  siblings;
	char             *key;
	void             *value;
	size_t            value_size;
	int               value_type;
};

#endif /* LASH_OLD_API */

#endif /* __LIBLASH_CONFIG_H__ */
