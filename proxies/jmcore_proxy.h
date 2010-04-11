/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to code that interfaces the jmcore through D-Bus
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

#ifndef JMCORE_PROXY_H__A39B2531_CD34_48B9_8561_323755ED551D__INCLUDED
#define JMCORE_PROXY_H__A39B2531_CD34_48B9_8561_323755ED551D__INCLUDED

#include "common.h"

bool jmcore_proxy_init(void);
void jmcore_proxy_uninit(void);
const int64_t jmcore_proxy_get_pid_cached(void);
bool jmcore_proxy_get_pid_noncached(int64_t * pid_ptr);
bool jmcore_proxy_create_link(bool midi, const char * input_port_name, const char * output_port_name);
bool jmcore_proxy_destroy_link(const char * port_name);

#endif /* #ifndef JMCORE_PROXY_H__A39B2531_CD34_48B9_8561_323755ED551D__INCLUDED */
