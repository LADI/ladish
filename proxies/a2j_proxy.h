/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008,2009,2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to code that interfaces a2jmidid through D-Bus
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

#ifndef A2J_PROXY_HPP__24525CB1_8AED_4697_8C56_5C57473839CC__INCLUDED
#define A2J_PROXY_HPP__24525CB1_8AED_4697_8C56_5C57473839CC__INCLUDED

#include "common.h"

bool a2j_proxy_init(void);
void a2j_proxy_uninit(void);
const char * a2j_proxy_get_jack_client_name_cached(void);
bool a2j_proxy_get_jack_client_name_noncached(char ** client_name_ptr_ptr);

bool
a2j_proxy_map_jack_port(
    const char * jack_port_name,
    char ** alsa_client_name_ptr_ptr,
    char ** alsa_port_name_ptr_ptr,
    uint32_t * alsa_client_id_ptr);

bool a2j_proxy_is_started(void);
bool a2j_proxy_start_bridge(void);
bool a2j_proxy_stop_bridge(void);

bool a2j_proxy_exit(void);

#endif // #ifndef A2J_PROXY_HPP__24525CB1_8AED_4697_8C56_5C57473839CC__INCLUDED
