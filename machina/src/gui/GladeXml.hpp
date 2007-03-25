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

#include <fstream>
#include <string>
#include <gtkmm.h>
#include <libglademm/xml.h>

class GladeXml
{
public:
	static Glib::RefPtr<Gnome::Glade::Xml> create() {
		Glib::RefPtr<Gnome::Glade::Xml> xml;

		// Check for the .glade file in current directory
		std::string glade_filename = "./machina.glade";
		std::ifstream fs(glade_filename.c_str());
		if (fs.fail()) { // didn't find it, check PKGDATADIR
			fs.clear();
			glade_filename = PKGDATADIR;
			glade_filename += "/machina.glade";

			fs.open(glade_filename.c_str());
			if (fs.fail()) {
				std::cerr << "Unable to find machina.glade in current directory or "
					<< PKGDATADIR << "." << std::endl;
				exit(EXIT_FAILURE);
			}
			fs.close();
		}

		try {
			xml = Gnome::Glade::Xml::create(glade_filename);
		} catch(const Gnome::Glade::XmlError& ex) {
			std::cerr << ex.what() << std::endl;
			throw ex;
		}

		return xml;
	}
};

