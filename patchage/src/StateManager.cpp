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

#include <stdexcept>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include "StateManager.hpp"
#include "Patchage.hpp"

using namespace std;


StateManager::StateManager()
{
	_window_location.x = 0;
	_window_location.y = 0;
	_window_size.x = 640;
	_window_size.y = 480;
	_zoom = 1.0;
}


Coord
StateManager::get_module_location(const string& name, ModuleType type) 
{
	for (std::list<ModuleLocation>::iterator i = _module_locations.begin(); i != _module_locations.end(); ++i) {
		if ((*i).name == name && (*i).type == type)
			return (*i).loc;
	}

	// -1 used as a flag value
	Coord r = { -1, -1 };
	return r;
}


void
StateManager::set_module_location(const string& name, ModuleType type, Coord loc) 
{
	for (std::list<ModuleLocation>::iterator i = _module_locations.begin(); i != _module_locations.end(); ++i) {
		if ((*i).name == name && (*i).type == type) {
			(*i).loc = loc;
			return;
		}
	}
	
	// If we get here, module isn't in list yet
	ModuleLocation ml = { name, type, loc };
	_module_locations.push_back(ml);
}


/** Returns whether or not this module should be split.
 *
 * If nothing is known about the given module, @a default_val is returned (this is
 * to allow driver's to request terminal ports get split by default).
 */
bool
StateManager::get_module_split(const string& name, bool default_val) const
{
	map<string, bool>::const_iterator i = _module_splits.find(name);
	if (i == _module_splits.end())
		return default_val;
	else
		return (*i).second;
}


void
StateManager::set_module_split(const string& name, bool split) 
{
	_module_splits[name] = split;
}


void
StateManager::load(const string& filename) 
{	
	_module_locations.clear();

	cerr << "Loading configuration file " << filename << endl;
	
	std::ifstream is;
	is.open(filename.c_str(), std::ios::in);

	if ( ! is.good()) {
		std::cerr << "Unable to load file " << filename << "!" << endl;
		return;
	}

	string s;

	is >> s;
	if (s != "window_location") throw std::runtime_error("Corrupt settings file.");
	is >> s;
	_window_location.x = atoi(s.c_str());
	is >> s;
	_window_location.y = atoi(s.c_str());

	is >> s;
	if (s != "window_size") throw std::runtime_error("Corrupt settings file.");
	is >> s;
	_window_size.x = atoi(s.c_str());
	is >> s;
	_window_size.y = atoi(s.c_str());

	is >> s;
	if (s != "zoom_level") throw std::runtime_error("Corrupt settings file.");
	is >> s;
	_zoom = atof(s.c_str());

	ModuleLocation ml;
	while (1) {
		is >> s;
		if (is.eof()) break;
		
		// Old versions didn't quote, so need to support both :/
		if (s[0] == '\"') {
			if (s.length() > 1 && s[s.length()-1] == '\"') {
				ml.name = s.substr(1, s.length()-2);
			} else {
				ml.name = s.substr(1);
				is >> s;
				while (s[s.length()-1] != '\"') {
					ml.name.append(" ").append(s);
					is >> s;
				}
				ml.name.append(" ").append(s.substr(0, s.length()-1));
			}
		} else {
			ml.name = s;
		}

		is >> s;
		if (s == "input") ml.type = Input;
		else if (s == "output") ml.type = Output;
		else if (s == "inputoutput") ml.type = InputOutput;
		else throw std::runtime_error("Corrupt settings file.");

		is >> s;
		ml.loc.x = atoi(s.c_str());
		is >> s;
		ml.loc.y = atoi(s.c_str());

		_module_locations.push_back(ml);
	}

	is.close();
}


void
StateManager::save(const string& filename) 
{
	std::ofstream os;
	os.open(filename.c_str(), std::ios::out);

	os << "window_location " << _window_location.x << " " << _window_location.y << std::endl;
	os << "window_size " << _window_size.x << " " << _window_size.y << std::endl;
	os << "zoom_level " << _zoom << std::endl;

	ModuleLocation ml;
	for (std::list<ModuleLocation>::iterator i = _module_locations.begin(); i != _module_locations.end(); ++i) {
		ml = *i;
		os << "\"" << ml.name << "\"";
		
		if (ml.type == Input) os << " input ";
		else if (ml.type == Output) os << " output ";
		else if (ml.type == InputOutput) os << " inputoutput ";
		else throw std::runtime_error("Invalid module type");

		os << ml.loc.x << " " << ml.loc.y << std::endl;
	}

	os.close();
}


Coord
StateManager::get_window_location() 
{
	return _window_location;
}


void
StateManager::set_window_location(Coord loc) 
{
	_window_location = loc;
}


Coord
StateManager::get_window_size() 
{
	return _window_size;
}


void
StateManager::set_window_size(Coord size) 
{
	_window_size = size;
}


float
StateManager::get_zoom() 
{
	return _zoom;
}


void
StateManager::set_zoom(float zoom) 
{
	_zoom = zoom;
}


int
StateManager::get_port_color(PortType type)
{
	if (type == JACK_AUDIO)
		return 0x305171B0;
	else if (type == JACK_MIDI)
		return 0x663939B0;
	else if (type == ALSA_MIDI)
		return 0x307130B0;
	else
		return 0xFF0000B0;
}	
