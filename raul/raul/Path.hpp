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

#ifndef RAUL_PATH_HPP
#define RAUL_PATH_HPP

#include <cctype>
#include <string>
#include <cassert>
#include <iostream>
using std::string;

namespace Raul {

	
/** Simple wrapper around standard string with useful path-specific methods.
 *
 * This enforces that a Path is a valid OSC path (though it is used for
 * GraphObject paths, which aren't directly OSC paths but a portion of one).
 *
 * A path is divided by slashes (/).  The first character MUST be a slash, and
 * the last character MUST NOT be a slash (except in the special case of the 
 * root path "/", which is the only valid single-character path).
 *
 * Valid characters are the 95 printable ASCII characters (32-126), excluding:
 * space # * , ? [ ] { }
 *
 * \ingroup raul
 */
class Path : public std::basic_string<char> {
public:

	/** Contrust an uninitialzed path, because the STL is annoying. */
	Path() : std::basic_string<char>("/") {}

	/** Construct a Path from an std::string.
	 *
	 * It is a fatal error to construct a Path from an invalid string,
	 * use is_valid first to check.
	 */
	Path(const std::basic_string<char>& path)
	: std::basic_string<char>(path)
	{
		assert(is_valid(path));
	}
	

	/** Construct a Path from a C string.
	 *
	 * It is a fatal error to construct a Path from an invalid string,
	 * use is_valid first to check.
	 */
	Path(const char* cpath)
	: std::basic_string<char>(cpath)
	{
		assert(is_valid(cpath));
	}
	
	static bool is_valid(const std::basic_string<char>& path);
	
	static bool is_valid_name(const std::basic_string<char>& name)
	{
		return name.length() > 0 && is_valid(string("/").append(name));
	}

	static string pathify(const std::basic_string<char>& str);
	static string nameify(const std::basic_string<char>& str);

	static void replace_invalid_chars(string& str, bool replace_slash = false);
	
	bool is_child_of(const Path& parent) const;
	bool is_parent_of(const Path& child) const;

	
	/** Return the name of this object (everything after the last '/').
	 * This is the "method name" for OSC paths.
	 */
	inline std::basic_string<char> name() const
	{
		if ((*this) == "/")
			return "";
		else
			return substr(find_last_of("/")+1);
	}
	
	
	/** Return the parent's path.
	 *
	 * Calling this on the path "/" will return "/".
	 * This is the (deepest) "container path" for OSC paths.
	 */
	inline Path parent() const
	{
		std::basic_string<char> parent = substr(0, find_last_of("/"));
		return (parent == "") ? "/" : parent;
	}
	

	/** Parent path with a "/" appended.
	 *
	 * This exists to avoid needing to be careful about the special case of "/".
	 * To create a child of a path, use parent.base() + child_name.
	 * Returned value is always a valid path, with the single exception that
	 * the last character is "/".
	 */
	inline const string base() const
	{
		if ((*this) == "/")
			return *this;
		else
			return (*this) + "/";
	}

	/** Return true if \a child is equal to, or a descendant of \a parent */
	static bool descendant_comparator(const Path& parent, const Path& child)
	{
		return ( child == parent || (child.length() > parent.length() &&
				(!strncmp(parent.c_str(), child.c_str(), parent.length())
						&& child[parent.length()] == '/')) );
	}
};


} // namespace Raul

#endif // RAUL_PATH_HPP
