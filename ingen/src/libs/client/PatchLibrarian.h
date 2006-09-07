/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
 * 
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef PATCHLIBRARIAN_H
#define PATCHLIBRARIAN_H

#include <map>
#include <utility>
#include <string>
#include <libxml/tree.h>
#include <cassert>
//#include "DummyModelClientInterface.h"

using std::string;

namespace Ingen {
namespace Client {

class PatchModel;
class NodeModel;
class ConnectionModel;
class PresetModel;
class OSCModelEngineInterface;
class ModelClientInterface;

	
/** Handles all patch saving and loading.
 *
 * \ingroup IngenClient
 */
class PatchLibrarian
{
public:
	// FIXME: return booleans and set an errstr that can be checked or something?
	
	PatchLibrarian(OSCModelEngineInterface* osc_model_engine_interface)
	: _patch_search_path("."), _engine(osc_model_engine_interface)
	{
		assert(_engine);
	}

	void          path(const string& path) { _patch_search_path = path; }
	const string& path()                   { return _patch_search_path; }
	
	string find_file(const string& filename, const string& additional_path = "");
	
	void save_patch(PatchModel* patch_model, const string& filename, bool recursive);
	string load_patch(PatchModel* pm, bool wait = true, bool existing = false);

private:
	string translate_load_path(const string& path);

	string                         _patch_search_path;
	OSCModelEngineInterface* const _engine;

	/// Translations of paths from the loading file to actual paths (for deprecated patches)
	std::map<string, string> _load_path_translations;

	NodeModel*       parse_node(const PatchModel* parent, xmlDocPtr doc, const xmlNodePtr cur);
	ConnectionModel* parse_connection(const PatchModel* parent, xmlDocPtr doc, const xmlNodePtr cur);
	PresetModel*     parse_preset(const PatchModel* parent, xmlDocPtr doc, const xmlNodePtr cur);
	void             load_subpatch(PatchModel* parent, xmlDocPtr doc, const xmlNodePtr cur);
};


} // namespace Client
} // namespace Ingen

#endif // PATCHLIBRARIAN_H
