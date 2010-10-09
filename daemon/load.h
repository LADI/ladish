/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains inteface for the load helper functions
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

#ifndef LOAD_H__43B1ECB8_247F_4868_AE95_563DD968D7B0__INCLUDED
#define LOAD_H__43B1ECB8_247F_4868_AE95_563DD968D7B0__INCLUDED

#include <expat.h>
#include "common.h"
#include "client.h"
#include "port.h"
#include "room.h"

#define PARSE_CONTEXT_ROOT                0
#define PARSE_CONTEXT_STUDIO              1
#define PARSE_CONTEXT_JACK                2
#define PARSE_CONTEXT_CONF                3
#define PARSE_CONTEXT_PARAMETER           4
#define PARSE_CONTEXT_CLIENTS             5
#define PARSE_CONTEXT_CLIENT              6
#define PARSE_CONTEXT_PORTS               7
#define PARSE_CONTEXT_PORT                8
#define PARSE_CONTEXT_DICT                9
#define PARSE_CONTEXT_KEY                10
#define PARSE_CONTEXT_CONNECTIONS        11
#define PARSE_CONTEXT_CONNECTION         12
#define PARSE_CONTEXT_APPLICATIONS       13
#define PARSE_CONTEXT_APPLICATION        14
#define PARSE_CONTEXT_ROOMS              15
#define PARSE_CONTEXT_ROOM               16
#define PARSE_CONTEXT_PROJECT            17

#define MAX_STACK_DEPTH       10
#define MAX_DATA_SIZE         10240

struct ladish_parse_context
{
  XML_Bool error;
  unsigned int element[MAX_STACK_DEPTH];
  signed int depth;
  char data[MAX_DATA_SIZE];
  int data_used;
  char * str;
  ladish_client_handle client;
  ladish_port_handle port;
  ladish_dict_handle dict;
  ladish_room_handle room;
  uint64_t connection_id;
  bool terminal;
  bool autorun;
  uint8_t level;
  void * parser;
};

void ladish_dump_element_stack(struct ladish_parse_context * context_ptr);
const char * ladish_get_string_attribute(const char * const * attr, const char * key);
const char * ladish_get_uuid_attribute(const char * const * attr, const char * key, uuid_t uuid, bool optional);
const char * ladish_get_bool_attribute(const char * const * attr, const char * key, bool * bool_value_ptr);
const char * ladish_get_byte_attribute(const char * const * attr, const char * key, uint8_t * byte_value_ptr);

bool
ladish_get_name_and_uuid_attributes(
  const char * element_description,
  const char * const * attr,
  const char ** name_str_ptr,
  const char ** uuid_str_ptr,
  uuid_t uuid);

bool
ladish_parse_port_type_and_direction_attributes(
  const char * element_description,
  const char * const * attr,
  uint32_t * type_ptr,
  uint32_t * flags_ptr);

void ladish_interlink_clients(ladish_graph_handle vgraph, ladish_app_supervisor_handle app_supervisor);

#endif /* #ifndef LOAD_H__43B1ECB8_247F_4868_AE95_563DD968D7B0__INCLUDED */
