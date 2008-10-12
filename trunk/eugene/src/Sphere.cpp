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

#include <cassert>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <fstream>
#include <eugene/Sphere.hpp>

using namespace std;

namespace Eugene {
	
	
boost::shared_ptr<Sphere::Population>
Sphere::initial_population(size_t gene_size, size_t pop_size) const
{
	boost::shared_ptr<Population> ret(new std::vector<GeneType>(pop_size, GeneType(gene_size)));

	for (size_t i = 0; i < pop_size; ++i) {
		for (size_t j = 0; j < gene_size; ++j) {
			(*ret)[i][j] = rand() / (float)RAND_MAX;
		}

		assert(assert_gene((*ret)[i]));
	}
	
	return ret;
}


float
Sphere::fitness(const GeneType& gene) const
{
	float f = 0.0f;
		
	for (size_t i = 0; i < gene.size(); ++i)
		f += gene[i] * gene[i];

	return f;
}
	

} // namespace Eugene

