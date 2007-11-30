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

#ifndef RAUL_ATOM_HPP
#define RAUL_ATOM_HPP

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <string>
#include <sstream>
#include <iostream>

#include CONFIG_H_PATH
#ifdef HAVE_REDLANDMM
#include <redlandmm/Node.hpp>
#include <redlandmm/World.hpp>
#endif

#define CUC(x) ((const unsigned char*)(x))

namespace Raul {


/** An OSC atom (fundamental data types OSC messages are composed of).
 *
 * \ingroup raul
 */
class Atom {
public:

	enum Type {
		NIL,
		INT,
		FLOAT,
		BOOL,
		STRING,
		BLOB
	};

	Atom()                       : _type(NIL),    _blob_val(0)                     {}
	Atom(int32_t val)            : _type(INT),    _int_val(val)                    {}
	Atom(float val)              : _type(FLOAT),  _float_val(val)                  {}
	Atom(bool val)               : _type(BOOL),   _bool_val(val)                   {}
	Atom(const char* val)        : _type(STRING), _string_val(strdup(val))         {}
	Atom(const std::string& val) : _type(STRING), _string_val(strdup(val.c_str())) {}
	
	Atom(void* val) : _type(BLOB), _blob_size(sizeof(val)), _blob_val(malloc(_blob_size))
	{ memcpy(_blob_val, val, sizeof(_blob_size)); }

#ifdef HAVE_REDLANDMM
	Atom(const Redland::Node& node)
	{
		if (node.type() == Redland::Node::RESOURCE) {
			_type = STRING;
			_string_val = strdup(node.to_string().c_str());
		} else if (node.is_float()) {
			_type = FLOAT;
			_float_val = node.to_float();
		} else if (node.is_int()) {
			_type = INT;
			_int_val = node.to_int();
		} else if (node.is_bool()) { 
			_type = BOOL;
			_bool_val = node.to_bool();
		} else {
			_type = STRING;
			_string_val = strdup(node.to_string().c_str());
		}
	}
#endif

	~Atom()
	{
		if (_type == STRING)
			free(_string_val);
		else if (_type == BLOB)
			free(_blob_val);
	}

	// Gotta love C++ boilerplate:
	
	Atom(const Atom& copy)
	: _type(copy._type)
	, _blob_size(copy._blob_size)
	{
		switch (_type) {
		case NIL:    _blob_val   = 0;                        break;
		case INT:    _int_val    = copy._int_val;            break;
		case FLOAT:  _float_val  = copy._float_val;          break;
		case BOOL:   _bool_val   = copy._bool_val;           break;
		case STRING: _string_val = strdup(copy._string_val); break;

		case BLOB:   _blob_val = malloc(_blob_size);
		             memcpy(_blob_val, copy._blob_val, _blob_size);
					 break;

		default: break;
		}
	}
	
	Atom& operator=(const Atom& other)
	{
		if (_type == BLOB)
			free(_blob_val);
		else if (_type == STRING)
			free(_string_val);

		_type = other._type;
		_blob_size = other._blob_size;

		switch (_type) {
		case NIL:    _blob_val   = 0;                         break;
		case INT:    _int_val    = other._int_val;            break;
		case FLOAT:  _float_val  = other._float_val;          break;
		case BOOL:   _bool_val   = other._bool_val;           break;
		case STRING: _string_val = strdup(other._string_val); break;

		case BLOB:   _blob_val = malloc(_blob_size);
		             memcpy(_blob_val, other._blob_val, _blob_size);
					 break;

		default: break;
		}
		return *this;
	}

	/** Type of this atom.  Always check this before attempting to get the
	 * value - attempting to get the incorrectly typed value is a fatal error.
	 */
	Type type() const { return _type; }

	inline int32_t     get_int32()  const { assert(_type == INT);    return _int_val; }
	inline float       get_float()  const { assert(_type == FLOAT);  return _float_val; }
	inline bool        get_bool()   const { assert(_type == BOOL);   return _bool_val; }
	inline const char* get_string() const { assert(_type == STRING); return _string_val; }
	inline const void* get_blob()   const { assert(_type == BLOB);   return _blob_val; }

	inline operator bool() const { return (_type != NIL); }


#ifdef HAVE_REDLANDMM
	Redland::Node to_rdf_node(Redland::World& world) const
	{
		std::ostringstream os;
		std::string        str;
		librdf_uri*        type = NULL;
		librdf_node*       node = NULL;

		switch (_type) {
		case Atom::INT:
			os << get_int32();
			str = os.str();
			// xsd:integer -> pretty integer literals in Turtle
			type = librdf_new_uri(world.world(), CUC("http://www.w3.org/2001/XMLSchema#integer"));
			break;
		case Atom::FLOAT:
			os.precision(20);
			os << get_float();
			str = os.str();
			// xsd:decimal -> pretty decimal (float) literals in Turtle
			type = librdf_new_uri(world.world(), CUC("http://www.w3.org/2001/XMLSchema#decimal"));
			break;
		case Atom::BOOL:
			// xsd:boolean -> pretty boolean literals in Turtle
			if (get_bool())
				str = "true";
			else
				str = "false";
			type = librdf_new_uri(world.world(), CUC("http://www.w3.org/2001/XMLSchema#boolean"));
			break;
		case Atom::STRING:
			str = get_string();
			break;
		case Atom::BLOB:
		case Atom::NIL:
		default:
			std::cerr << "WARNING: Unserializable Atom!" << std::endl;
		}
		
		if (str != "")
			node = librdf_new_node_from_typed_literal(world.world(), CUC(str.c_str()), NULL, type);

		return Redland::Node(world, node);
	}
#endif


private:
	Type _type;
	
	size_t _blob_size; ///< always a multiple of 32
	
	union {
		int32_t _int_val;
		float   _float_val;
		bool    _bool_val;
		char*   _string_val;
		void*   _blob_val;
	};
};


} // namespace Raul

#endif // RAUL_ATOM_HPP
