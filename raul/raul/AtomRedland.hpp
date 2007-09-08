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

#ifndef RAUL_ATOM_REDLAND_HPP
#define RAUL_ATOM_REDLAND_HPP

#include <iostream>
#include <sstream>
#include <cstring>
#include <librdf.h>
#include <raul/Atom.hpp>
using std::cerr; using std::endl;

#define U(x) ((const unsigned char*)(x))

namespace Raul {


/** Support for serializing an Atom to/from RDF (via librdf).
 *
 * (Here to prevent a unnecessary reddland dependency for Atom).
 * 
 * \ingroup raul
 */
class AtomRedland {
public:
	/** Set this atom's value to the object (value) portion of an RDF triple.
	 *
	 * Caller is responsible for calling free(triple->object).
	 */
	static librdf_node* atom_to_rdf_node(librdf_world* world,
	                                     const Atom&   atom)
	{
		std::ostringstream os;

		librdf_uri*  type = NULL;
		librdf_node* node = NULL;
		std::string  str;

		switch (atom.type()) {
		case Atom::INT:
			os << atom.get_int32();
			str = os.str();
			// xsd:integer -> pretty integer literals in Turtle
			type = librdf_new_uri(world, U("http://www.w3.org/2001/XMLSchema#integer"));
			break;
		case Atom::FLOAT:
			os.precision(20);
			os << atom.get_float();
			str = os.str();
			// xsd:decimal -> pretty decimal (float) literals in Turtle
			type = librdf_new_uri(world, U("http://www.w3.org/2001/XMLSchema#decimal"));
			break;
		case Atom::BOOL:
			// xsd:boolean -> pretty boolean literals in Turtle
			if (atom.get_bool())
				str = "true";
			else
				str = "false";
			type = librdf_new_uri(world, U("http://www.w3.org/2001/XMLSchema#boolean"));
			break;
		case Atom::STRING:
			str = atom.get_string();
			break;
		case Atom::BLOB:
		case Atom::NIL:
		default:
			cerr << "WARNING: Unserializable Atom!" << endl;
		}
		
		if (str != "")
			node = librdf_new_node_from_typed_literal(world, U(str.c_str()), NULL, type);

		return node;
	}

#if 0
	static Atom node_to_atom(librdf_node* node) {
		/*switch (type) {
		case 'i':
			return Atom(arg->i);
		case 'f':
			return Atom(arg->f);
		case 's':
			return Atom(&arg->s);
		//case 'b'
			// FIXME: How to get a blob from a lo_arg?
			//return Atom(arg->b);
		default:
			return Atom();
		}*/
		cerr << "FIXME: node_to_atom\n";
		return Atom();
	}
#endif

};


} // namespace Raul

#endif // RAUL_ATOM_REDLAND_HPP
