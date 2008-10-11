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

#include <raul/Path.hpp>

using namespace std;

namespace Raul {


bool
Path::is_valid(const std::basic_string<char>& path)
{
	if (path.length() == 0)
		return false;

	// Must start with a /
	if (path[0] != '/')
		return false;
	
	// Must not end with a slash unless "/"
	if (path.length() > 1 && path[path.length()-1] == '/')
		return false;

	assert(path.find_last_of("/") != string::npos);
	
	// Double slash not allowed
	if (path.find("//") != string::npos)
		return false;


	for (size_t i=0; i < path.length(); ++i)
		// All contained symbols must not start with a digit
		if (i > 0 && path[i-1] == '/' && isdigit(path[i]))
			return false;
		// All characters must be _, a-z, A-Z, 0-9
		else if ( path[i] != '/' && path[i] != '_'
				&& (path[i] < 'a' || path[i] > 'z')
				&& (path[i] < 'A' || path[i] > 'Z')
				&& (path[i] < '0' || path[i] > '9') )
			return false;

#if 0
	// Disallowed characters
	if (       path.find(" ") != string::npos 
			|| path.find("#") != string::npos
			|| path.find("*") != string::npos
			|| path.find(",") != string::npos
			|| path.find("?") != string::npos
			|| path.find("[") != string::npos
			|| path.find("]") != string::npos
			|| path.find("{") != string::npos
			|| path.find("}") != string::npos)
		return false;
#endif

	return true;
}


/** Convert a string to a valid full path.
 *
 * This will make a best effort at turning @a str into a complete, valid
 * Path, and will always return one.
 */
string
Path::pathify(const std::basic_string<char>& str)
{
	string path = str;

	if (path.length() == 0)
		return "/"; // this might not be wise

	// Must start with a /
	if (path.at(0) != '/')
		path = string("/").append(path);
	
	// Must not end with a slash unless "/"
	if (path.length() > 1 && path.at(path.length()-1) == '/')
		path = path.substr(0, path.length()-1); // chop trailing slash

	assert(path.find_last_of("/") != string::npos);
	
	replace_invalid_chars(path, false);

	assert(is_valid(path));
	
	return path;
}


/** Convert a string to a valid name (or "method" - tokens between slashes)
 *
 * This will strip all slashes, etc, and always return a valid name/method.
 */
string
Path::nameify(const std::basic_string<char>& str)
{
	string name = str;

	if (name.length() == 0)
		return "_"; // this might not be wise

	replace_invalid_chars(name, true);

	assert(is_valid(string("/") + name));

	return name;
}


/** Replace any invalid characters in @a str with a suitable replacement.
 */
void
Path::replace_invalid_chars(string& str, bool replace_slash)
{
	size_t open_bracket = str.find_first_of('(');
	if (open_bracket != string::npos)
		str = str.substr(0, open_bracket);
	
	open_bracket = str.find_first_of('[');
	if (open_bracket != string::npos)
		str = str.substr(0, open_bracket);

	if (str[str.length()-1] == ' ')
		str = str.substr(0, str.length()-1);

//#define STRICT_SYMBOLS

#ifdef STRICT_SYMBOLS
	// dB = special case, need to avoid CamelCase killing below
	while (true) {
		size_t decibels = str.find("dB");
		if (decibels != string::npos)
			str[decibels+1] = 'b';
		else
			break;
	}

	// Kill CamelCase in favour of god_fearing_symbol_names
	for (size_t i=1; i < str.length(); ++i) {
		if (str[i] >= 'A' && str[i] <= 'Z' && str[i-1] >= 'a' && str[i-1] <= 'z')
			str = str.substr(0, i) + '_' + str.substr(i);
	}
#endif

	if (isdigit(str[0]))
		str = string("_").append(str);
	
	for (size_t i=0; i < str.length(); ++i) {
		if (i > 0 && str[i-1] == '/' && isdigit(str[i])) {
			str = str.substr(0, i) + "_" + str.substr(i);
		} else if (str[i] == '\'') {
			str = str.substr(0, i) + str.substr(i+1);
#ifdef STRICT_SYMBOLS
		} else if (str[i] >= 'A' && str[i] <= 'Z') {
			str[i] = std::tolower(str[i]);
#endif
		} else if ( str[i] != '_' && str[i] != '/'
				&& (str[i] < 'a' || str[i] > 'z')
#ifndef STRICT_SYMBOLS
				&& (str[i] < 'A' || str[i] > 'Z')
#endif
				&& (str[i] < '0' || str[i] > '9') ) {
			if (i > 0 && str[i-1] == '_') {
				str = str.substr(0, i) + str.substr(i+1);
				--i;
			} else {
				str[i] = '_';
			}
		} else if (replace_slash && str[i] == '/') {
			str[i] = '_';
		}
	}
	
	if (str[str.length()-1] == '_')
		str = str.substr(0, str.length()-1);
}


bool
Path::is_child_of(const Path& parent) const
{
	const string parent_base = parent.base();
	return (substr(0, parent_base.length()) == parent_base);
}


bool
Path::is_parent_of(const Path& child) const
{
	return child.is_child_of(*this);
}


} // namespace Raul

