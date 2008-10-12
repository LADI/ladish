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
#include <vector>
#include <algorithm>
#include <boost/shared_ptr.hpp>

namespace Eugene {


template <typename G>
struct Mutation {
	virtual void mutate(G& g) = 0;
};


template <typename G>
class RandomMutation : public Mutation<G> {
public:
	RandomMutation(typename G::value_type min, typename G::value_type max)
		: _min(min)
		, _max(max)
	{
	}

	void mutate(G& g) {
		const size_t index = rand() % g.size();
		g[index] = (rand() % (_max - _min + 1)) + _min;
	}

private:
	typename G::value_type _min;
	typename G::value_type _max;
};


template <typename G>
class FlipMutation : public Mutation<G> {
public:
	void mutate(G& g) {
		const size_t index = rand() % g.size();
		g[index] *= -1;
	}
};


template <typename G>
class FlipRangeMutation : public Mutation<G> {
public:
	void mutate(G& g) {
		const size_t rand_1 = rand() % g.size();
		const size_t rand_2 = rand() % g.size();
		const size_t left = std::min(rand_1, rand_2);
		const size_t right = std::max(rand_1, rand_2);

		for (size_t i=left; i <= right; ++i) {
			assert(g[i] == 1 || g[i] == -1);
			g[i] *= -1;
		}
	}
};


template <typename G>
class FlipRandomMutation : public Mutation<G> {
public:
	void mutate(G& g) {
		for (size_t i=0; i < g.size(); ++i) {
			assert(g[i] == 1 || g[i] == -1);
			if ((rand() % (g.size()/2)) == 0)
				g[i] *= -1;
		}
	}
};


template <typename G>
class SwapRangeMutation : public Mutation<G> {
public:
	void mutate(G& g) {
		const size_t index_a = rand() % g.size();
		const size_t index_b = rand() % g.size();
		typename G::value_type temp = g[index_a];
		g[index_a] = g[index_b];
		g[index_b] = temp;
	}
};


template <typename G>
class SwapAlleleMutation : public Mutation<G> {
public:
	void mutate(G& g) {
		const size_t index_a = rand() % g.size();
		const size_t index_b = rand() % g.size();
		typename G::value_type temp = g[index_a];
		g[index_a] = g[index_b];
		g[index_b] = temp;
	}
};


template <typename G>
class ReverseMutation : public Mutation<G> {
public:
	void mutate(G& g) {
		const size_t index_a = rand() % g.size();
		const size_t index_b = rand() % g.size();
		const size_t left    = std::min(index_a, index_b);
		const size_t right   = std::max(index_a, index_b);

		for (size_t i = 0; i < right - left; ++i) {
			typename G::value_type temp = g[left + i];
			g[left + i] = g[right - i];
			g[right - i] = temp;
		}
	}
};


template <typename G>
class PermuteMutation : public Mutation<G> {
public:
	void mutate(G& g) {
		std::random_shuffle(g.begin(), g.end());
	}
};



template <typename G>
class NullMutation : public Mutation<G> {
public:
	inline static void mutate(G& g) {}
};


} // namespace Eugene
