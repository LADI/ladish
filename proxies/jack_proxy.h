/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009,2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains interface to the helper functionality for accessing
 * JACK through D-Bus
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

#ifndef JACK_PROXY_H__88702EEC_4B82_407F_A664_AD70C1E14D02__INCLUDED
#define JACK_PROXY_H__88702EEC_4B82_407F_A664_AD70C1E14D02__INCLUDED

#include "common.h"

struct jack_parameter_variant
{
  enum
  {
    jack_byte,
    jack_boolean,
    jack_int16,
    jack_uint16,
    jack_int32,
    jack_uint32,
    jack_int64,
    jack_uint64,
    jack_doubl,
    jack_string,
  } type;

  union
  {
    unsigned char byte;
    bool boolean;
    int16_t int16;
    uint16_t uint16;
    int32_t int32;
    uint32_t uint32;
    int64_t int64;
    uint64_t uint64;
    double doubl;
    char *string;
  } value;
};

typedef
void
(* jack_proxy_callback_server_started)(
  void);

typedef
void
(* jack_proxy_callback_server_stopped)(
  void);

typedef
void
(* jack_proxy_callback_server_appeared)(
  void);

typedef
void
(* jack_proxy_callback_server_disappeared)(
  void);

bool
jack_proxy_init(
  jack_proxy_callback_server_started server_started,
  jack_proxy_callback_server_stopped server_stopped,
  jack_proxy_callback_server_appeared server_appeared,
  jack_proxy_callback_server_disappeared server_disappeared);

void
jack_proxy_uninit(
  void);

bool
jack_proxy_is_started(
  bool * started_ptr);

bool jack_proxy_start_server(void);
bool jack_proxy_stop_server(void);

bool jack_proxy_is_realtime(bool * realtime_ptr);
bool jack_proxy_sample_rate(uint32_t * sample_rate_ptr);
bool jack_proxy_get_xruns(uint32_t * xruns_ptr);
bool jack_proxy_get_dsp_load(double * dsp_load_ptr);
bool jack_proxy_get_buffer_size(uint32_t * size_ptr);
bool jack_proxy_set_buffer_size(uint32_t size);
bool jack_proxy_reset_xruns(void);

bool
jack_proxy_connect_ports(
  uint64_t port1_id,
  uint64_t port2_id);

bool
jack_proxy_read_conf_container(
  const char * address,
  void * callback_context,
  bool (* callback)(void * context, bool leaf, const char * address, char * child));

bool
jack_proxy_get_parameter_value(
  const char * address,
  bool * is_set_ptr,
  struct jack_parameter_variant * parameter_ptr);

bool
jack_proxy_set_parameter_value(
  const char * address,
  const struct jack_parameter_variant * parameter_ptr);

bool
jack_proxy_reset_parameter_value(
  const char * address);

bool jack_reset_all_params(void);

#endif /* #ifndef JACK_PROXY_H__88702EEC_4B82_407F_A664_AD70C1E14D02__INCLUDED */
