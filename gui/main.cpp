/*
 * This file is part of LADI session handler.
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
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

#include "config.h"

#include <iostream>
#include <libgnomecanvasmm.h>
#include <glibmm/exception.h>

#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <libglademm/xml.h>
#include "config.h"
#include "binary_location.h"

#include "Patchage.hpp"
#include "globals.hpp"

Glib::RefPtr<Gnome::Glade::Xml> g_xml;

static
Glib::RefPtr<Gnome::Glade::Xml>
glade_open()
{
  // Check for the .glade file in ./src/
  std::string glade_filename = std::string("./gui/gui.glade");
  std::ifstream fs(glade_filename.c_str());
  if (fs.fail())
  { // didn't find it, check DATA_PATH
    fs.clear();

    glade_filename = DATA_DIR "/gui.glade";

    fs.open(glade_filename.c_str());
    if (fs.fail())
    {
      std::ostringstream ss;
      ss << "Unable to find gui.glade file" << std::endl;
      throw std::runtime_error(ss.str());
    }
    fs.close();
  }

  return Gnome::Glade::Xml::create(glade_filename);
}

int main(int argc, char** argv)
{
	try {
	
	Gnome::Canvas::init();
	Gtk::Main app(argc, argv);

	g_xml = glade_open();
	
	Patchage patchage(argc, argv);
	app.run(*patchage.window());
	
	} catch (std::exception& e) {
		std::cerr << "Caught exception, aborting.  Error message was: "
				<< e.what() << std::endl;
		return 1;
	
	} catch (Glib::Exception& e) {
		std::cerr << "Caught exception, aborting.  Error message was: "
				<< e.what() << std::endl;
		return 1;
	}

	return 0;
}


