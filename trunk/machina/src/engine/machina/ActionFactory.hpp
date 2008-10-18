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

#ifndef MACHINA_ACTIONFACTORY_HPP
#define MACHINA_ACTIONFACTORY_HPP

#include "raul/SharedPtr.hpp"

namespace Machina {

class Action;


namespace ActionFactory {
	SharedPtr<Action> copy(SharedPtr<Action> copy);
	SharedPtr<Action> note_on(unsigned char note);
	SharedPtr<Action> note_off(unsigned char note);
}


} // namespace Machina

#endif // MACHINA_ACTIONFACTORY_HPP
