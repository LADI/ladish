/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009,2010 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains stuff that is needed almost everywhere in the ladishd
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

#ifndef COMMON_H__CFDC869A_31AE_4FA3_B2D3_DACA8488CA55__INCLUDED
#define COMMON_H__CFDC869A_31AE_4FA3_B2D3_DACA8488CA55__INCLUDED

#include "../common.h"

#include <errno.h>
#include <uuid/uuid.h>

#include "../dbus/helpers.h"

/* ~/BASE_DIR is where studio, room and project xml files are stored */
#define BASE_DIR "/." BASE_NAME
extern char * g_base_dir;

#define LADISH_CHECK_LOG_TEXT "Please inspect the ladishd log (~/.log/ladish/ladish.log) for more info"

/* connection between two ports */
/* virtual connection is connection where at least one the ports is virtual */
/* superconnection is connection that implements virtual connection chain at JACK level */
struct connection
{
  struct list_head siblings_studio_all;    /* link for the studio::all_connections list */
  struct list_head siblings_jack;          /* link for the studio::jack_connections list, not valid for virtual connections */
  struct list_head siblings_capture_port;  /* link for the port::output_connections list, not valid for superconnections */
  struct list_head siblings_playback_port; /* link for the port::input_connections list, not valid for superconnections */

  struct connection * superconnection;     /* Superconnection. NULL for non-virtual connections */
  uint64_t jack_id;                        /* JACK connection ID, not valid for virtual connections */

  struct port * capture_port_ptr;          /* The capture output port */
  struct port * playback_port_ptr;         /* The playback input port */
};

extern bool g_quit;

void ladish_check_integrity(void);

#endif /* #ifndef COMMON_H__CFDC869A_31AE_4FA3_B2D3_DACA8488CA55__INCLUDED */
