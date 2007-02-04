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

#ifndef LOADER_HPP
#define LOADER_HPP

#include <glibmm/ustring.h>
#include "raul/SharedPtr.h"
#include "raul/Path.h"
#include "raul/Namespaces.h"

using Raul::Namespaces;

namespace Machina {

class Machine;


class Loader {
public:
	Loader(SharedPtr<Namespaces> = SharedPtr<Namespaces>());

	SharedPtr<Machine> load(const Glib::ustring& filename);

private:
	SharedPtr<Namespaces>      _namespaces;
};


} // namespace Machina

#endif // LOADER_HPP
