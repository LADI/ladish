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

#include <signal.h>
#include <iostream>
#include <string>
#include <libgnomecanvasmm.h>
#include "machina/Loader.hpp"
#include "machina/JackDriver.hpp"
#include "machina/SMFDriver.hpp"
#include "MachinaGUI.hpp"

using namespace std;
using namespace Machina;


int
main(int argc, char** argv)
{
	SharedPtr<Machina::Machine> machine;
	
	// Load machine, if given
	if (argc == 2) {
		string filename = argv[1];
		cout << "Building machine from MIDI file " << filename << endl;
		SharedPtr<Machina::SMFDriver> file_driver(new Machina::SMFDriver());
		machine = file_driver->learn(filename, 0.0);
	}

	// Build engine
	SharedPtr<JackDriver> driver(new JackDriver(machine));
	driver->attach("machina");
	SharedPtr<Engine> engine(new Engine(driver));

	// Launch GUI
	try {

		Gnome::Canvas::init();
		Gtk::Main app(argc, argv);

		driver->activate();
		MachinaGUI gui(engine);

		app.run(*gui.window());

	} catch (string msg) {
		cerr << "Caught exception, aborting.  Error message was: " << msg << endl;
		return 1;
	}
	
	driver->detach();

	return 0;
}


