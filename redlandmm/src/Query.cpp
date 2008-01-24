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
#include <rasqal.h>
#include <redlandmm/Query.hpp>
#include <redlandmm/Model.hpp>

using namespace std;

namespace Redland {


Query::Results
Query::run(World& world, Model& model, const Glib::ustring base_uri_str) const
{
	Glib::Mutex::Lock lock(world.mutex());

	//cout << "\n**************** QUERY *******************\n";
	//cout << _query << endl;
	//cout << "******************************************\n\n";

	Results result;

	librdf_uri* base_uri = NULL;
	if (base_uri_str != "")
		base_uri = librdf_new_uri(world.world(),
				(const unsigned char*)base_uri_str.c_str());

	librdf_query* q = librdf_new_query(world.world(), "sparql",
		NULL, (unsigned char*)_query.c_str(), base_uri);
	
	if (!q) {
		cerr << "Unable to create query:" << endl << _query << endl;
		return result; /* Return an empty Results */
	}

	// FIXME: locale kludges to work around librdf bug
	char* locale = strdup(setlocale(LC_NUMERIC, NULL));
	setlocale(LC_NUMERIC, "POSIX");

	librdf_query_results* results = librdf_query_execute(q, model._c_obj);
	
	if (!results) {
		cerr << "Failed query:" << endl << _query << endl;
		return result; /* Return an empty Results */
	}

	while (!librdf_query_results_finished(results)) {
		
		Bindings bindings;
		
		for (int i=0; i < librdf_query_results_get_bindings_count(results); i++) {
			const char*  name  = (char*)librdf_query_results_get_binding_name(results, i);
			librdf_node* value = librdf_query_results_get_binding_value(results, i);

			if (name && value)
				bindings.insert(std::make_pair(std::string(name), Node(world, value)));
		}

		result.push_back(bindings);
		librdf_query_results_next(results);
	}
	
	setlocale(LC_NUMERIC, locale);
	free(locale);

	librdf_free_query_results(results);
	librdf_free_query(q);
	
	if (base_uri)
		librdf_free_uri(base_uri);

	return result;
}


} // namespace Redland

