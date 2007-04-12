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
#include "raul/RDFWriter.h"
#include "raul/AtomRaptor.h"

#define U(x) ((const unsigned char*)(x))

using namespace std;

namespace Raul {


//static const char* const RDF_LANG = "rdfxml-abbrev";
static const char* const RDF_LANG = "turtle";


RDFWriter::RDFWriter()
	: _serializer(NULL)
	, _string_output(NULL)
	, _next_blank_id(0)
{ 
	add_prefix("rdf", "http://www.w3.org/1999/02/22-rdf-syntax-ns#");
}


void
RDFWriter::add_prefix(const string& prefix, const string& uri)
{
	_prefixes[prefix] = uri;
	if (_serializer)
		raptor_serialize_set_namespace(_serializer,
			raptor_new_uri(U(uri.c_str())), U(prefix.c_str()));
}


void
RDFWriter::setup_prefixes()
{
	assert(_serializer);

	for (Namespaces::const_iterator i = _prefixes.begin(); i != _prefixes.end(); ++i) {
		raptor_serialize_set_namespace(_serializer,
			raptor_new_uri(U(i->second.c_str())), U(i->first.c_str()));
	}
}


/** Expands the prefix of URI, if the prefix is registered.
 */
string
RDFWriter::expand_uri(const string& uri)
{
	for (Namespaces::const_iterator i = _prefixes.begin(); i != _prefixes.end(); ++i)
		if (uri.substr(0, i->first.length()+1) == i->first + ":")
			return i->second + uri.substr(i->first.length()+1);

	return uri;
}


RdfId
RDFWriter::blank_id()
{
	std::ostringstream ss;
	ss << "n" << _next_blank_id++;
	return RdfId(RdfId::ANONYMOUS, ss.str());
}


/** Begin a serialization to a C file handle.
 *
 * This must be called before any write methods.
 */
void
RDFWriter::start_to_file_handle(FILE* fd) throw (std::logic_error)
{
	if (_serializer)
		throw std::logic_error("start_to_string called with serialization in progress");

	raptor_init();
	_serializer = raptor_new_serializer(RDF_LANG);
	raptor_serialize_start_to_file_handle(_serializer, NULL, fd);
	setup_prefixes();
}


/** Begin a serialization to a file.
 *
 * This must be called before any write methods.
 */
void
RDFWriter::start_to_filename(const string& filename) throw (std::logic_error)
{
	if (_serializer)
		throw std::logic_error("start_to_string called with serialization in progress");

	raptor_init();
	_serializer = raptor_new_serializer(RDF_LANG);
	raptor_serialize_start_to_filename(_serializer, filename.c_str());
	setup_prefixes();
}


/** Begin a serialization to a string.
 *
 * This must be called before any write methods.
 *
 * The results of the serialization will be returned by the finish() method after
 * the desired objects have been serialized.
 */
void
RDFWriter::start_to_string() throw (std::logic_error)
{
	if (_serializer)
		throw std::logic_error("start_to_string called with serialization in progress");

	raptor_init();
	_serializer = raptor_new_serializer(RDF_LANG);
	raptor_serialize_start_to_string(_serializer,
	                                 NULL /*base_uri*/,
									 (void**)&_string_output,
									 NULL /*size*/);
	setup_prefixes();
}


/** Finish a serialization.
 *
 * If this was a serialization to a string, the serialization output
 * will be returned, otherwise the empty string is returned.
 */
string
RDFWriter::finish() throw(std::logic_error)
{
	string ret = "";

	if (!_serializer)
		throw std::logic_error("finish() called with no serialization in progress");

	raptor_serialize_end(_serializer);

	if (_string_output) {
		ret = string((char*)_string_output);
		free(_string_output);
		_string_output = NULL;
	}

	raptor_free_serializer(_serializer);
	_serializer = NULL;

	raptor_finish();

	return ret;
}


void
RDFWriter::write(const RdfId& subject,
                 const RdfId& predicate,
                 const RdfId& object)
{
	assert(_serializer);

	raptor_statement triple;
	
	// FIXME: leaks?
	
	if (subject.type() == RdfId::RESOURCE) {
		triple.subject = (void*)raptor_new_uri((const unsigned char*)
				expand_uri(subject.to_string()).c_str());
		triple.subject_type = RAPTOR_IDENTIFIER_TYPE_RESOURCE;
	} else {
		assert(subject.type() == RdfId::ANONYMOUS);
		triple.subject = (unsigned char*)(strdup(subject.to_string().c_str()));
		triple.subject_type = RAPTOR_IDENTIFIER_TYPE_ANONYMOUS;
	}
	
	assert(predicate.type() == RdfId::RESOURCE);
	triple.predicate = (void*)raptor_new_uri((const unsigned char*)
			expand_uri(predicate.to_string()).c_str());
	triple.predicate_type = RAPTOR_IDENTIFIER_TYPE_RESOURCE;

	if (object.type() == RdfId::RESOURCE) {
		triple.object = (void*)raptor_new_uri((const unsigned char*)
				expand_uri(object.to_string()).c_str());
		triple.object_type = RAPTOR_IDENTIFIER_TYPE_RESOURCE;
	} else {
		assert(object.type() == RdfId::ANONYMOUS);
		triple.object = (unsigned char*)(strdup(object.to_string().c_str()));
		triple.object_type = RAPTOR_IDENTIFIER_TYPE_ANONYMOUS;
	}
	
	raptor_serialize_statement(_serializer, &triple);
	
	if (subject.type() == RdfId::RESOURCE)
		raptor_free_uri((raptor_uri*)triple.subject);
	
	raptor_free_uri((raptor_uri*)triple.predicate);
	
	if (object.type() == RdfId::RESOURCE)
		raptor_free_uri((raptor_uri*)triple.object);
}


void
RDFWriter::write(const RdfId& subject,
                 const RdfId& predicate,
                 const Atom&  object)
{
	assert(_serializer);

	raptor_statement triple;
	
	if (subject.type() == RdfId::RESOURCE) {
		triple.subject = (void*)raptor_new_uri((const unsigned char*)
				expand_uri(subject.to_string()).c_str());
		triple.subject_type = RAPTOR_IDENTIFIER_TYPE_RESOURCE;
	} else {
		assert(subject.type() == RdfId::ANONYMOUS);
		triple.subject = (unsigned char*)(strdup(subject.to_string().c_str()));
		triple.subject_type = RAPTOR_IDENTIFIER_TYPE_ANONYMOUS;
	}
	
	assert(predicate.type() == RdfId::RESOURCE);
	triple.predicate = (void*)raptor_new_uri((const unsigned char*)
			expand_uri(predicate.to_string()).c_str());
	triple.predicate_type = RAPTOR_IDENTIFIER_TYPE_RESOURCE;

	AtomRaptor::atom_to_triple_object(&triple, object);
	
	raptor_serialize_statement(_serializer, &triple);
	
	if (subject.type() == RdfId::RESOURCE)
		raptor_free_uri((raptor_uri*)triple.subject);
	
	raptor_free_uri((raptor_uri*)triple.predicate);
}


} // namespace Raul

