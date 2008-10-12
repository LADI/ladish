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
class InjectionCrossover : public Crossover<G> {
public:
	std::pair<G,G> crossover(const G& parent_1, const G& parent_2) {
		cerr << "FIXME: Injection crossover" << endl;
		return make_pair(parent_1, parent_2);
	}

private:

};


} // namespace Eugene

