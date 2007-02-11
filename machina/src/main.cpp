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

#include <iostream>
#include <signal.h>
#include "machina/Machine.hpp"
#include "machina/Node.hpp"
#include "machina/Action.hpp"
#include "machina/Edge.hpp"
#include "machina/Loader.hpp"
#include "machina/JackDriver.hpp"
#include "machina/JackNodeFactory.hpp"
#include "machina/MidiAction.hpp"

using namespace std;
using namespace Machina;


bool quit = false;


void
catch_int(int)
{
	signal(SIGINT, catch_int);
	signal(SIGTERM, catch_int);

	std::cout << "Interrupted" << std::endl;

	quit = true;
}


int
main(int argc, char** argv)
{
	if (argc != 2) {
		cout << "Usage: " << argv[0] << " FILE" << endl;
		return -1;
	}
	
	SharedPtr<JackDriver>  driver(new JackDriver());
	SharedPtr<NodeFactory> factory(new JackNodeFactory(driver));

	MidiAction::set_driver(driver);

	Loader l(factory);

	SharedPtr<Machine> m = l.load(argv[1]);

	m->activate();

	driver->set_machine(m);
	driver->attach("machina");

	signal(SIGINT, catch_int);
	signal(SIGTERM, catch_int);

	while (!quit)
		sleep(1);
	
	driver->detach();

	return 0;
}

