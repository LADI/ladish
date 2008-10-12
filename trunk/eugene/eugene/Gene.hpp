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
#include <cassert>
#include <vector>
#include <ostream>

namespace Eugene {


struct Gene
{
};


template <typename A>
struct GeneImpl : public std::vector<A>, public Gene
{
	/** Don't reserve or initialise */
	//GeneImpl() : std::vector<A>() {}
	
	/** Reserve, don't initialise */
	GeneImpl(size_t gene_size) : std::vector<A>(gene_size, 12345) {}

	/** Initialise randomly in the given range (inclusive) */
	GeneImpl(size_t size, A min, A max)
		: std::vector<A>(size, min)
	{
		for (size_t i=0; i < size; ++i)
			(*this)[i] = rand() % (max - min + 1) + min;
	}
	
	inline bool operator==(const GeneImpl<A>& other) const {
		for (size_t i=0; i < this->size(); ++i)
			if ((*this)[i] != other[i])
				return false;

		return true;
	}
	
	inline bool operator<(const GeneImpl<A>& other) const {
		for (size_t i=0; i < this->size(); ++i)
			if ((*this)[i] < other[i])
				return true;

		return false;
	}
};


} // namespace Eugene
