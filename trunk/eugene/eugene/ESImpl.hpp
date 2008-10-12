/* This file is part of Eugene
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Eugene is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Eugene is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Eugene.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <numeric>
#include <cstdlib>
#include <float.h>
#include "Gene.hpp"
#include "ES.hpp"
#include "Crossover.hpp"
#include "Selection.hpp"
#include "Problem.hpp"

using namespace std;

namespace Eugene {


template <typename G>
ESImpl<G>::ESImpl(boost::shared_ptr< Problem<G> >    problem,
                  boost::shared_ptr< Selection<G> >  selection,
                  boost::shared_ptr< Crossover<G> >  crossover,
                  boost::shared_ptr< ESMutation<G> > mutation,
                  size_t                             gene_length,
                  size_t                             population_size)
	: _problem(problem)
	, _selection(selection)
	, _mutation(mutation)
	, _gene_length(gene_length)
	, _population(problem->initial_population(gene_length, population_size))
{
	assert(gene_length > 0);
	_best = boost::shared_ptr<G>(new G(_population->front()));
}


template <typename G>
void
ESImpl<G>::iteration()
{
	++_generation;
	
	_selection->prepare(_population);
	
	boost::shared_ptr<typename Problem<G>::Population> new_population(
			new typename Problem<G>::Population());

	new_population->reserve(_population->size());

	unsigned mutation_success = 0;
	unsigned mutation_fail = 0;
	
	#pragma omp parallel for
	for (int i = 0; i < (int)_population->size(); ++i) {
	//for (typename Problem<G>::Population::iterator i = _population->begin();
	//		i != _population->end(); ++i) {

		const G& parent = (*_population.get())[i];

		// Duplicate
		G child = parent;

		// Mutate
		_mutation->mutate(child);

		// Insert the best into new population
		#pragma omp critical (new_population_lock)
		{
			if (_problem->fitness_less(parent, child)) {
				new_population->push_back(child);
				++mutation_success;
			} else {
				new_population->push_back(parent);
				++mutation_fail;
			}
		}
		
		#pragma omp critical (best_lock)
		{
			if (!_best || _problem->fitness_less(*_best.get(), new_population->back()))
				_best = boost::shared_ptr<G>(new G(new_population->back()));
		}
	}

	_population = new_population;

	float success_rate = (mutation_success / (float)mutation_fail);

	if (success_rate > 1/5.0f) {
		_mutation->set_sigma(_mutation->sigma() / 0.9);
	} else if (success_rate < 1/5.0f) {
		_mutation->set_sigma(_mutation->sigma() * 0.9);
	}
}


template <typename G>
void
ESImpl<G>::print_best() const
{
	_problem->print_gene(*_best.get());
}		


template <typename G>
void
ESImpl<G>::print_population(ostream& str) const
{
	for (typename Problem<G>::Population::const_iterator i = _population->begin(); i != _population->end(); ++i)
		_problem->print_gene(*i, str);
}		


} // namespace Eugene
