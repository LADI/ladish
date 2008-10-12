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
#include "Crossover.hpp"

namespace Eugene {


template <typename G>
struct OnePointCrossover : public Crossover<G> {
	std::pair<G,G> crossover(const G& parent_1, const G& parent_2) {
		assert(parent_1.size() == parent_2.size());

		const size_t size = parent_1.size();

		G child_a(size);
		G child_b(size);
		
		const size_t chop_index = rand() % size;
		
		for (size_t i=0; i < chop_index; ++i) {
			child_a[i] = parent_1[i];
			child_b[i] = parent_2[i];
		}

		for (size_t i = chop_index; i < size; ++i) {
			child_a[i] = parent_2[i];
			child_b[i] = parent_1[i];
		}

		assert(child_a.size() == parent_1.size());
		assert(child_a.size() == child_b.size());

		return make_pair(child_a, child_b);
	}
};


} // namespace Eugene

