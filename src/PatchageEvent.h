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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef PATCHAGEEVENT_H
#define PATCHAGEEVENT_H

#include <string>
#include <jack/jack.h>

class Patchage;


/** A Driver event to be processed by the GUI thread.
 */
class PatchageEvent {
public:
	enum Type {
		NULL_EVENT,
		PORT_CREATION,
		PORT_DESTRUCTION
	};

	PatchageEvent(Patchage* patchage)
		: _patchage(patchage)
		, _type(NULL_EVENT)
	{}

	PatchageEvent(Patchage* patchage, Type type, jack_port_id_t port_id)
		: _patchage(patchage)
		, _type(type)
		, _port_id(port_id)
	{}

	void execute();

	Type           type()    { return _type; }
	jack_port_id_t port_id() { return _port_id; }

private:
	Patchage*      _patchage;
	Type           _type;
	jack_port_id_t _port_id;
};


#endif // PATCHAGEEVENT_H

