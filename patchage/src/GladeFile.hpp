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

#ifndef GLADEFILE_HPP
#define GLADEFILE_HPP

#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <libglademm/xml.h>

class GladeFile {
public:
	static Glib::RefPtr<Gnome::Glade::Xml> open(const std::string& base_name) {
		Glib::RefPtr<Gnome::Glade::Xml> xml;

		// Check for the .glade file in current directory
		std::string glade_filename = std::string("./").append(base_name).append(".glade");
		std::ifstream fs(glade_filename.c_str());
		if (fs.fail()) { // didn't find it, check DATA_PATH
			fs.clear();
			glade_filename = std::string(DATA_DIR).append("/").append(base_name).append(".glade");

			fs.open(glade_filename.c_str());
			if (fs.fail()) {
				std::ostringstream ss;
				ss << "Unable to find " << base_name << "glade in current directory or "
					<< DATA_DIR << std::endl;
				throw std::runtime_error(ss.str());
			}
			fs.close();
		}

		return Gnome::Glade::Xml::create(glade_filename);
	}
};

#endif // GLADEFILE_HPP
