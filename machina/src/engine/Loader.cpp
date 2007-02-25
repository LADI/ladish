/* This file is part of Machina.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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
#include <map>
#include <cmath>
#include <raptor.h>
#include <rasqal.h>
#include <glibmm/ustring.h>
#include <raul/RDFQuery.h>
#include "machina/Loader.hpp"
#include "machina/Node.hpp"
#include "machina/Edge.hpp"
#include "machina/Machine.hpp"

using namespace Raul;
using std::cerr; using std::cout; using std::endl;

namespace Machina {


Loader::Loader(SharedPtr<Namespaces>  namespaces)
	: _namespaces(namespaces)
{
	if (!_namespaces)
		_namespaces = SharedPtr<Namespaces>(new Namespaces());

	(*_namespaces)[""] = "http://drobilla.net/ns/machina#";
	(*_namespaces)["xsd"] = "http://www.w3.org/2001/XMLSchema#";
}


/** Load (create) all objects from an RDF into the engine.
 *
 * @param uri URI of machine (e.g. resolvable URI to an RDF document).
 * @return Loaded Machine.
 */
SharedPtr<Machine>
Loader::load(const Glib::ustring& uri)
{
	using Raul::RDFQuery;
	SharedPtr<Machine> machine;

	rasqal_init();

	//unsigned char* document_uri_str = raptor_uri_filename_to_uri_string(filename.c_str());
	//assert(document_uri_str);
	//raptor_uri* document_raptor_uri = raptor_new_uri(document_uri_str);
	raptor_uri* document_raptor_uri = raptor_new_uri((const unsigned char*)uri.c_str());

	if (!document_raptor_uri) 
		return machine; // NULL

	machine = SharedPtr<Machine>(new Machine());

	//Glib::ustring document_uri = (const char*)document_uri_str;
	const Glib::ustring& document_uri = uri;

	//string machine_uri = "<> ";
	//string machine_uri = string("<") + document_uri + "> ";
	string machine_uri = "?foo ";

	cout << "[Loader] Loading " << machine_uri << " from " << document_uri << endl;

	typedef std::map<string, SharedPtr<Node> > Created;
	Created created;


	/* Get initial nodes */
	
	Raul::RDFQuery query = Raul::RDFQuery(*_namespaces, Glib::ustring(
		"SELECT DISTINCT ?initialNode ?duration FROM <")
		+ document_uri + "> WHERE {\n" +
		machine_uri + " :initialNode ?initialNode .\n"
		"?initialNode   :duration    ?duration .\n"
		"}\n");

	RDFQuery::Results results = query.run(document_uri);

	for (RDFQuery::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const Glib::ustring& node_uri  = (*i)["initialNode"];
		const Glib::ustring& duration  = (*i)["duration"];
		
		raptor_uri* node_raptor_uri
			= raptor_new_uri((const unsigned char*)node_uri.c_str());

		char* node_name = (char*)
			raptor_uri_to_relative_uri_string(document_raptor_uri, node_raptor_uri);

		cout << "Initial: " << node_name << " - " << duration << endl;

		cerr << "FIXME: load\n";
/*
		SharedPtr<Node> node = SharedPtr<Node>(_node_factory->create_node(
			node_name,
			strtol(midi_note.c_str(), NULL, 10),
			strtol(duration.c_str(), NULL, 10)));

		node->set_initial(true);	
		//machine->add_node(string(node_name).substr(1), node); // (chop leading "#")
		machine->add_node(node);
	
		created.insert(std::make_pair(node_uri.collate_key(), node));
		*/
 
		SharedPtr<Node> node(new Node(strtod(duration.c_str(), NULL), true));
		machine->add_node(node);
		created.insert(std::make_pair(node_uri.collate_key(), node));

		raptor_free_uri(node_raptor_uri);
		free(node_name);
	}
	
	
	/* Get remaining nodes */
	
	query = Raul::RDFQuery(*_namespaces, Glib::ustring(
		"SELECT DISTINCT ?node ?duration FROM <")
		+ document_uri + "> WHERE {\n" +
		machine_uri + " :node     ?node .\n"
		"?node          :duration ?duration .\n"
		"}\n");

	results = query.run(document_uri);

	for (RDFQuery::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const Glib::ustring& node_uri  = (*i)["node"];
		const Glib::ustring& duration  = (*i)["duration"];
		
		raptor_uri* node_raptor_uri
			= raptor_new_uri((const unsigned char*)node_uri.c_str());

		char* node_name = (char*)
			raptor_uri_to_relative_uri_string(document_raptor_uri, node_raptor_uri);

		cout << "Node: " << node_name << " - " << duration << endl;

		cerr << "FIXME: load (2)\n";
		/*
		SharedPtr<Node> node = SharedPtr<Node>(_node_factory->create_node(
			node_name,
			strtol(midi_note.c_str(), NULL, 10),
			strtol(duration.c_str(), NULL, 10)));
		
		
		if (created.find(node_uri) == created.end()) {
			//cout << "Node: " << node_name << ": " << midi_note << " - " << duration << endl;
			//machine->add_node(string(node_name).substr(1), node); // (chop leading "#")
			machine->add_node(node);
			created.insert(std::make_pair(node_uri.collate_key(), node));
		}
		*/
		SharedPtr<Node> node(new Node(strtod(duration.c_str(), NULL), true));
		machine->add_node(node); 
		created.insert(std::make_pair(node_uri.collate_key(), node));

		raptor_free_uri(node_raptor_uri);
		free(node_name);
	}


	/* Get edges */

	query = Raul::RDFQuery(*_namespaces, Glib::ustring(
		"SELECT DISTINCT ?edge ?src ?dst ?prob FROM <")
		+ document_uri + "> WHERE {\n" +
		machine_uri + " :edge ?edge .\n"
		"?edge :tail        ?src ;\n"
		"      :head        ?dst ;\n"
		"      :probability ?prob .\n }");
	
	results = query.run(document_uri);

	for (RDFQuery::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const Glib::ustring& src_uri = (*i)["src"];
		const Glib::ustring& dst_uri = (*i)["dst"]; 
		double               prob    = strtod((*i)["prob"].c_str(), NULL);

		Created::iterator src_i = created.find(src_uri.collate_key());
		Created::iterator dst_i = created.find(dst_uri.collate_key());

		if (src_i != created.end() && dst_i != created.end()) {
			const SharedPtr<Node> src = src_i->second;
			const SharedPtr<Node> dst = dst_i->second;
		
			SharedPtr<Edge> edge(new Edge(src, dst));
			edge->set_probability(prob);
			src->add_outgoing_edge(edge);

		} else {
			cerr << "[Loader] WARNING: Ignored edge between unknown nodes "
				<< src_uri << " -> " << dst_uri << endl;
		}

	}
		
	//free(document_uri_str);
	raptor_free_uri(document_raptor_uri);

	return machine;
}


} // namespace Machina

