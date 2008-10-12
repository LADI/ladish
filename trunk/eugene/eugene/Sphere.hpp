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
#include <utility>
#include <string>
#include <set>
#include <cmath>
#include <boost/utility.hpp>
#include "Problem.hpp"
#include "Gene.hpp"

namespace Eugene {


class Sphere : public Problem< GeneImpl<float> >, boost::noncopyable {
public:
	Sphere(size_t gene_size) : Problem< GeneImpl<float> >(gene_size) {}

	typedef float            Allele;
	typedef GeneImpl<Allele> GeneType;

	float fitness(const GeneType& g) const;
	bool fitness_less_than(float a, float b) const { return a > b; }

	virtual boost::shared_ptr<Population>
	initial_population(size_t gene_size, size_t pop_size) const;
	
#ifndef NDEBUG
	bool assert_gene(const GeneType& g) const {
		return true;
	}
#endif

	bool optimum_known() { return true; }
	float optimum() const { return 0.0f; }
};


} // namespace Eugene
