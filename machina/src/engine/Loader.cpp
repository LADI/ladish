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
#include <glibmm/ustring.h>
#include <redlandmm/Query.hpp>
#include "machina/Loader.hpp"
#include "machina/Node.hpp"
#include "machina/Edge.hpp"
#include "machina/Machine.hpp"
#include "machina/ActionFactory.hpp"

using namespace Raul;
using namespace std;

namespace Machina {


Loader::Loader(Redland::World& rdf_world)
	: _rdf_world(rdf_world)
{
	_rdf_world.add_prefix("xsd", "http://www.w3.org/2001/XMLSchema#");
	_rdf_world.add_prefix("", "http://drobilla.net/ns/machina#");
}


/** Load (create) all objects from RDF into the engine.
 *
 * @param uri URI of machine (resolvable URI to an RDF document).
 * @return Loaded Machine.
 */
SharedPtr<Machine>
Loader::load(const Glib::ustring& uri)
{
	using Redland::Query;
	SharedPtr<Machine> machine;

	Glib::ustring document_uri = uri;

	// If "URI" doesn't contain a colon, try to resolve as a filename
	if (uri.find(":") == Glib::ustring::npos) {
		raptor_uri* base_uri = raptor_new_uri((const unsigned char*)"file:.");
		raptor_uri* document_raptor_uri = raptor_new_uri_relative_to_base(
				base_uri, (const unsigned char*)uri.c_str());
		if (document_raptor_uri) {
			document_uri = (char*)raptor_uri_as_string(document_raptor_uri);
			raptor_free_uri(document_raptor_uri);
			raptor_free_uri(base_uri);
		} else {
			raptor_free_uri(base_uri);
			return machine; // NULL
		}
	}

	const Glib::ustring machine_uri = "<>";

	cout << "[Loader] Loading " << machine_uri << " from " << document_uri << endl;

	machine = SharedPtr<Machine>(new Machine());
	
	typedef std::map<string, SharedPtr<Node> > Created;
	Created created;

	Redland::Model model(_rdf_world, document_uri);

	/* Get initial nodes */

	Query query = Query(_rdf_world, Glib::ustring(
			"SELECT DISTINCT ?initialNode ?duration FROM <")
			+ document_uri + "> WHERE {\n" +
			machine_uri + " :initialNode ?initialNode .\n"
			"?initialNode   :duration    ?duration .\n"
			"}\n");

	Query::Results results = query.run(_rdf_world, model);

	for (Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		Glib::ustring node_id  = (*i)["initialNode"].to_string();
		float         duration = (*i)["duration"].to_float();

		SharedPtr<Node> node(new Node(duration, true));
		machine->add_node(node);
		created.insert(std::make_pair(node_id.collate_key(), node));
	}


	/* Get remaining (non-initial) nodes */

	query = Query(_rdf_world, Glib::ustring(
			"SELECT DISTINCT ?node ?duration FROM <")
			+ document_uri + "> WHERE {\n" +
			machine_uri + " :node     ?node .\n"
			"?node          :duration ?duration .\n"
			"}\n");

	results = query.run(_rdf_world, model);

	for (Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		Glib::ustring node_id  = (*i)["initialNode"].to_string();
		float         duration = (*i)["duration"].to_float();

		if (created.find(node_id.collate_key()) == created.end()) {
			SharedPtr<Node> node(new Node(duration, false));
			machine->add_node(node); 
			created.insert(std::make_pair(node_id.collate_key(), node));
		} else {
			cout << "Already created, skipping." << endl;
		}
	}


	/* Get note actions */

	query = Query(_rdf_world, Glib::ustring(
			"SELECT DISTINCT ?node ?note FROM <")
			+ document_uri + "> WHERE {\n" +
			"?node :enterAction [ a :MidiAction; :midiNote ?note ] ;\n"
			"      :exitAction  [ a :MidiAction; :midiNote ?note ] .\n"
			"}\n");

	results = query.run(_rdf_world, model);

	for (Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		Glib::ustring node_id  = (*i)["initialNode"].to_string();
		Glib::ustring note     = (*i)["note"].to_string();

		Created::iterator node_i = created.find(node_id);
		if (node_i != created.end()) {
			SharedPtr<Node> node = node_i->second;
			const long note_num = strtol(note.c_str(), NULL, 10);
			if (note_num >= 0 && note_num <= 127) {
				node->set_enter_action(ActionFactory::note_on((unsigned char)note_num));
				node->set_exit_action(ActionFactory::note_off((unsigned char)note_num));
			} else {
				cerr << "WARNING:  MIDI note number out of range, ignoring." << endl;
			}
		} else {
			cerr << "WARNING:  Found note for unknown states.  Ignoring." << endl;
		}
	}


	/* Get edges */

	query = Query(_rdf_world, Glib::ustring(
			"SELECT DISTINCT ?edge ?src ?dst ?prob FROM <")
			+ document_uri + "> WHERE {\n" +
			machine_uri + " :edge ?edge .\n"
			"?edge :tail        ?src ;\n"
			"      :head        ?dst ;\n"
			"      :probability ?prob .\n }");

	results = query.run(_rdf_world, model);

	for (Query::Results::iterator i = results.begin(); i != results.end(); ++i) {
		Glib::ustring src_uri = (*i)["src"].to_string();
		Glib::ustring dst_uri = (*i)["dst"].to_string(); 
		float         prob    = (*i)["prob"].to_float();

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

	if (machine && machine->nodes().size() > 0) {
		machine->reset();
		return machine;
	} else {
		return SharedPtr<Machine>();
	}
}


} // namespace Machina

