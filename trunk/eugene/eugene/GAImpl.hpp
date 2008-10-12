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
#include "GA.hpp"
#include "Crossover.hpp"
#include "Selection.hpp"
#include "Problem.hpp"

using namespace std;

namespace Eugene {


template <typename G>
GAImpl<G>::GAImpl(boost::shared_ptr< Problem<G> >   problem,
                  boost::shared_ptr< Selection<G> > selection,
                  boost::shared_ptr< Crossover<G> > crossover,
                  boost::shared_ptr< Mutation<G> >  mutation,
                  size_t                            gene_length,
                  size_t                            population_size,
				  size_t                            num_elites,
                  float                             mutation_probability,
                  float                             crossover_probability)
	: _problem(problem)
	, _selection(selection)
	, _crossover(crossover)
	, _mutation(mutation)
	, _gene_length(gene_length)
	, _mutation_probability(mutation_probability)
	, _crossover_probability(crossover_probability)
	, _elite_threshold(FLT_MAX)
	, _num_elites(num_elites)
	, _population(problem->initial_population(gene_length, population_size))
{
	find_elites(_population);
	assert(_elites.size() == num_elites);
	assert(gene_length > 0);
}


template<typename G>
struct FitnessComparator
{
	FitnessComparator(Problem<G>& problem) : _problem(problem) {}
	inline bool operator()(const G& a, float f) const { return _problem.fitness_less(f, a); }
	Problem<G>& _problem;
};


template<typename G>
void
GAImpl<G>::find_elites(boost::shared_ptr<typename Problem<G>::Population> population)
{
	GeneFitnessComparator<G> cmp(*_problem.get());

	// Need more elites, select fittest members from population
	if (_elites.size() < _num_elites) {
		sort(population->begin(), population->end(), cmp);

		for (typename Problem<G>::Population::const_iterator i = population->begin();
				i != population->end() && _elites.size() < _num_elites; ++i) {

			// FIXME
#if 0
			if (_elites.empty() || cmp(_elites.back(), *i)
					/*&& ! std::binary_search(_elites.begin(), _elites.end(), *i)*/) {
				_elites.push_back(*i);
				_elites.sort(cmp);
			}
#endif
			_elites.push_back(*i);
		}
		_elites.sort(cmp);

	// Too many elites, cull the herd
	} else if (_elites.size() > _num_elites) {
		_elites.sort(cmp);

		// Remove dupes
		if (_elites.size() > 1) {
			typename Elites::iterator i = _elites.begin();
			while (i != _elites.end()) {
				typename Elites::iterator next = i;
				++next;

				if (next != _elites.end() && *i == *next)
					_elites.erase(i);

				i = next;
			}
		}

		while (_elites.size() > _num_elites)
			_elites.pop_back();
	}

	const float leetest = _problem->fitness(_elites.front());
	/*cout << "leetest: " << leetest << endl;
	if (_best)
		cout << "best: " << _problem->fitness(*_best.get());*/

	assert(_elites.size() == _num_elites);
	if (!_best || _problem->fitness_less(*_best.get(), leetest)) {
		//cout << "EUGENE new best" << endl;
		_best = boost::shared_ptr<G>(new G(_elites.front()));
	}

	_elite_threshold = _problem->fitness(_elites.back());
}


template <typename G>
void
GAImpl<G>::iteration()
{
	++_generation;
	
	_selection->prepare(_population);

	// Crossover
	boost::shared_ptr<typename Problem<G>::Population> new_population(
			new typename Problem<G>::Population());

	new_population->reserve(_population->size());
	
	// Elitism
	new_population->insert(new_population->begin(), _elites.begin(), _elites.end());
	
	#pragma omp parallel
	while (new_population->size() < _population->size()) {
		const pair<typename Problem<G>::Population::const_iterator,
			  typename Problem<G>::Population::const_iterator> parents
				  = _selection->select_parents(_population);

		const bool do_crossover = _crossover && ((rand() / (float)RAND_MAX) < _crossover_probability);

		const pair<G,G> children = (do_crossover)
			? _crossover->crossover(*parents.first, *parents.second)
			: make_pair(*parents.first, *parents.second);

		if (do_crossover) {
			#pragma omp critical (elite_lock)
			{
				if (_problem->fitness_less(_elite_threshold, children.first))
					_elites.push_back(children.first);

				if (_problem->fitness_less(_elite_threshold, children.second))
					_elites.push_back(children.second);
			}
		}

		#pragma omp critical (new_population_lock)
		{
			if (new_population->size() < _population->size())
				new_population->push_back(children.first);

			if (new_population->size() < _population->size())
				new_population->push_back(children.second);
		}
	}

	// Mutate
	for (typename Problem<G>::Population::iterator i = new_population->begin(); i != new_population->end(); ++i) {
		if ((rand() / (float)RAND_MAX) <= _mutation_probability) {
			_mutation->mutate((G&)*i);
			if (_problem->fitness_less(_elite_threshold, *i)) {
				_elites.push_back(*i);
			}
		}
	}

	find_elites(new_population);

#ifndef NDEBUG
	for (typename Problem<G>::Population::iterator i = new_population->begin(); i != new_population->end(); ++i)
		assert(_problem->assert_gene(*i));
#endif

	_population = new_population;
}

#ifdef GENE_PRINTING

template <typename G>
void
GAImpl<G>::print_best() const
{
	_problem->print_gene(*_best.get());
}		


template <typename G>
void
GAImpl<G>::print_population(ostream& str) const
{
	for (typename Problem<G>::Population::const_iterator i = _population->begin(); i != _population->end(); ++i)
		_problem->print_gene(*i, str);
}		


template <typename G>
void
GAImpl<G>::print_elites() const
{
	for (typename Elites::const_iterator i = _elites.begin(); i != _elites.end(); ++i)
		_problem->print_gene(*i);
}		

#endif


} // namespace Eugene
