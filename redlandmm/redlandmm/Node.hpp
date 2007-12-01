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

namespace Redland {

class World;


/** An RDF Node (resource, literal, etc)
 */
class Node {
public:
	enum Type {
		UNKNOWN  = LIBRDF_NODE_TYPE_UNKNOWN,
		RESOURCE = LIBRDF_NODE_TYPE_RESOURCE,
		LITERAL  = LIBRDF_NODE_TYPE_LITERAL,
		BLANK    = LIBRDF_NODE_TYPE_BLANK
	};

	Node() : _world(NULL), _node(NULL) {}

	Node(World& world, Type t, const std::string& s);
	Node(World& world);
	Node(World& world, librdf_node* node);
	Node(const Node& other);
	~Node();

	Type type() const { return ((_node) ? (Type)librdf_node_get_type(_node) : UNKNOWN); }

	World* world() const { return _world; }
	
	librdf_node* get_node() const { return _node; }
	librdf_uri*  get_uri()  const { return librdf_node_get_uri(_node); }
	
	inline operator int()           const { return to_int(); }
	inline operator float()         const { return to_float(); }
	inline operator bool()          const { return is_bool() ? to_bool() : (_world && _node); }
	inline operator const char*()   const { return to_c_string(); }
	//inline operator Glib::ustring() const { return to_string(); }

	const Node& operator=(const Node& other) {
		if (_node)
			librdf_free_node(_node);
		_world = other._world;
		_node = (other._node) ? librdf_new_node_from_node(other._node) : NULL;
		return *this;
	}
	
	const char* to_c_string() const;
	std::string to_string() const;
	//Glib::ustring to_string() const;
	//Glib::ustring to_quoted_uri_string() const;

	bool is_int() const;
	int  to_int() const;

	bool  is_float() const;
	float to_float() const;
	
	bool  is_bool() const;
	float to_bool() const;

private:
	World*       _world;
	librdf_node* _node;
};


} // namespace Redland

#endif // REDLANDMM_NODE_HPP
