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

#ifndef REDLANDMM_MODEL_HPP
#define REDLANDMM_MODEL_HPP

#include <stdexcept>
#include <string>
#include <librdf.h>
#include <glibmm/ustring.h>
#include <boost/utility.hpp>
#include <redlandmm/Namespaces.hpp>
#include <redlandmm/Node.hpp>

namespace Redland {

class World;


/** An RDF Model (collection of triples).
 */
class Model : public boost::noncopyable, public Wrapper<librdf_model> {
public:
	Model(World& world);
	Model(World& world, const Glib::ustring& uri, Glib::ustring base_uri="");
	Model(World& world, const char* str, size_t len, Glib::ustring base_uri="");
	~Model();

	void        set_base_uri(const Glib::ustring& uri);
	const Node& base_uri() const { return _base; }
	
	void  serialise_to_file_handle(FILE* fd);
	void  serialise_to_file(const Glib::ustring& uri);
	char* serialise_to_string();
	
	void add_statement(const Node& subject,
	                   const Node& predicate,
	                   const Node& object);   
	
	void add_statement(const Node&        subject,
	                   const std::string& predicate,
	                   const Node&        object);   
	
	World& world() const { return _world; }

private:
	friend class Query;

	void setup_prefixes();

	World&             _world;
	Node               _base;
	librdf_storage*    _storage;
	librdf_serializer* _serialiser;

	size_t _next_blank_id;
};


} // namespace Redland

#endif // REDLANDMM_MODEL_HPP
