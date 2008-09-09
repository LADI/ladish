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

#ifndef REDLANDMM_NAMESPACES_HPP
#define REDLANDMM_NAMESPACES_HPP

#include <map>
#include <string>

namespace Redland {


/** Collection of RDF namespaces with prefixes.
 */
class Namespaces : public std::map<std::string, std::string> {
public:
	std::string qualify(std::string uri) const;
};


} // namespace Redland

#endif // REDLANDMM_NAMESPACES_HPP
