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

#ifndef MACHINA_ENGINE_HPP
#define MACHINA_ENGINE_HPP

#include <raul/SharedPtr.h>

namespace Machina {

class Machine;
class JackDriver;


class Engine {
public:
	Engine(SharedPtr<JackDriver> driver, SharedPtr<Machine> machine)
		: _driver(driver)
		, _machine(machine)
	{}
	
	SharedPtr<JackDriver> driver()  { return _driver; }
	SharedPtr<Machine>    machine() { return _machine; }

	void set_bpm(double bpm);
	
	void set_quantization(double beat_fraction);

private:
	SharedPtr<JackDriver> _driver;
	SharedPtr<Machine>    _machine;
};


} // namespace Machina

#endif // MACHINA_ENGINE_HPP
