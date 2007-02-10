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

//#include "../../config.h"

#include <signal.h>
#include <iostream>
#include <libgnomecanvasmm.h>

#include "machina/Loader.hpp"
#include "machina/JackDriver.hpp"
#include "machina/JackNodeFactory.hpp"

#include "MachinaGUI.hpp"

using namespace Machina;


int
main(int argc, char** argv)
{
	// Build engine
	
	SharedPtr<JackDriver> driver(new JackDriver());

	SharedPtr<Machina::Machine> m(new Machine());

	m->activate();

	driver->set_machine(m);
	driver->attach("machina");



	// Launch GUI
	try {
	
	Gnome::Canvas::init();
	Gtk::Main app(argc, argv);
	
	MachinaGUI gui(m);
	app.run(*gui.window());
	
	} catch (std::string msg) {
		std::cerr << "Caught exception, aborting.  Error message was: " << msg << std::endl;
		return 1;
	}
	
	driver->detach();

	return 0;
}


