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
	: _window_location(0, 0)
	, _window_size(640, 480)
	, _zoom(1.0)
{
}


bool
StateManager::get_module_location(const string& name, ModuleType type, Coord& loc) 
{
	map<string, ModuleSettings>::const_iterator i = _module_settings.find(name);
	if (i == _module_settings.end())
		return false;

	const ModuleSettings& settings = (*i).second;
		
	switch (type) {
	case Input:
		if (settings.input_location) {
			loc = *settings.input_location;
			return true;
		}
		break;
	case Output:
		if (settings.output_location) {
			loc = *settings.output_location;
			return true;
		}
		break;
	case InputOutput:
		if (settings.inout_location) {
			loc = *settings.inout_location;
			return true;
		}
		break;
	default:
		throw std::runtime_error("Invalid module type");
	}

	return false;
}


void
StateManager::set_module_location(const string& name, ModuleType type, Coord loc) 
{
retry:
	map<string, ModuleSettings>::iterator i = _module_settings.find(name);
	if (i == _module_settings.end()) {
		// no mapping exists, insert new element and set its split type, then retry to retrieve reference to it
		_module_settings[name].split = type != InputOutput;
		goto retry;
	}

	ModuleSettings& settings_ref = (*i).second;

	switch (type) {
	case Input:
		settings_ref.input_location = loc;
		break;
	case Output:
		settings_ref.output_location = loc;
		break;
	case InputOutput:
		settings_ref.inout_location = loc;
		break;
	default:
		throw std::runtime_error("Invalid module type");
	}
}


/** Returns whether or not this module should be split.
 *
 * If nothing is known about the given module, @a default_val is returned (this is
 * to allow driver's to request terminal ports get split by default).
 */
bool
StateManager::get_module_split(const string& name, bool default_val) const
{
	map<string, ModuleSettings>::const_iterator i = _module_settings.find(name);
	if (i == _module_settings.end())
		return default_val;

	return (*i).second.split;
}


void
StateManager::set_module_split(const string& name, bool split) 
{
	_module_settings[name].split = split;
}


void
StateManager::load(const string& filename) 
{	
	_module_settings.clear();

	cerr << "Loading configuration file " << filename << endl;
	
	std::ifstream is;
	is.open(filename.c_str(), std::ios::in);

	if ( ! is.good()) {
		std::cerr << "Unable to load file " << filename << "!" << endl;
		return;
	}

	string s;

	is >> s;
	if (s == "window_location") {
		is >> s;
		_window_location.x = atoi(s.c_str());
		is >> s;
		_window_location.y = atoi(s.c_str());
	}
	
	is >> s;
	if (s == "window_size") {
		is >> s;
		_window_size.x = atoi(s.c_str());
		is >> s;
		_window_size.y = atoi(s.c_str());
	}

	is >> s;
	if (s != "zoom_level") {
		std::string msg = "Corrupt settings file: expected \"zoom_level\", found \"";
		msg.append(s).append("\"");
		throw std::runtime_error(msg);
	}
	
	is >> s;
	_zoom = atof(s.c_str());

	Coord loc;
	ModuleType type;
	string name;

	while (1) {
		is >> s;
		if (is.eof()) break;
		
		// Old versions didn't quote, so need to support both :/
		if (s[0] == '\"') {
			if (s.length() > 1 && s[s.length()-1] == '\"') {
				name = s.substr(1, s.length()-2);
			} else {
				name = s.substr(1);
				is >> s;
				while (s[s.length()-1] != '\"') {
					name.append(" ").append(s);
					is >> s;
				}
				name.append(" ").append(s.substr(0, s.length()-1));
			}
		} else {
			name = s;
		}

		is >> s;
		if (s == "input") type = Input;
		else if (s == "output") type = Output;
		else if (s == "inputoutput") type = InputOutput;
		else throw std::runtime_error("Corrupt settings file.");

		is >> s;
		loc.x = atoi(s.c_str());
		is >> s;
		loc.y = atoi(s.c_str());

		set_module_location(name, type, loc);
	}

	is.close();
}

static inline void
write_module_settings_entry(
	std::ofstream& os,
	const string& name,
	const char * type,
	const Coord& loc)
{
	os << "\"" << name << "\"" << " " << type << " " << loc.x << " " << loc.y << std::endl;
}

void
StateManager::save(const string& filename) 
{
	std::ofstream os;
	os.open(filename.c_str(), std::ios::out);

	os << "window_location " << _window_location.x << " " << _window_location.y << std::endl;
	os << "window_size " << _window_size.x << " " << _window_size.y << std::endl;
	os << "zoom_level " << _zoom << std::endl;


	for (map<string, ModuleSettings>::iterator i = _module_settings.begin(); i != _module_settings.end(); ++i) {
		const ModuleSettings& settings = (*i).second;
		const string& name = (*i).first;

		if (settings.split) {
			write_module_settings_entry(os, name, "input", *settings.input_location);
			write_module_settings_entry(os, name, "output", *settings.output_location);
		} else {
			write_module_settings_entry(os, name, "inputoutput", *settings.inout_location);
		}
	}

	os.close();
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
	// Darkest tango palette colour, with S -= 6, V -= 6, w/ transparency
	
	if (type == JACK_AUDIO)
		return 0x244678C0;
	else if (type == JACK_MIDI)
		return 0x960909C0;
	else if (type == ALSA_MIDI)
		return 0x4A8A0EC0;
	else
		return 0xFF0000FF;
}

