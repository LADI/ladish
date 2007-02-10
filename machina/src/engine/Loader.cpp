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
#include <raptor.h>
#include <rasqal.h>
#include <glibmm/ustring.h>
#include <raul/RDFQuery.h>
#include "machina/Loader.hpp"
#include "machina/Node.hpp"
#include "machina/Edge.hpp"
#include "machina/Machine.hpp"
#include "machina/NodeFactory.hpp"

using namespace Raul;
using std::cerr; using std::cout; using std::endl;

namespace Machina {


/*
// FIXME: remove
Node* create_debug_node(const Node::ID& id, FrameCount duration)
{
	// leaks like a sieve, obviously
	
	Node* n = new Node(duration);
	PrintAction* a_enter = new PrintAction(string("\t> ") + id);
	PrintAction* a_exit = new PrintAction(string("\t< ") + id);

	n->add_enter_action(a_enter);
	n->add_exit_action(a_exit);

	cerr << "dur: " << duration  << endl;

	return n;
}
*/
	
Loader::Loader(SharedPtr<NodeFactory> node_factory,
               SharedPtr<Namespaces>  namespaces)
	: _node_factory(node_factory)
	, _namespaces(namespaces)
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
	using Raul::RDFQuery;
	SharedPtr<Machine> machine;

	rasqal_init();

	unsigned char* document_uri_str = raptor_uri_filename_to_uri_string(filename.c_str());
	assert(document_uri_str);
	raptor_uri* document_raptor_uri = raptor_new_uri(document_uri_str);

	if (!document_uri_str)
		return machine;

	machine = SharedPtr<Machine>(new Machine());

	Glib::ustring document_uri = (const char*)document_uri_str;

	string machine_uri = "<> ";

	cout << "[Loader] Loading " << machine_uri << " from " << document_uri << endl;

	typedef std::map<string, SharedPtr<Node> > Created;
	Created created;


	/* Get initial nodes */
	
	Raul::RDFQuery query = Raul::RDFQuery(*_namespaces, Glib::ustring(
		"SELECT DISTINCT ?initialNode ?midiNote ?duration FROM <")
		+ document_uri + "> WHERE {\n" +
		machine_uri + " :initialNode ?initialNode .\n"
		"?initialNode   :midiNote    ?midiNote ;\n"
		"               :duration    ?duration .\n"
		"}\n");

	RDFQuery::Results results = query.run(document_uri);

	for (RDFQuery::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const Glib::ustring& node_uri  = (*i)["initialNode"];
		const Glib::ustring& midi_note = (*i)["midiNote"];
		const Glib::ustring& duration  = (*i)["duration"];
		
		raptor_uri* node_raptor_uri
			= raptor_new_uri((const unsigned char*)node_uri.c_str());

		char* node_name = (char*)
			raptor_uri_to_relative_uri_string(document_raptor_uri, node_raptor_uri);

		//cout << "Initial: " << node_name << ": " << midi_note << " - " << duration << endl;

		SharedPtr<Node> node = SharedPtr<Node>(_node_factory->create_node(
			node_name,
			strtol(midi_note.c_str(), NULL, 10),
			strtol(duration.c_str(), NULL, 10)));
		
		node->set_initial(true);	
		//machine->add_node(string(node_name).substr(1), node); // (chop leading "#")
		machine->add_node(node);
	
		created.insert(std::make_pair(node_uri.collate_key(), node));

		raptor_free_uri(node_raptor_uri);
		free(node_name);
	}
	
	
	/* Get remaining nodes */
	
	query = Raul::RDFQuery(*_namespaces, Glib::ustring(
		"SELECT DISTINCT ?node ?midiNote ?duration FROM <")
		+ document_uri + "> WHERE {\n" +
		machine_uri + " :node     ?node .\n"
		"?node          :midiNote ?midiNote ;\n"
		"               :duration ?duration .\n"
		"}\n");

	results = query.run(document_uri);

	for (RDFQuery::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const Glib::ustring& node_uri  = (*i)["node"];
		const Glib::ustring& midi_note = (*i)["midiNote"];
		const Glib::ustring& duration  = (*i)["duration"];
		
		raptor_uri* node_raptor_uri
			= raptor_new_uri((const unsigned char*)node_uri.c_str());

		char* node_name = (char*)
			raptor_uri_to_relative_uri_string(document_raptor_uri, node_raptor_uri);


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

		raptor_free_uri(node_raptor_uri);
		free(node_name);
	}


	/* Get edges */

	query = Raul::RDFQuery(*_namespaces, Glib::ustring(
		"SELECT DISTINCT ?src ?edge ?dst FROM <")
		+ document_uri + "> WHERE {\n" +
		machine_uri + " :edge ?edge .\n"
		"?edge :tail ?src ;\n"
		"      :head ?dst .\n }");
	results = query.run(document_uri);

	for (RDFQuery::Results::iterator i = results.begin(); i != results.end(); ++i) {
		const Glib::ustring& src_uri = (*i)["src"];
		const Glib::ustring& dst_uri = (*i)["dst"];

		Created::iterator src_i = created.find(src_uri.collate_key());
		Created::iterator dst_i = created.find(dst_uri.collate_key());

		if (src_i != created.end() && dst_i != created.end()) {
			const SharedPtr<Node> src = src_i->second;
			const SharedPtr<Node> dst = dst_i->second;
		
			src->add_outgoing_edge(SharedPtr<Edge>(new Edge(src, dst)));

		} else {
			cerr << "[Loader] WARNING: Ignored edge between unknown nodes "
				<< src_uri << " -> " << dst_uri << endl;
		}

	}
		
	free(document_uri_str);
	raptor_free_uri(document_raptor_uri);

	return machine;
}


} // namespace Machina

