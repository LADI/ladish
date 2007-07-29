/* This file is part of Patchage.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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

#ifndef PATCHAGEEVENT_H
#define PATCHAGEEVENT_H

#include <jack/jack.h>
#include CONFIG_H_PATH
#ifdef HAVE_ALSA
#include <alsa/asoundlib.h>
#endif
#include "PatchagePort.hpp"

#include <iostream>
using namespace std;

class Patchage;


/** A Driver event to be processed by the GUI thread.
 */
class PatchageEvent {
public:
	enum Type {
		NULL_EVENT = 0,
		PORT_CREATION,
		PORT_DESTRUCTION,
		CONNECTION,
		DISCONNECTION
	};

	PatchageEvent()
		: _type(NULL_EVENT)
	{}

	PatchageEvent(Type type, jack_port_id_t port)
		: _type(type)
		, _port_1(port)
	{}
	
	PatchageEvent(Type type, jack_port_id_t port_1, jack_port_id_t port_2)
		: _type(type)
		, _port_1(port_1)
		, _port_2(port_2)
	{}
	
#ifdef HAVE_ALSA
	PatchageEvent(Type type, snd_seq_addr_t port_1, snd_seq_addr_t port_2)
		: _type(type)
		, _port_1(port_1, false)
		, _port_2(port_2, true)
	{}
#endif

	void execute(Patchage* patchage);

	inline Type type() const { return (Type)_type; }

private:
	uint8_t _type;
	
	struct PortRef {
		PortRef() : type(NULL_PORT_REF) { memset(&id, 0, sizeof(id)); }

		PortRef(jack_port_id_t jack_id) : type(JACK_ID) { id.jack_id = jack_id; }
		PortRef(jack_port_t* jack_port) : type(JACK_PORT) { id.jack_port = jack_port; }

#ifdef HAVE_ALSA
		PortRef(snd_seq_addr_t addr, bool input) : type(ALSA_ADDR)
			{ id.alsa_addr = addr; is_input = input; }
#endif

		enum { NULL_PORT_REF, JACK_ID, JACK_PORT, ALSA_ADDR } type;

		union {
			jack_port_t* jack_port;
			jack_port_id_t jack_id;
#ifdef HAVE_ALSA
			snd_seq_addr_t alsa_addr;
#endif
		} id;

#ifdef HAVE_ALSA
		bool is_input;
#endif

	};

	PortRef _port_1;
	PortRef _port_2;
	
	boost::shared_ptr<PatchagePort> find_port(const Patchage* patchage, const PortRef& ref);
};


#endif // PATCHAGEEVENT_H

