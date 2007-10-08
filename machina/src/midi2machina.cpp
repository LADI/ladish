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

#include <iostream>
#include <signal.h>
#include <raul/RDFModel.hpp>
#include "machina/Engine.hpp"
#include "machina/Machine.hpp"
#include "machina/Node.hpp"
#include "machina/Action.hpp"
#include "machina/Edge.hpp"
#include "machina/SMFDriver.hpp"
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
	if (argc != 3) {
		cout << "Usage: midi2machina QUANTIZATION FILE" << endl;
		cout << "Specify quantization in beats, e.g. 1.0, or 0 for none" << endl;
		return -1;
	}

	SharedPtr<SMFDriver> driver(new SMFDriver());

	SharedPtr<Machine> machine = driver->learn(argv[2], strtof(argv[1], NULL));

	if (!machine) {
		cout << "Failed to load MIDI file." << endl;
		return -1;
	}
	
	/*
	string out_filename = argv[1];
	if (out_filename.find_last_of("/") != string::npos)
		out_filename = out_filename.substr(out_filename.find_last_of("/")+1);

	const string::size_type last_dot = out_filename.find_last_of(".");
	if (last_dot != string::npos && last_dot != 0)
		out_filename = out_filename.substr(0, last_dot);
	out_filename += ".machina";

	cout << "Writing output to " << out_filename << endl;
	*/

	Raul::RDF::World world;
	Raul::RDF::Model model(world);
	machine->write_state(model);
	model.serialise_to_file_handle(stdout);
	
	return 0;
}

