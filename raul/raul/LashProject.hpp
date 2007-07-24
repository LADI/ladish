/* This file is part of Raul.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Raul is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Raul is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef RAUL_LASH_PROJECT_HPP
#define RAUL_LASH_PROJECT_HPP

#include <string>
#include <sigc++/sigc++.h>
#include <raul/SharedPtr.hpp>
#include <raul/WeakPtr.hpp>
#include <raul/LashClient.hpp>

namespace Raul {


class LashProject : public sigc::trackable {
public:
	LashProject(SharedPtr<LashClient> client, const std::string& name);

	void save();
	void close();

	const std::string& name()      { return _name; }
	const std::string& directory() { return _directory; }

	void set_directory(const std::string& filename);
	void set_name(const std::string& name);
	
	sigc::signal<void, const std::string&> signal_name;
	sigc::signal<void, const std::string&> signal_directory;
	sigc::signal<void, const std::string&> signal_save_file;
	sigc::signal<void, const std::string&> signal_restore_file;

	bool operator==(const std::string& name) { return _name == name; }

private:
	WeakPtr<LashClient> _client;
	std::string         _name;
	std::string         _directory;
};


} // namespace Raul

#endif // RAUL_LASH_PROJECT_HPP

