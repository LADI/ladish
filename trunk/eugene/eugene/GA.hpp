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
#include <list>
#include <ostream>
#include <boost/shared_ptr.hpp>
#include <boost/detail/atomic_count.hpp>

namespace Eugene {

template <typename G> class Problem;
template <typename G> class Selection;
template <typename G> class Crossover;
template <typename G> class Mutation;


class GA {
public:
	GA() : _generation(0) {}

	virtual void set_mutation_probability(float p) {}
	virtual void set_crossover_probability(float p) {}
	virtual void set_num_elites(size_t n) {}
	
	virtual void iteration()          = 0;

#ifdef GENE_PRINTING
	virtual void print_best()   const = 0;
	virtual void print_elites() const {}
	virtual void print_population(std::ostream& str) const = 0;
#endif

	virtual float best_fitness() const = 0;
	virtual float fitness_less_than(float a, float b) const = 0;
	
	inline int generation() const { return _generation; }
	virtual int evaluations() const = 0;
	
	virtual bool optimum_known() { return false; }
	virtual int32_t optimum() const { return 0; }
	
protected:
	boost::detail::atomic_count _generation;
};


template <typename G>
class GAImpl : public GA
{
public:
	GAImpl(boost::shared_ptr< Problem<G> >   problem,
	       boost::shared_ptr< Selection<G> > selection,
	       boost::shared_ptr< Crossover<G> > crossover,
	       boost::shared_ptr< Mutation<G> >  mutation,
	       size_t                            gene_length,
	       size_t                            population_size,
		   size_t                            num_elites,
	       float                             mutation_probability,
	       float                             crossover_probability);

	size_t population_size() const { return _population->size(); }

	boost::shared_ptr<const G> best() const { return _best; }

	// FIXME: not really atomic
	void set_mutation_probability(float p)  { _mutation_probability = p; }
	void set_crossover_probability(float p) { _crossover_probability = p; }
	void set_num_elites(size_t n)           { _num_elites = n; }

	virtual void iteration();

	void print_best() const;
	void print_population(std::ostream& str) const;
	void print_elites() const;
	
	float best_fitness() const { return _problem->fitness(*_best.get()); }
	
	float fitness_less_than(float a, float b) const
		{ return _problem->fitness_less_than(a, b); }
	
	bool optimum_known() { return _problem->optimum_known(); }
	int32_t optimum() const { return _problem->optimum(); }
	
	int evaluations() const { return _selection->evaluations(); }
	
	boost::shared_ptr< Problem<G> >   problem()   const { return _problem; }
	boost::shared_ptr< Selection<G> > selection() const { return _selection; }
	boost::shared_ptr< Crossover<G> > crossover() const { return _crossover; }
	boost::shared_ptr< Mutation<G> >  mutation()  const { return _mutation; }
	
	boost::shared_ptr<typename Problem<G>::Population> const population()
		{ return _population; }

private:
	typedef std::pair<typename Problem<G>::Population::const_iterator,
                      typename Problem<G>::Population::const_iterator>
			GenePair;
	
	typedef std::list<G> Elites;

	GenePair select_parents(const float total) const;
	void     find_elites(boost::shared_ptr<typename Problem<G>::Population> population);
	
	boost::shared_ptr< Problem<G> >   _problem;
	boost::shared_ptr< Selection<G> > _selection;
	boost::shared_ptr< Crossover<G> > _crossover;
	boost::shared_ptr< Mutation<G> >  _mutation;

	size_t _gene_length;
	float  _mutation_probability;
	float  _crossover_probability;
	float  _elite_threshold;
	size_t _num_elites;

	boost::shared_ptr<typename Problem<G>::Population> _population;

	boost::shared_ptr<const G> _best;
	Elites                     _elites;
};


} // namespace Eugene
