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

#pragma once
#include "Selection.hpp"
#include "Problem.hpp"

namespace Eugene {


template <typename G>
struct RouletteSelection : public Selection<G> {

	RouletteSelection(shared_ptr< Problem<G> > problem) : Selection<G>(problem) {}
	
	void prepare(shared_ptr<typename Problem<G>::Population> pop) const {
		GeneFitnessComparator<G> cmp(*(Selection<G>::_problem).get());
		sort(pop->begin(), pop->end(), cmp);
	}

	typename Selection<G>::GenePair
	select_parents(shared_ptr<typename Problem<G>::Population> pop) const
	{
		/* FIXME: slow, this could be O(log(n)) */

		const float total = (Selection<G>::_problem)->total_fitness(pop);

		typename Selection<G>::GenePair result = make_pair(pop->end(), pop->end());

		float accum = 0;

		const float spin_1 = (rand() / (float)RAND_MAX) * total;
		const float spin_2 = (rand() / (float)RAND_MAX) * total;

		typename Problem<G>::Population::iterator i;

		for (i = pop->begin(); i != pop->end(); ++i) {
			accum += (Selection<G>::_problem)->fitness(*i);

			if (result.first == pop->end() && accum >= spin_1) {
				result.first = i;
				if (result.second != pop->end())
					break;
			}

			if (result.second == pop->end() && accum >= spin_2) {
				result.second = i;
				if (result.first != pop->end())
					break;
			}
		}

		return result;
	}
};


} // namespace Eugene

