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
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef MACHINA_MACHINE_MUTATION_HPP
#define MACHINA_MACHINE_MUTATION_HPP

#include CONFIG_H_PATH

#if HAVE_EUGENE
	#include <eugene/core/Mutation.hpp>
	#define SUPER : public Eugene::Mutation<Machine>
#else
	#define SUPER
#endif

namespace Machina {

class Machine;

namespace Mutation {

struct Mutation { virtual void mutate(Machine& machine) = 0; };

struct Compress   SUPER { void mutate(Machine& machine); };
struct AddNode    SUPER { void mutate(Machine& machine); };
struct RemoveNode SUPER { void mutate(Machine& machine); };
struct AdjustNode SUPER { void mutate(Machine& machine); };
struct SwapNodes  SUPER { void mutate(Machine& machine); };
struct AddEdge    SUPER { void mutate(Machine& machine); };
struct RemoveEdge SUPER { void mutate(Machine& machine); };
struct AdjustEdge SUPER { void mutate(Machine& machine); };

} // namespace Mutation

} // namespace Machina

#endif // MACHINA_MACHINE_MUTATION_HPP
