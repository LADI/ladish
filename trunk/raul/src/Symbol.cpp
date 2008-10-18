/* This file is part of Raul.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
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

#include "raul/Symbol.hpp"
#include "raul/Path.hpp"

using namespace std;

namespace Raul {


bool
Symbol::is_valid(const std::basic_string<char>& symbol)
{
	if (symbol.length() == 0)
		return false;

	// Slash not allowed
	if (symbol.find("/") != string::npos)
		return false;

	// All characters must be _, a-z, A-Z, 0-9
	for (size_t i=0; i < symbol.length(); ++i)
		if (symbol[i] != '_'
				&& (symbol[i] < 'a' || symbol[i] > 'z')
				&& (symbol[i] < 'A' || symbol[i] > 'Z')
				&& (symbol[i] < '0' || symbol[i] > '9') )
			return false;

	// First character must not be a number
	if (std::isdigit(symbol[0]))
		return false;

	return true;
}


/** Convert a string to a valid symbol.
 *
 * This will make a best effort at turning @a str into a complete, valid
 * Symbol, and will always return one.
 */
string
Symbol::symbolify(const std::basic_string<char>& str)
{
	string symbol = str;

	Path::replace_invalid_chars(symbol, true);
	
	if (symbol.length() == 0)
		return "_";

	if (isdigit(symbol[0]))
		symbol = string("_").append(symbol); 

	assert(is_valid(symbol));
	
	return symbol;
}


} // namespace Raul

