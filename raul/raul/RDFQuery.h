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

#ifndef RAUL_RDFQUERY_H
#define RAUL_RDFQUERY_H

#include <map>
#include <list>
#include <glibmm/ustring.h>
#include "raul/Namespaces.h"

namespace Raul {


/** Pretty wrapper for a SPARQL query.
 *
 * Automatically handles things like prepending prefixes, etc.  Raul specific.
 */
class RDFQuery {
public:
	typedef std::map<Glib::ustring, Glib::ustring> Bindings;
	typedef std::list<Bindings>                    Results;

	RDFQuery(const Namespaces& prefixes, Glib::ustring query)
	{
		// Prepend prefix header
		for (Namespaces::const_iterator i = prefixes.begin();
				i != prefixes.end(); ++i) {
			_query += "PREFIX ";
			_query += i->first + ": <" + i->second + ">\n";
		}
		_query += "\n";
		_query += query;
	}

	Results run(const Glib::ustring base_uri) const;

	Glib::ustring string() const { return _query; };

private:

	Glib::ustring _query;
};


} // namespace Raul

#endif // RAUL_RDFQUERY_H

