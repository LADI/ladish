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

#ifndef MACHINA_EVOLVER_HPP
#define MACHINA_EVOLVER_HPP

#include "raul/SharedPtr.hpp"
#include "raul/Thread.hpp"
#include "eugene/core/GAImpl.hpp"
#include "Schrodinbit.hpp"

namespace Eugene { template <typename G> class HybridMutation; }

namespace Machina {

class Machine;
class Problem;


class Evolver : public Raul::Thread {
public:
	Evolver(const string& target_midi, SharedPtr<Machine> seed);
	
	void seed(SharedPtr<Machine> parent);
	bool improvement() { return _improvement; }

	SharedPtr<const Machine> best() { return _ga->best(); }
	
	typedef Eugene::GAImpl<Machine> MachinaGA;
	
private:
	void _run();

	SharedPtr<MachinaGA> _ga;
	SharedPtr<Problem>   _problem;
	float                _seed_fitness;
	Schrodinbit          _improvement;
};


} // namespace Machina

#endif // MACHINA_EVOLVER_HPP
