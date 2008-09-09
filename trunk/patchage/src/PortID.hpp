/* This file is part of Patchage.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
 * 
 * Patchage is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Patchage is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef PATCHAGE_PORTID_HPP
#define PATCHAGE_PORTID_HPP

#include CONFIG_H_PATH

#include <cstring>
#ifdef HAVE_JACK
#include <jack/jack.h>
#endif
#ifdef HAVE_ALSA
#include <alsa/asoundlib.h>
#endif

#include "PatchagePort.hpp"

struct PortID {
	PortID() : type(NULL_PORT_ID) { memset(&id, 0, sizeof(id)); }
	
	enum { NULL_PORT_ID, JACK_ID, ALSA_ADDR } type;

#ifdef HAVE_JACK
	PortID(jack_port_id_t jack_id, bool ign=false)
		: type(JACK_ID) { id.jack_id = jack_id; }
#endif

#ifdef HAVE_ALSA
	PortID(snd_seq_addr_t addr, bool in)
		: type(ALSA_ADDR) { id.alsa_addr = addr; is_input = in; }

	bool is_input;
#endif

	union {
#ifdef HAVE_JACK
		jack_port_id_t jack_id;
#endif
#ifdef HAVE_ALSA
		snd_seq_addr_t alsa_addr;
#endif
	} id;
};

#endif // PATCHAGE_PORTID_HPP

