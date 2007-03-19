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

#ifndef RDFWRITER_H
#define RDFWRITER_H

#include <stdexcept>
#include <string>
#include <raptor.h>
#include "raul/Namespaces.h"
#include "raul/Atom.h"

namespace Raul {


class RdfId {
public:
	enum Type { NULL_ID, ANONYMOUS, RESOURCE };

	RdfId(Type t, const std::string& s) : _type(t), _string(s) {}
	RdfId() : _type(NULL_ID) {}

	Type               type() const      { return _type; }
	const std::string& to_string() const { return _string; }

	operator bool() { return (_type != NULL_ID); }

private:
	Type        _type;
	std::string _string; ///< URI or blank node ID, depending on _type
};


class RDFWriter {
public:
	RDFWriter();

	void add_prefix(const std::string& prefix, const std::string& uri);
	std::string expand_uri(const std::string& uri);

	void start_to_file_handle(FILE* fd)                 throw (std::logic_error);
	void start_to_filename(const std::string& filename) throw (std::logic_error);
	void start_to_string()                              throw (std::logic_error);
	std::string finish()                                throw (std::logic_error);
	
	RdfId blank_id();
	
	bool serialization_in_progress() { return (_serializer != NULL); }

	void write(const RdfId& subject,
	           const RdfId& predicate,
	           const RdfId& object);   
	
	void write(const RdfId& subject,
	           const RdfId& predicate,
	           const Atom&  object);

private:
	void setup_prefixes();

	raptor_serializer* _serializer;
	unsigned char*     _string_output;
	Namespaces         _prefixes;

	size_t _next_blank_id;
};


} // namespace Raul

#endif // RDFWRITER_H
