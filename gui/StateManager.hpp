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

#ifndef PATCHAGE_STATEMANAGER_HPP
#define PATCHAGE_STATEMANAGER_HPP

#include <string>
#include <list>
#include <map>
#include <iostream>
#include <boost/optional.hpp>
#include "PatchagePort.hpp"

enum ModuleType { Input, Output, InputOutput };

struct Coord {
  Coord(double x_=0, double y_=0) : x(x_), y(y_) {}
  double x;
  double y;
};

class StateManager
{
public:
  StateManager();

  void load(const std::string& filename);
  void save(const std::string& filename);

  bool get_module_location(const std::string& name, ModuleType type, Coord& loc);
  void  set_module_location(const std::string& name, ModuleType type, Coord loc);

  void set_module_split(const std::string& name, bool split);
  bool get_module_split(const std::string& name, bool default_val) const;
  
  float get_zoom();
  void  set_zoom(float zoom);
  
  int get_port_color(PortType type);

  Coord get_window_location()          { return _window_location; }
  void  set_window_location(Coord loc) { _window_location = loc; }
  Coord get_window_size()              { return _window_size; }
  void  set_window_size(Coord size)    { _window_size = size; }

private:
  struct ModuleSettings {
    ModuleSettings() : split(false) {}
    bool split;
    boost::optional<Coord> input_location;
    boost::optional<Coord> output_location;
    boost::optional<Coord> inout_location;
  };

  std::map<std::string,ModuleSettings> _module_settings;
  Coord _window_location;
  Coord _window_size;
  float _zoom;
};


#endif // PATCHAGE_STATEMANAGER_HPP
