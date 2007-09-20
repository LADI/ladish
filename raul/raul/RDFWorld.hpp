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

#ifndef RAUL_RDF_WORLD_HPP
#define RAUL_RDF_WORLD_HPP

#include <stdexcept>
#include <string>
#include <librdf.h>
#include <boost/utility.hpp>
#include <glibmm/thread.h>
#include <raul/Namespaces.hpp>
#include <raul/RDFNode.hpp>

namespace Raul {
namespace RDF {


class World : public boost::noncopyable {
public:
	World();
	~World();
	
	Node blank_id();

	void        add_prefix(const std::string& prefix, const std::string& uri);
	std::string expand_uri(const std::string& uri) const;
	std::string qualify(const std::string& uri) const;

	const Namespaces& prefixes() const { return _prefixes; }

	librdf_world* world() { return _world; }

	Glib::Mutex& mutex() { return _mutex; }

private:
	void setup_prefixes();

	librdf_world* _world;
	Glib::Mutex   _mutex;
	Namespaces    _prefixes;

	size_t _next_blank_id;
};


} // namespace RDF
} // namespace Raul

#endif // RAUL_RDF_WORLD_HPP
