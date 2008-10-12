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

#include <boost/detail/atomic_count.hpp>

using namespace std;
using namespace boost;

namespace Eugene {

template <typename G> class Problem;


template <typename G>
class Selection {
public:
	typedef std::pair<typename Problem<G>::Population::iterator,
    	              typename Problem<G>::Population::iterator>
		GenePair;

	Selection(shared_ptr< Problem<G> > problem) : _problem(problem), _evaluations(0) {}

	virtual GenePair select_parents(shared_ptr<typename Problem<G>::Population> pop) const = 0;
	virtual void     prepare(shared_ptr<typename Problem<G>::Population> pop) const {}

	int evaluations() { return _evaluations; }

protected:
	shared_ptr< Problem<G> > _problem;
	mutable boost::detail::atomic_count _evaluations;
};


} // namespace Eugene

