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

#include <sstream>
#include <raul/RDFWorld.hpp>
#include <raul/RDFNode.hpp>
#include <raul/AtomRedland.hpp>

#define U(x) ((const unsigned char*)(x))

using namespace std;

namespace Raul {
namespace RDF {


//static const char* const RDF_LANG = "rdfxml-abbrev";
static const char* const RDF_LANG = "turtle";


/** Create an empty in-memory RDF model.
 */
World::World()
{
	_world = librdf_new_world();
	assert(_world);
	librdf_world_open(_world);
	
	add_prefix("rdf", "http://www.w3.org/1999/02/22-rdf-syntax-ns#");
}


World::~World()
{
	Glib::Mutex::Lock lock(_mutex);
	librdf_free_world(_world);
}


void
World::add_prefix(const string& prefix, const string& uri)
{
	_prefixes[prefix] = uri;
}


/** Expands the prefix of URI, if the prefix is registered.
 */
string
World::expand_uri(const string& uri) const
{
	if (uri.find(":") == string::npos)
		return uri;

	for (Namespaces::const_iterator i = _prefixes.begin(); i != _prefixes.end(); ++i)
		if (uri.substr(0, i->first.length()+1) == i->first + ":")
			return i->second + uri.substr(i->first.length()+1);

	return uri;
}


/** Opposite of expand_uri
 */
string
World::qualify(const string& uri) const
{
	return _prefixes.qualify(uri);
}


Node
World::blank_id(const string base_name)
{
	std::ostringstream ss;
	ss << "b" << _next_blank_id++ << "_";
	
	if (base_name != "")
		ss << base_name;

	Node result = Node(*this, Node::BLANK, ss.str());
	assert(result.to_string() == ss.str());
	return result;
}


} // namespace RDF
} // namespace Raul

