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

#ifndef RAUL_RDF_MODEL_HPP
#define RAUL_RDF_MODEL_HPP

#include <stdexcept>
#include <string>
#include <librdf.h>
#include <glibmm/ustring.h>
#include <boost/utility.hpp>
#include <raul/Namespaces.hpp>
#include <raul/Atom.hpp>
#include <raul/RDFNode.hpp>


namespace Raul {
namespace RDF {

class World;


class Model : public boost::noncopyable {
public:
	Model(World& world);
	Model(World& world, const Glib::ustring& uri, Glib::ustring base_uri="");
	~Model();

	void        set_base_uri(const Glib::ustring& uri);
	const Node& base_uri() const { return _base; }
	
	void        serialise_to_file_handle(FILE* fd);
	void        serialise_to_file(const Glib::ustring& uri);
	std::string serialise_to_string();
	
	void add_statement(const Node& subject,
	                   const Node& predicate,
	                   const Node& object);   
	
	void add_statement(const Node&        subject,
	                   const std::string& predicate,
	                   const Node&        object);   
	
	void add_statement(const Node& subject,
	                   const Node& predicate,
	                   const Atom& object);
	
	void add_statement(const Node&        subject,
	                   const std::string& predicate,
	                   const Atom&        object);

	World& world() const { return _world; }

private:
	friend class Query;

	void setup_prefixes();

	World&             _world;
	Node               _base;
	librdf_storage*    _storage;
	librdf_model*      _model;
	librdf_serializer* _serialiser;

	size_t _next_blank_id;
};


} // namespace RDF
} // namespace Raul

#endif // RAUL_RDF_MODEL_HPP
