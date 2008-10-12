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
#include <set>
#include "Crossover.hpp"

using namespace std;

namespace Eugene {


template <typename G>
class OrderCrossover : public Crossover<G> {
public:
	std::pair<G,G> crossover(const G& parent_1, const G& parent_2) {
		assert(parent_1.size() == parent_2.size());

		const size_t gene_size = parent_1.size();

		std::numeric_limits<typename G::value_type> value_limits;

		G child_a(gene_size, value_limits.max(), value_limits.max());
		G child_b(gene_size, value_limits.max(), value_limits.max());
	
		const size_t rand_1 = rand() % gene_size;
		size_t rand_2 = rand() % gene_size;
		while (rand_2 == rand_1)
			rand_2 = rand() % gene_size;

		const size_t cut_a = std::min(rand_1, rand_2);
		const size_t cut_b = std::max(rand_1, rand_2);
		
		// Copy a chunk from parent1->child_a && parent2->child_b
		for (size_t i=cut_a; i <= cut_b; ++i)
			child_a[i] = parent_1[i];
		for (size_t i=cut_a; i <= cut_b; ++i)
			child_b[i] = parent_2[i];

		const size_t left  = cut_a;
		const size_t right = (cut_b + 1) % gene_size;

		// Fill in the rest with cities in order from the other parent
		fill_in_order(parent_2, child_a, left, right);
		fill_in_order(parent_1, child_b, left, right);
		
		return make_pair(*(G*)&child_a, *(G*)&child_b);
	}

private:
	inline static void fill_in_order(const G&     parent,
	                                 G&           child,
	                                 const size_t left,
	                                 size_t       right)
	{
		assert(parent.size() == child.size());
		size_t parent_read = right;

		while (right != left) {
			if ( ! contains(child, left, right, parent[parent_read])) {
				child[right] = parent[parent_read];
				right = (right + 1) % parent.size();
			}
			
			parent_read = (parent_read + 1) % parent.size();
		}
	}

};


} // namespace Eugene

