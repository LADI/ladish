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

#ifndef RDFNODE_H
#define RDFNODE_H

#include <stdexcept>
#include <string>
#include <librdf.h>
#include "raul/Atom.h"

namespace Raul {
namespace RDF {

class World;


class Node {
public:
	// Matches librdf_node_type
	enum Type { UNKNOWN=0, RESOURCE, LITERAL, BLANK };

	Node() : _node(NULL) {}

	Node(World& world, Type t, const std::string& s);
	Node(World& world);
	Node(librdf_node* node);
	Node(const Node& other);
	~Node();

	Type type() const { return ((_node) ? (Type)librdf_node_get_type(_node) : UNKNOWN); }
	
	librdf_node* get_node() const { return _node; }

	operator bool() const { return (_node != NULL); }

	const Node& operator=(const Node& other) {
		if (_node)
			librdf_free_node(_node);
		_node = (other._node) ? librdf_new_node_from_node(other._node) : NULL;
		return *this;
	}
	
	std::string to_string() const;
	std::string to_quoted_uri_string() const;

	bool is_int();
	int  to_int();

	bool  is_float();
	float to_float();

private:
	librdf_node* _node;
};


} // namespace RDF
} // namespace Raul

#endif // RDFNODE_H
