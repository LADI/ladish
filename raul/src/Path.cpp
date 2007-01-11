/* This file is part of Ingen.  Copyright (C) 2006-2007 Dave Robillard.
 * 
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "raul/Path.h"


bool
Path::is_valid(const std::basic_string<char>& path)
{
	if (path.length() == 0)
		return false;

	// Must start with a /
	if (path.at(0) != '/')
		return false;
	
	// Must not end with a slash unless "/"
	if (path.length() > 1 && path.at(path.length()-1) == '/')
		return false;

	assert(path.find_last_of("/") != string::npos);
	
	// Double slash not allowed
	if (path.find("//") != string::npos)
		return false;

	// All characters must be printable ASCII
	for (size_t i=0; i < path.length(); ++i)
		if (path.at(i) < 32 || path.at(i) > 126)
			return false;

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
		return "."; // this might not be wise

	replace_invalid_chars(name, true);

	assert(is_valid(string("/") + name));

	return name;
}


/** Replace any invalid characters in @a str with a suitable replacement.
 *
 * Makes a pretty name - underscores are a valid character, but this chops
 * both spaces and underscores, uppercasing the next letter, to create
 * uniform CamelCase names that look nice
 */
void
Path::replace_invalid_chars(string& str, bool replace_slash)
{
	for (size_t i=0; i < str.length(); ++i) {
		if (str[i] == ' ' || str[i] == '_') {
			str[i+1] = std::toupper(str[i+1]); // capitalize next char
			str = str.substr(0, i) + str.substr(i+1); // chop space/underscore
			--i;
		} else if (str[i] ==  '[' || str[i] ==  '{') {
			str[i] = '(';
		} else if (str[i] ==  ']' || str[i] ==  '}') {
			str[i] = ')';
		} else if (str[i] < 32 || str.at(i) > 126
				|| str[i] ==  '#' 
				|| str[i] ==  '*' 
				|| str[i] ==  ',' 
				|| str[i] ==  '?' 
				|| (replace_slash && str[i] ==  '/')) {
			str[i] = '.';
		}
	}

	// Chop brackets
	// No, ruins uniqueness (LADSPA plugins)
	/*while (true) {

		const string::size_type open  = str.find("(");
		const string::size_type close = str.find(")");

		if (open != string::npos) {
			if (close != string::npos)
				str.erase(open, (close - open) + 1);
		} else {
			break;
		}

	}*/
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

