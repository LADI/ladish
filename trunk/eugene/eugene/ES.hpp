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
#include "GA.hpp"

namespace Eugene {

template <typename G> class ESMutation;

template <typename G>
class ESImpl : public GA
{
public:
	ESImpl(boost::shared_ptr< Problem<G> >    problem,
	       boost::shared_ptr< Selection<G> >  selection,
	       boost::shared_ptr< Crossover<G> >  crossover,
	       boost::shared_ptr< ESMutation<G> > mutation,
	       size_t                             gene_length,
	       size_t                             population_size);

	size_t population_size() const { return _population->size(); }

	boost::shared_ptr<const G> best() const { return _best; }

	virtual void iteration();

	void print_best() const;
	void print_population(std::ostream& str) const;
	
	float best_fitness() const { return _problem->fitness(*_best.get()); }
	
	float fitness_less_than(float a, float b) const
		{ return _problem->fitness_less_than(a, b); }
	
	bool optimum_known() { return _problem->optimum_known(); }
	int32_t optimum() const { return _problem->optimum(); }
	
	int evaluations() const { return _selection->evaluations(); }
	
	boost::shared_ptr< Problem<G> >   problem()   const { return _problem; }
	boost::shared_ptr< Selection<G> > selection() const { return _selection; }
	boost::shared_ptr< Mutation<G> >  mutation()  const { return _mutation; }
	
	boost::shared_ptr<typename Problem<G>::Population> const population()
		{ return _population; }

private:
	typedef std::pair<typename Problem<G>::Population::const_iterator,
                      typename Problem<G>::Population::const_iterator>
			GenePair;
	
	GenePair select_parents(const float total) const;
	
	boost::shared_ptr< Problem<G> >    _problem;
	boost::shared_ptr< Selection<G> >  _selection;
	boost::shared_ptr< ESMutation<G> > _mutation;

	size_t _gene_length;

	boost::shared_ptr<typename Problem<G>::Population> _population;

	boost::shared_ptr<const G> _best;
};


} // namespace Eugene
