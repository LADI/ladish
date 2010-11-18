/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains inteface for the save helper functions
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

#ifndef SAVE_H__120D6D3D_90A9_4998_8F00_23FCB8BA8DE9__INCLUDED
#define SAVE_H__120D6D3D_90A9_4998_8F00_23FCB8BA8DE9__INCLUDED

#include "common.h"
#include "dict.h"
#include "graph.h"
#include "app_supervisor.h"
#include "room.h"

#define LADISH_XML_BASE_INDENT "  "

struct ladish_write_context
{
  int fd;
  int indent;
};

bool ladish_write_string(int fd, const char * string);
bool ladish_write_indented_string(int fd, int indent, const char * string);
bool ladish_write_string_escape(int fd, const char * string);
bool ladish_write_dict(int fd, int indent, ladish_dict_handle dict);
bool ladish_write_vgraph(int fd, int indent, ladish_graph_handle vgraph, ladish_app_supervisor_handle app_supervisor);
bool ladish_write_room_link_ports(int fd, int indent, ladish_room_handle room);
bool ladish_write_jgraph(int fd, int indent, ladish_graph_handle vgraph);

#endif /* #ifndef SAVE_H__120D6D3D_90A9_4998_8F00_23FCB8BA8DE9__INCLUDED */
