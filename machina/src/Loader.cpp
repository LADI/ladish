/* This file is part of Machina.  Copyright (C) 2006 Dave Robillard.
 * 
 * Machina is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Machina is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <iostream>
#include <raptor.h>
#include <rasqal.h>
#include "raul/RDFQuery.h"
#include "Loader.hpp"
#include "Node.hpp"
#include "Machine.hpp"

using namespace Raul;
using std::cerr; using std::cout; using std::endl;

namespace Machina {


// FIXME: remove
Node* create_debug_node(const Node::ID& id, FrameCount duration)
{
	// leaks like a sieve, obviously
	
	Node* n = new Node(duration);
	PrintAction* a_enter = new PrintAction(string("> ") + id);
	PrintAction* a_exit = new PrintAction(string("< ")/* + name*/);

	n->add_enter_action(a_enter);
	n->add_exit_action(a_exit);

	cerr << "dur: " << duration  << endl;

	return n;
}

	
Loader::Loader(SharedPtr<Namespaces> namespaces)
	: _namespaces(namespaces)
{
	if (!_namespaces)
		_namespaces = SharedPtr<Namespaces>(new Namespaces());

	(*_namespaces)[""] = "http://drobilla.net/ns/machina#";
	(*_namespaces)["xsd"] = "http://www.w3.org/2001/XMLSchema#";
}


/** Load (create) all objects from an RDF into the engine.
 *
 * @param filename Filename to load objects from.
 * @param parent Path of parent under which to load objects.
 * @return whether or not load was successful.
 */
SharedPtr<Machine>
Loader::load(const Glib::ustring& filename)
{
	SharedPtr<Machine> machine;

	rasqal_init();

	unsigned char* document_uri_str = raptor_uri_filename_to_uri_string(filename.c_str());
	assert(document_uri_str);
	raptor_uri* document_raptor_uri = raptor_new_uri(document_uri_str);

	if (!document_uri_str)
		return machine;

	machine = SharedPtr<Machine>(new Machine(1));

	Glib::ustring document_uri = (const char*)document_uri_str;

	string machine_uri = "<> ";

	cerr << "[Loader] Loading " << machine_uri << " from " << document_uri << endl;

	/* Load nodes */
	
	RDFQuery query = RDFQuery(*_namespaces, Glib::ustring(
		"SELECT DISTINCT ?node ?midiNote ?duration FROM <") + document_uri + "> WHERE {\n" +
		machine_uri + " :node     ?node .\n"
		"?node          :midiNote ?midiNote ;\n"
		"               :duration ?duration .\n"
		//"           FILTER ( datatype(?midiNote) = xsd:decimal )\n"
		//"           FILTER ( datatype(?duration) = xsd:decimal )\n"
		"}");
	
	RDFQuery::Results results = query.run(document_uri);

	for (RDFQuery::Results::iterator i = results.begin(); i != results.end(); ++i) {
		raptor_uri* node_uri  = raptor_new_uri((const unsigned char*)(*i)["node"].c_str());
		unsigned char* node_name
			= raptor_uri_to_relative_uri_string(document_raptor_uri, node_uri);

		const Glib::ustring& note     = (*i)["midiNote"];
		const Glib::ustring& duration = (*i)["duration"];

		cout << "NODE: " << node_name << ": " << note << " - " << duration << endl;
		SharedPtr<Node> node = SharedPtr<Node>(
			create_debug_node((const char*)node_name, strtol(duration.c_str(), NULL, 10)));
		
		machine->add_node(string((const char*)node_name).substr(1), node); // (chop leading "#")

		raptor_free_uri(node_uri);
		free(node_name);
	}
	
	free(document_uri_str);
	raptor_free_uri(document_raptor_uri);

	return machine;
}


} // namespace Machina

