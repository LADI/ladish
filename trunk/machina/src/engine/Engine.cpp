/* This file is part of Machina.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Machina is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Machina is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <glibmm/ustring.h>
#include "machina/Engine.hpp"
#include "machina/JackDriver.hpp"
#include "machina/Loader.hpp"
#include "machina/Machine.hpp"
#include "machina/SMFDriver.hpp"

namespace Machina {


/** Load the machine at @a uri, and run it (replacing current machine).
 * Safe to call while engine is processing.
 */
SharedPtr<Machine>
Engine::load_machine(const Glib::ustring& uri)
{
	SharedPtr<Machine> old_machine = _driver->machine(); // Hold a reference to current machine..

	SharedPtr<Machine> m = _loader.load(uri);
	if (m) {
		m->activate();
		_driver->set_machine(m);
	}
	
	// .. and drop it in this thread (to prevent deallocation in the RT thread)
	
	return m;
}


/** Load the machine at @a uri, and insert it into the current machine..
 * Safe to call while engine is processing.
 */
SharedPtr<Machine>
Engine::import_machine(const Glib::ustring& uri)
{
	SharedPtr<Machine> m = _loader.load(uri);
	if (m) {
		m->activate();
		_driver->machine()->nodes().append(m->nodes());
	}
	
	// Discard m
	
	return _driver->machine();
}


/** Learn the SMF (MIDI) file at @a uri, add it to the current machine.
 * Safe to call while engine is processing.
 */
SharedPtr<Machine>
Engine::import_midi(const Glib::ustring& uri, Raul::TimeStamp q, Raul::TimeDuration duration)
{
	SharedPtr<SMFDriver> file_driver(new SMFDriver());
	SharedPtr<Machine> m = file_driver->learn(uri, q, duration);
	m->activate();
	_driver->machine()->nodes().append(m->nodes());
	
	// Discard m
	
	return _driver->machine();
}


void
Engine::set_bpm(double bpm)
{
	_driver->set_bpm(bpm);
}


void
Engine::set_quantization(double q)
{
	_driver->set_quantization(q);
}


} // namespace Machina


