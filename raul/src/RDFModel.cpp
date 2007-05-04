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

#include <sstream>
#include "raul/RDFWorld.h"
#include "raul/RDFModel.h"
#include "raul/RDFNode.h"
#include "raul/AtomRedland.h"

#define U(x) ((const unsigned char*)(x))

using namespace std;

namespace Raul {
namespace RDF {


//static const char* const RDF_LANG = "rdfxml-abbrev";
static const char* const RDF_LANG = "turtle";


/** Create an empty in-memory RDF model.
 */
Model::Model(RDF::World& world)
	: _world(world)
	, _serializer(NULL)
{ 
	_storage = librdf_new_storage(_world.world(), "hashes", NULL, "hash-type='memory'");
	_model = librdf_new_model(_world.world(), _storage, NULL);
}


/** Load a model from a URI (local file or remote).
 */
Model::Model(World& world, const Glib::ustring& data_uri, Glib::ustring base_uri_str)
	: _world(world)
	, _serializer(NULL)
{
	_storage = librdf_new_storage(_world.world(), "hashes", NULL, "hash-type='memory'");
	_model = librdf_new_model(_world.world(), _storage, NULL);

	librdf_uri* uri = librdf_new_uri(world.world(), (const unsigned char*)data_uri.c_str());
	librdf_uri* base_uri = NULL;
	if (base_uri_str != "")
		base_uri = librdf_new_uri(world.world(), (const unsigned char*)base_uri_str.c_str());
	
	if (uri) {
		librdf_parser* parser = librdf_new_parser(world.world(), "guess", NULL, NULL);
		librdf_parser_parse_into_model(parser, uri, base_uri, _model);
		

		librdf_free_parser(parser);
	} else {
		cerr << "Unable to create URI " << data_uri << endl;
	}
	
	if (base_uri)
		librdf_free_uri(base_uri);
	
	if (uri)
		librdf_free_uri(uri);

	/*cout << endl << "Loaded model from " << data_uri << ":" << endl;
	serialize_to_file_handle(stdout);
	cout << endl << endl;*/
}


Model::~Model()
{
	if (_serializer)
		librdf_free_serializer(_serializer);

	librdf_free_model(_model);
	librdf_free_storage(_storage);
}


void
Model::setup_prefixes()
{
	assert(_serializer);

	for (Namespaces::const_iterator i = _world.prefixes().begin(); i != _world.prefixes().end(); ++i) {
		librdf_serializer_set_namespace(_serializer,
			librdf_new_uri(_world.world(), U(i->second.c_str())), i->first.c_str());
	}
}


/** Begin a serialization to a C file handle.
 *
 * This must be called before any write methods.
 */
void
Model::serialize_to_file_handle(FILE* fd)
{
	_serializer = librdf_new_serializer(_world.world(), RDF_LANG, NULL, NULL);
	setup_prefixes();
	librdf_serializer_serialize_model_to_file_handle(_serializer, fd, NULL, _model);
	librdf_free_serializer(_serializer);
	_serializer = NULL;
}


/** Begin a serialization to a file.
 *
 * This must be called before any write methods.
 */
void
Model::serialize_to_file(const string& filename)
{
	_serializer = librdf_new_serializer(_world.world(), RDF_LANG, NULL, NULL);
	setup_prefixes();
	librdf_serializer_serialize_model_to_file(_serializer, filename.c_str(), NULL, _model);
	librdf_free_serializer(_serializer);
	_serializer = NULL;
}


/** Begin a serialization to a string.
 *
 * This must be called before any write methods.
 *
 * The results of the serialization will be returned by the finish() method after
 * the desired objects have been serialized.
 */
string
Model::serialize_to_string()
{
	_serializer = librdf_new_serializer(_world.world(), RDF_LANG, NULL, NULL);
	setup_prefixes();

	unsigned char* c_str
		= librdf_serializer_serialize_model_to_string(_serializer, NULL, _model);

	string result((const char*)c_str);
	free(c_str);
	
	librdf_free_serializer(_serializer);
	_serializer = NULL;

	return result;
}


void
Model::add_statement(const Node& subject,
                     const Node& predicate,
                     const Node& object)
{
	librdf_statement* triple = librdf_new_statement_from_nodes(_world.world(),
			subject.get_node(), predicate.get_node(), object.get_node());
	
	librdf_model_add_statement(_model, triple);
}

	
void
Model::add_statement(const Node&   subject,
                     const string& predicate_id,
                     const Node&   object)
{
	const string predicate_uri = _world.expand_uri(predicate_id);
	librdf_node* predicate = librdf_new_node_from_uri_string(_world.world(),
			(const unsigned char*)predicate_uri.c_str());

	librdf_statement* triple = librdf_new_statement_from_nodes(_world.world(),
			subject.get_node(), predicate, object.get_node());
	
	librdf_model_add_statement(_model, triple);
}


void
Model::add_statement(const Node& subject,
                     const Node& predicate,
                     const Atom& object)
{
	librdf_node* atom_node = AtomRedland::atom_to_rdf_node(_world.world(), object);

	if (atom_node) {
		librdf_statement* triple = librdf_new_statement_from_nodes(_world.world(),
			subject.get_node(), predicate.get_node(), atom_node);
		librdf_model_add_statement(_model, triple);
	} else {
		cerr << "WARNING: Unable to add statement (unserializable Atom)" << endl;
	}
}


void
Model::add_statement(const Node&   subject,
                     const string& predicate_id,
                     const Atom&   object)
{
	const string predicate_uri = _world.expand_uri(predicate_id);
	librdf_node* predicate = librdf_new_node_from_uri_string(_world.world(),
			(const unsigned char*)predicate_uri.c_str());

	librdf_node* atom_node = AtomRedland::atom_to_rdf_node(_world.world(), object);

	if (atom_node) {
		librdf_statement* triple = librdf_new_statement_from_nodes(_world.world(),
			subject.get_node(), predicate, atom_node);
		librdf_model_add_statement(_model, triple);
	} else {
		cerr << "WARNING: Unable to add statement (unserializable Atom)" << endl;
	}
}



} // namespace RDF
} // namespace Raul

