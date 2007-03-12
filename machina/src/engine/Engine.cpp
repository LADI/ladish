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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <glibmm/ustring.h>
#include "machina/Loader.hpp"
#include "machina/Engine.hpp"
#include "machina/JackDriver.hpp"
#include "machina/SMFDriver.hpp"

namespace Machina {


/** Load the machine at @a uri, and run it (replacing current machine).
 * Safe to call while engine is processing.
 */
SharedPtr<Machine>
Engine::load_machine(const Glib::ustring& uri)
{
	Loader l; // FIXME: namespaces?
	SharedPtr<Machine> m = l.load(uri);
	m->activate();
	_driver->set_machine(m);
	return m;
}


/** Learn the SMF (MIDI) file at @a uri, and run the resulting machine
 * (replacing current machine).
 * Safe to call while engine is processing.
 */
SharedPtr<Machine>
Engine::learn_midi(const Glib::ustring& uri)
{
	SharedPtr<SMFDriver> file_driver(new SMFDriver());
	SharedPtr<Machine> m = file_driver->learn(uri, 32.0);
	m->activate();
	_driver->set_machine(m);
	return m;
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


