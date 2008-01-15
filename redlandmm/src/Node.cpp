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

#include <iostream>
#include <cassert>
#include <sstream>
#include <redlandmm/World.hpp>
#include <redlandmm/Node.hpp>

using namespace std;

namespace Redland {


Node::Node(World& world, Type type, const std::string& s)
	: _world(&world)
{
	Glib::Mutex::Lock lock(world.mutex(), Glib::TRY_LOCK);

	if (type == RESOURCE) {
		const string uri = world.expand_uri(s);
		_c_obj = librdf_new_node_from_uri_string(world.world(), (const unsigned char*)uri.c_str());
	} else if (type == LITERAL) {
		_c_obj = librdf_new_node_from_literal(world.world(), (const unsigned char*)s.c_str(), NULL, 0);
	} else if (type == BLANK) {
		_c_obj = librdf_new_node_from_blank_identifier(world.world(), (const unsigned char*)s.c_str());
	} else {
		_c_obj = NULL;
	}

	assert(this->type() == type);
	assert(_world);
}
	

Node::Node(World& world)
	: _world(&world)
{
	Glib::Mutex::Lock lock(world.mutex(), Glib::TRY_LOCK);
	_c_obj = librdf_new_node(world.world());
	assert(_world);
}


Node::Node(World& world, librdf_node* node)
	: _world(&world)
{
	Glib::Mutex::Lock lock(world.mutex(), Glib::TRY_LOCK);
	_c_obj = librdf_new_node_from_node(node);
	assert(_world);
}


Node::Node(const Node& other)
	: Wrapper<librdf_node>()
	, _world(other.world())
{
	if (_world) {
		Glib::Mutex::Lock lock(_world->mutex(), Glib::TRY_LOCK);
		_c_obj = (other._c_obj ? librdf_new_node_from_node(other._c_obj) : NULL);
	}

	assert(to_string() == other.to_string());
}


Node::~Node()
{
	if (_world) {
		Glib::Mutex::Lock lock(_world->mutex(), Glib::TRY_LOCK);
		if (_c_obj)
			librdf_free_node(_c_obj);
	}
}


std::string
Node::to_string() const
{
	return std::string(to_c_string());
}


const char*
Node::to_c_string() const
{
	const Type type = this->type();
	if (type == RESOURCE) {
		assert(librdf_node_get_uri(_c_obj));
		return (const char*)librdf_uri_as_string(librdf_node_get_uri(_c_obj));
	} else if (type == LITERAL) {
		return (const char*)librdf_node_get_literal_value(_c_obj);
	} else if (type == BLANK) {
		return (const char*)librdf_node_get_blank_identifier(_c_obj);
	} else {
		return "";
	}
}

/*
Glib::ustring
Node::to_quoted_uri_string() const
{
	assert(type() == RESOURCE);
	assert(librdf_node_get_uri(_c_obj));
	Glib::ustring str = "<";
	str.append((const char*)librdf_uri_as_string(librdf_node_get_uri(_c_obj)));
	str.append(">");
	return str;
}
*/

bool
Node::is_int() const
{
	if (_c_obj && librdf_node_get_type(_c_obj) == LIBRDF_NODE_TYPE_LITERAL) {
		librdf_uri* datatype_uri = librdf_node_get_literal_value_datatype_uri(_c_obj);
		if (datatype_uri && !strcmp((const char*)librdf_uri_as_string(datatype_uri),
					"http://www.w3.org/2001/XMLSchema#integer"))
			return true;
	}
	return false;
}


int
Node::to_int() const
{
	assert(is_int());
	return strtol((const char*)librdf_node_get_literal_value(_c_obj), NULL, 10);
}


bool
Node::is_float() const
{
	if (_c_obj && librdf_node_get_type(_c_obj) == LIBRDF_NODE_TYPE_LITERAL) {
		librdf_uri* datatype_uri = librdf_node_get_literal_value_datatype_uri(_c_obj);
		if (datatype_uri && !strcmp((const char*)librdf_uri_as_string(datatype_uri),
					"http://www.w3.org/2001/XMLSchema#decimal"))
			return true;
	}
	return false;
}


float
Node::to_float() const
{
	assert(is_float());
	float f = 0.0f;
	std::stringstream ss((const char*)librdf_node_get_literal_value(_c_obj));
	ss >> f;
	return f;
}


bool
Node::is_bool() const
{
	if (_c_obj && librdf_node_get_type(_c_obj) == LIBRDF_NODE_TYPE_LITERAL) {
		librdf_uri* datatype_uri = librdf_node_get_literal_value_datatype_uri(_c_obj);
		if (datatype_uri && !strcmp((const char*)librdf_uri_as_string(datatype_uri),
					"http://www.w3.org/2001/XMLSchema#boolean"))
			return true;
	}
	return false;
}


float
Node::to_bool() const
{
	assert(is_bool());
	if (!strcmp((const char*)librdf_node_get_literal_value(_c_obj), "true"))
		return true;
	else
		return false;
}


} // namespace Redland

