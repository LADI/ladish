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

class Patchage;
class Driver;


/** A Driver event to be processed by the GUI thread.
 */
class PatchageEvent {
public:
	enum Type {
		NULL_EVENT = 0,
		REFRESH,
		PORT_CREATION,
		PORT_DESTRUCTION,
		CONNECTION,
		DISCONNECTION
	};

	PatchageEvent(Driver* d = NULL, Type type=NULL_EVENT)
		: _driver(d)
		, _type(type)
	{}

	template <typename P>
	PatchageEvent(Driver* driver, Type type, P port)
		: _driver(driver)
		, _port_1(port)
		, _type(type)
	{}
	
	template <typename P>
	PatchageEvent(Driver* driver, Type type, P port_1, P port_2)
		: _driver(driver)
		, _port_1(port_1, false)
		, _port_2(port_2, true)
		, _type(type)
	{}
	
	void execute(Patchage* patchage);

	inline Type type() const { return (Type)_type; }
	
	struct PortRef {
		PortRef() : type(NULL_PORT_REF) { memset(&id, 0, sizeof(id)); }

		PortRef(jack_port_id_t jack_id, bool ign=false)
			: type(JACK_ID) { id.jack_id = jack_id; }

		PortRef(jack_port_t* jack_port, bool ign=false)
			: type(JACK_PORT) { id.jack_port = jack_port; }

#ifdef HAVE_ALSA
		PortRef(snd_seq_addr_t addr, bool in)
			: type(ALSA_ADDR) { id.alsa_addr = addr; is_input = in; }
		
		bool is_input;
#endif

		enum { NULL_PORT_REF, JACK_ID, JACK_PORT, ALSA_ADDR } type;

		union {
			jack_port_t*   jack_port;
			jack_port_id_t jack_id;
#ifdef HAVE_ALSA
			snd_seq_addr_t alsa_addr;
#endif
		} id;
	};

private:
	Driver* _driver;
	PortRef _port_1;
	PortRef _port_2;
	uint8_t _type;
};


#endif // PATCHAGEEVENT_H

