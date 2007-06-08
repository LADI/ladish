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

#include <string>
#include <jack/jack.h>
#include "../config.h"
#include "PatchagePort.h"

class Patchage;


/** A Driver event to be processed by the GUI thread.
 */
class PatchageEvent {
public:
	enum Type {
		NULL_EVENT,
		PORT_CREATION,
		PORT_DESTRUCTION,
		CONNECTION,
		DISCONNECTION
	};

	PatchageEvent(Patchage* patchage)
		: _patchage(patchage)
		, _type(NULL_EVENT)
	{}

	PatchageEvent(Patchage* patchage, Type type, jack_port_id_t port)
		: _patchage(patchage)
		, _type(type)
		, _port_1(port)
	{}
	
	PatchageEvent(Patchage* patchage, Type type,
			snd_seq_addr_t port_1, snd_seq_addr_t port_2)
		: _patchage(patchage)
		, _type(type)
		, _port_1(port_1)
		, _port_2(port_2)
	{}

	void execute();

	Type           type()    { return _type; }

private:
	Patchage*      _patchage;
	Type           _type;
	
	struct PortRef {
		PortRef() : type((PortType)0xdeadbeef) { id.jack = 0; }

		PortRef(jack_port_id_t jack_id) : type(JACK_ANY) { id.jack = jack_id; }

#ifdef HAVE_ALSA
		PortRef(snd_seq_addr_t addr) : type(ALSA_MIDI) { id.alsa = addr; }
#endif

		PortType type;
		union {
			jack_port_id_t jack;
#ifdef HAVE_ALSA
			snd_seq_addr_t alsa;
#endif
		} id;
	};

	PortRef _port_1;
	PortRef _port_2;
	
	boost::shared_ptr<PatchagePort> find_port(const PortRef& ref);
};


#endif // PATCHAGEEVENT_H

