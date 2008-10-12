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
#include <vector>
#include <boost/shared_ptr.hpp>
#include "Gene.hpp"

using namespace boost;

namespace Eugene {

class Gene;


template <typename G>
class Problem {
public:
	Problem(size_t gene_size=0) : _gene_size(gene_size) {}

	typedef std::vector<G> Population;

	virtual float fitness(const G& g) const = 0;
	virtual bool fitness_less_than(float a, float b) const = 0;

	inline bool fitness_less(const G& a, const G& b) const
		{ return fitness_less_than(fitness(a), fitness(b)); } 
	
	inline bool fitness_less(const G& a, float b) const
		{ return fitness_less_than(fitness(a), b); } 
	
	inline bool fitness_less(float a, const G& b) const
		{ return fitness_less_than(a, fitness(b)); } 
	
	inline float total_fitness(shared_ptr<const Population> pop) const {
		float result = 0.0f;

		for (size_t i=0; i < pop->size(); ++i)
			result += fitness((*pop)[i]);

		return result;
	}
	
#ifdef GENE_PRINTING
	void print_gene(const G& g, std::ostream& os=std::cout) const {
		for (size_t i=0; i < g.size(); ++i)
			os << g[i] << " ";

		os << " -> " << fitness(g) << std::endl;
	}
#endif

	bool genes_equal(const G& a, const G& b) const {
		if (a.size() != b.size())
			return false;

		for (size_t i=0; i < a.size(); ++i)
			if (a[i] != b[i])
				return false;

		return true;
	}

	
#ifndef NDEBUG
	virtual bool assert_gene(const G& g) const { return true; }
#endif

	size_t gene_size() { return _gene_size; }

	virtual boost::shared_ptr<Population>
	initial_population(size_t gene_size, size_t pop_size) const = 0;
	
	virtual bool optimum_known() { return false; }
	virtual float optimum() const { return 0; }

protected:
	size_t _gene_size;
};


template <typename G>
struct GeneFitnessComparator
{
	GeneFitnessComparator(Problem<G>& problem) : _problem(problem) {}
	inline bool operator()(const G& a, const G& b) const
		{ return _problem.fitness_less(b, a); }
	Problem<G>& _problem;
};


class OneMax : public Problem< GeneImpl<uint32_t> > {
public:
	typedef GeneImpl<uint32_t> GeneType;
	
	OneMax(size_t gene_size) : Problem<GeneType>(gene_size) {}
	
	boost::shared_ptr<Population>
	initial_population(size_t gene_size, size_t pop_size) const {
		boost::shared_ptr<Population> ret(new Population());
		ret->reserve(pop_size);
		while (ret->size() < pop_size)
			ret->push_back(GeneType(gene_size, 0, 1));

		return ret;
	}

	float fitness(const GeneType& g) const {
		uint32_t sum = 0;
		for (size_t i = 0; i < g.size(); ++i)
			sum += g[i];
		
		return sum / (float)g.size();
	}
	
	bool fitness_less_than(float a, float b) const { return a < b; }

#ifndef NDEBUG
	virtual bool assert_gene(const GeneType& g) const {
		for (size_t i = 0; i < g.size(); ++i) {
			if (g[i] != 0 && g[i] != 1)
				return false;
		}

		return true;
	}
#endif
};


class SimpleMax : public Problem< GeneImpl<uint32_t> > {
public:
	typedef GeneImpl<uint32_t> GeneType;
	
	SimpleMax() : Problem<GeneType>(10) {}
	
	boost::shared_ptr<Population>
	initial_population(size_t gene_size, size_t pop_size) const {
		boost::shared_ptr<Population> ret(new Population());
		ret->reserve(pop_size);
		while (ret->size() < pop_size)
			ret->push_back(GeneType(gene_size, 1, 10));
		return ret;
	}

	float fitness(const GeneType& g) const {
		assert(g.size() == 10);

		return ( (g[0] * g[1] * g[2] * g[3] * g[4])
			/  (float)(g[5] * g[6] * g[7] * g[8] * g[9]) );
	}
	
	bool fitness_less_than(float a, float b) const { return a < b; }

#ifndef NDEBUG
	virtual bool assert_gene(const GeneType& g) const { return true; }
#endif
};


} // namespace Eugene
