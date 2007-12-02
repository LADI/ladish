/* This file is part of redlandmm.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * redlandmm is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * redlandmm is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef REDLANDMM_WORLD_HPP
#define REDLANDMM_WORLD_HPP

#include <stdexcept>
#include <string>
#include <librdf.h>
#include <boost/utility.hpp>
#include <glibmm/thread.h>
#include <redlandmm/Wrapper.hpp>
#include <redlandmm/Namespaces.hpp>
#include <redlandmm/Node.hpp>

namespace Redland {


/** Library state
 */
class World : public boost::noncopyable, public Wrapper<librdf_world> {
public:
	World();
	~World();
	
	Node blank_id(const std::string base_name="");

	void        add_prefix(const std::string& prefix, const std::string& uri);
	std::string expand_uri(const std::string& uri) const;
	std::string qualify(const std::string& uri) const;

	const Namespaces& prefixes() const { return _prefixes; }

	librdf_world* world() { return _c_obj; }

	Glib::Mutex& mutex() { return _mutex; }

private:
	void setup_prefixes();

	Glib::Mutex   _mutex;
	Namespaces    _prefixes;

	size_t _next_blank_id;
};


} // namespace Redland

#endif // REDLANDMM_WORLD_HPP
