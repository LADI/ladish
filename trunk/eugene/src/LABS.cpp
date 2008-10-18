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
#include "eugene/LABS.hpp"

using namespace std;

namespace Eugene {
	
	
boost::shared_ptr<LABS::Population>
LABS::initial_population(size_t gene_size, size_t pop_size) const
{
	boost::shared_ptr<Population> ret(new std::vector<GeneType>(pop_size, GeneType(gene_size)));

	for (size_t i = 0; i < pop_size; ++i) {
		for (size_t j = 0; j < gene_size; ++j) {
			if (rand() % 2)
				(*ret)[i][j] = -1;
			else
				(*ret)[i][j] = 1;
		}

		assert(assert_gene((*ret)[i]));
	}
	
	return ret;
}


float
LABS::fitness(const GeneType& gene) const
{
	float f = 0.0f;
		
	for (size_t g = 1; g < gene.size(); ++g) {
		int32_t cg = c_g(gene.size(), g, gene);
		f += cg * cg;
	}

	return f;
}
	

float
LABS::optimum() const
{
	assert(_gene_size >= 3);
	assert(_gene_size <= 48);

	int32_t optima[] = {
		1,
		2,
		2,
		7,
		3,
		8,
		12,
		13,
		5,
		10,
		6,
		19,
		15,
		24,
		32,
		25,
		29,
		26,
		26,
		39,
		47,
		36,
		36,
		45,
		37,
		50,
		62,
		59,
		67,
		64,
		64,
		65,
		73,
		82,
		86,
		87,
		99,
		108,
		108,
		101,
		109,
		122,
		118,
		131,
		135,
		140
	};

	return optima[_gene_size - 3];
}



} // namespace Eugene

