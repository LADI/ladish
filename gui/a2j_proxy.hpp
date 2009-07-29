/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
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

struct a2j_proxy_impl;

#define A2J_STATUS_NO_RESPONSE     0
#define A2J_STATUS_BRIDGE_STOPPED  1
#define A2J_STATUS_BRIDGE_STARTED  2

class a2j_proxy
{
public:
  a2j_proxy();
  ~a2j_proxy();

  const char *
  get_jack_client_name();

  bool
  map_jack_port(
    const char * jack_port_name,
    string& alsa_client_name,
    string& alsa_port_name,
    uint32_t& alsa_client_id);

  void
  start_bridge();

  void
  stop_bridge();

private:
  a2j_proxy_impl * _impl_ptr;
};

#endif // #ifndef A2J_PROXY_HPP__24525CB1_8AED_4697_8C56_5C57473839CC__INCLUDED
