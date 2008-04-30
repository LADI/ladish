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

#ifndef RAUL_ATOM_RDF_HPP
#define RAUL_ATOM_RDF_HPP

#include <cstring>
#include <string>
#include <sstream>

#include CONFIG_H_PATH

#include <redlandmm/Node.hpp>
#include <redlandmm/World.hpp>

#define CUC(x) ((const unsigned char*)(x))

namespace Raul {

/** Conversion between Raul Atoms and Redlandmm RDF nodes.
 * This code (in header raul/AtomRDF.hpp) depends on redlandmm, only apps
 * which directly depend on both raul and liblo should include it.
 */
namespace AtomRDF {

/** Convert a Redland::Node to a Raul::Atom */
inline Atom
node_to_atom(const Redland::Node& node)
{
	if (node.type() == Redland::Node::RESOURCE)
		return Atom(node.to_c_string());
	else if (node.is_float())
		return Atom(node.to_float());
	else if (node.is_int())
		return Atom(node.to_int());
	else if (node.is_bool()) 
		return Atom(node.to_bool());
	else
		return Atom(node.to_c_string());
}


/** Convert a Raul::Atom to a Redland::Node
 * Note that not all Atoms are serialisable, the returned node should
 * be checked (can be treated as a bool) before use. */
inline Redland::Node
atom_to_node(Redland::World& world, const Atom& atom)
{
	std::ostringstream os;
	std::string        str;
	librdf_uri*        type = NULL;
	librdf_node*       node = NULL;

	switch (atom.type()) {
	case Atom::INT:
		os << atom.get_int32();
		str = os.str();
		// xsd:integer -> pretty integer literals in Turtle
		type = librdf_new_uri(world.world(), CUC("http://www.w3.org/2001/XMLSchema#integer"));
		break;
	case Atom::FLOAT:
		os.precision(20);
		os << atom.get_float();
		str = os.str();
		// xsd:decimal -> pretty decimal (float) literals in Turtle
		type = librdf_new_uri(world.world(), CUC("http://www.w3.org/2001/XMLSchema#decimal"));
		break;
	case Atom::BOOL:
		// xsd:boolean -> pretty boolean literals in Turtle
		if (atom.get_bool())
			str = "true";
		else
			str = "false";
		type = librdf_new_uri(world.world(), CUC("http://www.w3.org/2001/XMLSchema#boolean"));
		break;
	case Atom::STRING:
		str = atom.get_string();
		break;
	case Atom::BLOB:
	case Atom::NIL:
	default:
		//std::cerr << "WARNING: Unserializable Atom!" << std::endl;
		break;
	}
	
	if (str != "")
		node = librdf_new_node_from_typed_literal(world.world(), CUC(str.c_str()), NULL, type);

	return Redland::Node(world, node);
}


} // namespace AtomRDF
} // namespace Raul

#endif // RAUL_ATOM_RDF_HPP

