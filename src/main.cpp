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

#include CONFIG_H_PATH

#include <iostream>
#include <libgnomecanvasmm.h>
#include <glibmm/exception.h>

#include "Patchage.hpp"
#include "GladeFile.hpp"
#include "globals.hpp"

Glib::RefPtr<Gnome::Glade::Xml> g_xml;

int main(int argc, char** argv)
{
	try {
	
	Gnome::Canvas::init();
	Gtk::Main app(argc, argv);

	g_xml = GladeFile::open("patchage");
	
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


