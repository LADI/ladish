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

#include "raul/Namespaces.hpp"

namespace Raul {


/** Create a prefixed qname from @a uri, if possible.
 *
 * If @a uri can not be qualified with the namespaces currently in this
 * Namespaces, @a uri will be returned unmodified.
 */
std::string
Namespaces::qualify(std::string uri) const
{
	for (const_iterator i = begin(); i != end(); ++i) {
		size_t ns_len = i->second.length();

		if (uri.substr(0, ns_len) == i->second)
			return i->first + ":" + uri.substr(ns_len);
	}

	return uri;
}


} // namespace Raul

