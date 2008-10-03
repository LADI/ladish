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
	
	Atom(const char* type_uri, size_t size, void* val) : _type(BLOB) {
		_blob_type_length = strlen(type_uri) + 1; // + 1 for \0
		_blob_size = size;
		_blob_val = malloc(_blob_type_length + _blob_size);
		memcpy(_blob_val, type_uri, _blob_type_length);
		memcpy((char*)_blob_val + _blob_type_length, val, size);
	}

	~Atom() {
		if (_type == STRING)
			free(_string_val);
		else if (_type == BLOB)
			free(_blob_val);
	}

	// Gotta love C++ boilerplate:
	
	Atom(const Atom& copy)
		: _type(copy._type)
	{
		switch (_type) {
		case NIL:    _blob_val   = 0;                        break;
		case INT:    _int_val    = copy._int_val;            break;
		case FLOAT:  _float_val  = copy._float_val;          break;
		case BOOL:   _bool_val   = copy._bool_val;           break;
		case STRING: _string_val = strdup(copy._string_val); break;
		case BLOB:   _blob_size = copy._blob_size;
		             _blob_type_length = copy._blob_type_length;
		             _blob_val = malloc(_blob_type_length + _blob_size);
		             memcpy(_blob_val, copy._blob_val, _blob_type_length + _blob_size);
					 break;
		}
	}
	
	Atom& operator=(const Atom& other) {
		if (_type == BLOB)
			free(_blob_val);
		else if (_type == STRING)
			free(_string_val);

		_type = other._type;

		switch (_type) {
		case NIL:    _blob_val   = 0;                         break;
		case INT:    _int_val    = other._int_val;            break;
		case FLOAT:  _float_val  = other._float_val;          break;
		case BOOL:   _bool_val   = other._bool_val;           break;
		case STRING: _string_val = strdup(other._string_val); break;
		case BLOB:   _blob_size = other._blob_size;
		             _blob_type_length = other._blob_type_length;
		             _blob_val = malloc(_blob_type_length + _blob_size);
		             memcpy(_blob_val, other._blob_val, _blob_type_length + _blob_size);
					 break;
		}
		return *this;
	}

	inline bool operator==(const Atom& other) const {
		if (_type == other.type()) {
			switch (_type) {
			case NIL:    return true;
			case INT:    return _int_val    == other._int_val;
			case FLOAT:  return _float_val  == other._float_val;
			case BOOL:   return _bool_val   == other._bool_val;
			case STRING: return strcmp(_string_val, other._string_val) == 0;
			case BLOB:   return _blob_val == other._blob_val;
			}
		}
		return false;
	}

	inline bool operator!=(const Atom& other) const { return ! operator==(other); }
	
	inline bool operator<(const Atom& other) const {
		if (_type == other.type()) {
			switch (_type) {
			case NIL:    return true;
			case INT:    return _int_val    < other._int_val;
			case FLOAT:  return _float_val  < other._float_val;
			case BOOL:   return _bool_val   < other._bool_val;
			case STRING: return strcmp(_string_val, other._string_val) < 0;
			case BLOB:   return _blob_val   < other._blob_val;
			}
		}
		return _type < other.type();
	}

	inline size_t data_size() const {
		switch (_type) {
		case NIL:    return 0;
		case INT:    return sizeof(uint32_t);
		case FLOAT:  return sizeof(float);
		case BOOL:   return sizeof(bool);
		case STRING: return strlen(_string_val);
		case BLOB:   return _blob_size;
		}
		return 0;
	}
	
	inline bool is_valid() const { return (_type != NIL); }

	/** Type of this atom.  Always check this before attempting to get the
	 * value - attempting to get the incorrectly typed value is a fatal error.
	 */
	Type type() const { return _type; }

	inline int32_t     get_int32()  const { assert(_type == INT);    return _int_val; }
	inline float       get_float()  const { assert(_type == FLOAT);  return _float_val; }
	inline bool        get_bool()   const { assert(_type == BOOL);   return _bool_val; }
	inline const char* get_string() const { assert(_type == STRING); return _string_val; }
	
	inline const char* get_blob_type() const { assert(_type == BLOB); return (const char*)_blob_val; }
	inline const void* get_blob()      const { assert(_type == BLOB); return (const char*)_blob_val + _blob_type_length; }

private:
	Type _type;
	
	union {
		int32_t _int_val;
		float   _float_val;
		bool    _bool_val;
		char*   _string_val;
		struct {
			size_t  _blob_type_length; // length of type string (first part of buffer, inc. \0)
			size_t  _blob_size;        // length of data after type string
			void*   _blob_val;         // buffer
		};
	};
};


} // namespace Raul

#endif // RAUL_ATOM_HPP
