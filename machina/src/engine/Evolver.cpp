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

#include <iostream>
#include <eugene/core/Mutation.hpp>
#include <eugene/core/HybridMutation.hpp>
#include <eugene/core/TournamentSelection.hpp>
#include <machina/Evolver.hpp>
#include <machina/Mutation.hpp>
#include <machina/Problem.hpp>

using namespace std;
using namespace Eugene;
using namespace boost;

namespace Machina {


Evolver::Evolver(const string& target_midi, SharedPtr<Machine> seed)
	: _problem(new Problem(target_midi, seed))
	, _active_fitness(-FLT_MAX)
{	
	SharedPtr<Eugene::HybridMutation<Machine> > m(new HybridMutation<Machine>());
	
	m->append_mutation(1/7.0f, boost::shared_ptr< Eugene::Mutation<Machine> >(
			new Mutation::Compress()));
	m->append_mutation(1/7.0f, boost::shared_ptr< Eugene::Mutation<Machine> >(
			new Mutation::AddNode()));
	m->append_mutation(1/7.0f, boost::shared_ptr< Eugene::Mutation<Machine> >(
			new Mutation::RemoveNode()));
	m->append_mutation(1/7.0f, boost::shared_ptr< Eugene::Mutation<Machine> >(
			new Mutation::AdjustNode()));
	m->append_mutation(1/7.0f, boost::shared_ptr< Eugene::Mutation<Machine> >(
			new Mutation::AddEdge()));
	m->append_mutation(1/7.0f, boost::shared_ptr< Eugene::Mutation<Machine> >(
			new Mutation::RemoveEdge()));
	m->append_mutation(1/7.0f, boost::shared_ptr< Eugene::Mutation<Machine> >(
			new Mutation::AdjustEdge()));

	boost::shared_ptr< Selection<Machine> > s(new TournamentSelection<Machine>(_problem, 3, 0.8));
	boost::shared_ptr< Crossover<Machine> > crossover;
	_ga = SharedPtr<MachinaGA>(new MachinaGA(_problem, s, crossover, m, 10, 10, 2, 1.0, 0.0));
}
	

void
Evolver::seed(SharedPtr<Machine> parent)
{
	/*_best = SharedPtr<Machine>(new Machine(*parent.get()));
	_best_fitness = _problem->fitness(*_best.get());*/
	_problem->seed(parent);
	_active_fitness = _problem->fitness(*parent.get());
}
	

void
Evolver::_run()
{
	float old_best = _ga->best_fitness();

	while (true) {
		_ga->iteration();
		//cout << "I" << endl;

		float new_best = _ga->best_fitness();
		if (new_best > old_best) {
			_improvement = true;
			old_best = new_best;
			cout << "NEW BEST: " << new_best << endl;
		}
	}
}


} // namespace Machina

