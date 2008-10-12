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

namespace Eugene {


template <typename G>
struct TournamentSelection : public Selection<G> {
	TournamentSelection(shared_ptr< Problem<G> > problem, size_t n, float p)
		: Selection<G>(problem)
		, _n(n)
		, _p(p)
	{
		assert(p >= 0.0f && p <= 1.0f);
		assert(_n > 0);
	}

	typename Selection<G>::GenePair
	select_parents(shared_ptr<typename Problem<G>::Population> pop) const
	{
		assert(pop->size() >= _n);
		typename Problem<G>::Population::iterator parent_1 = select_parent(pop);
		typename Problem<G>::Population::iterator parent_2 = select_parent(pop);
		while (parent_1 == parent_2)
			parent_2 = select_parent(pop);

		return make_pair(parent_1, parent_2);
	}
	
	typename Problem<G>::Population::iterator
	select_parent(shared_ptr<typename Problem<G>::Population> pop) const
	{
		vector<size_t> players;
		players.reserve(_n);

		while (players.size() < _n) {
			size_t index = rand() % pop->size();
			while (find(players.begin(), players.end(), index) != players.end())
				index = rand() % pop->size();

			players.push_back(index);
		}
		
		if ((rand() / (float)RAND_MAX) < _p) {
			typename Problem<G>::Population::iterator ret = pop->begin() + players[0];
			for (size_t i=1; i < _n; ++i) {
				++Selection<G>::_evaluations;
				if ((Selection<G>::_problem)->fitness_less(*ret, *(pop->begin() + players[i])))
					ret = pop->begin() + players[i];
			}

			return ret;
		} else {
			return pop->begin() + players[rand() % _n];
		}
	}

private:
	size_t _n;
	float  _p;
};


} // namespace Eugene

