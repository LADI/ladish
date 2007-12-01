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

#ifndef REDLANDMM_NODE_HPP
#define REDLANDMM_NODE_HPP

#include <string>
#include <stdexcept>
#include <librdf.h>
#include <redlandmm/Wrapper.hpp>

namespace Redland {

class World;


/** An RDF Node (resource, literal, etc)
 */
class Node : public Wrapper<librdf_node> {
public:
	enum Type {
		UNKNOWN  = LIBRDF_NODE_TYPE_UNKNOWN,
		RESOURCE = LIBRDF_NODE_TYPE_RESOURCE,
		LITERAL  = LIBRDF_NODE_TYPE_LITERAL,
		BLANK    = LIBRDF_NODE_TYPE_BLANK
	};

	Node() : _world(NULL) {}

	Node(World& world, Type t, const std::string& s);
	Node(World& world);
	Node(World& world, librdf_node* node);
	Node(const Node& other);
	~Node();

	Type type() const { return ((_c_obj) ? (Type)librdf_node_get_type(_c_obj) : UNKNOWN); }

	World* world() const { return _world; }
	
	librdf_node* get_node() const { return _c_obj; }
	librdf_uri*  get_uri()  const { return librdf_node_get_uri(_c_obj); }
	
	inline operator int()           const { return to_int(); }
	inline operator float()         const { return to_float(); }
	inline operator bool()          const { return is_bool() ? to_bool() : (_world && _c_obj); }
	inline operator const char*()   const { return to_c_string(); }

	const Node& operator=(const Node& other) {
		if (_c_obj)
			librdf_free_node(_c_obj);
		_world = other._world;
		_c_obj = (other._c_obj) ? librdf_new_node_from_node(other._c_obj) : NULL;
		return *this;
	}
	
	const char* to_c_string() const;
	std::string to_string() const;

	bool is_int() const;
	int  to_int() const;

	bool  is_float() const;
	float to_float() const;
	
	bool  is_bool() const;
	float to_bool() const;

private:
	World* _world;
};


} // namespace Redland

#endif // REDLANDMM_NODE_HPP
