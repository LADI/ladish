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
#include <iostream>
#include <cstdlib>
#include "Crossover.hpp"

using namespace std;
using namespace boost;

namespace Eugene {


/** A probabilistic composite of several crossover functions
 */
template <typename G>
class HybridCrossover : public Crossover<G> {
public:
	typedef std::vector< std::pair< float, shared_ptr< Crossover<G> > > > Crossovers;

	void append_crossover(float probability, shared_ptr< Crossover<G> > c) {
		_crossovers.push_back(make_pair(probability, c));
	}

	void set_probability(size_t crossover_index, float probability) {
		const float delta  = probability - _crossovers[crossover_index].first;
		const float others = 1.0 - _crossovers[crossover_index].first;

		if (others == 0.0f)
			return; // bad user, no cookie

		_crossovers[crossover_index].first = probability;
		
		if (probability == 1.0) {
			for (size_t i=0; i < _crossovers.size(); ++i)
				if (i != crossover_index) 
					_crossovers[i].first = 0.0f;
		} else {
			for (size_t i=0; i < _crossovers.size(); ++i)
				if (i != crossover_index) 
					_crossovers[i].first -= delta * (_crossovers[i].first / others);
		}
	}

	std::pair<G,G> crossover(const G& parent_1, const G& parent_2) {
		const float flip = rand() / (float)RAND_MAX;

		float accum = 0;
		for (typename Crossovers::iterator i = _crossovers.begin(); i != _crossovers.end(); ++i) {
			accum += i->first;
			if (flip <= accum)
				return i->second->crossover(parent_1, parent_2);
		}
				
		return _crossovers.back().second->crossover(parent_1, parent_2);
	}

	const Crossovers& crossovers() const { return _crossovers; }

private:
	Crossovers _crossovers;
};


} // namespace Eugene

