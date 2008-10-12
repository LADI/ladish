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
#include <ostream>
#include <boost/random.hpp>
#include "eugene/Mutation.hpp"

namespace Eugene {


template <typename G>
class ESMutation : public Mutation<G> {
public:
	ESMutation()
		: _sigma(1.0)
		, _rng(static_cast<unsigned>(std::time(0)))
		, _norm_dist(0, _sigma)
		, _normal_sampler(new Sampler(_rng, _norm_dist))
	{
	}

	float sigma() { return _sigma; }

	void set_sigma(float sigma)
	{
		_sigma = sigma;
		_norm_dist = boost::normal_distribution<float>(0, _sigma);
		_norm_dist.reset();
		_normal_sampler = boost::shared_ptr<Sampler>(new Sampler(_rng, _norm_dist));
	}

	void mutate(G& g) {
		for (size_t i = 0; i < g.size(); ++i) {
			float delta = (*_normal_sampler.get())();
			g[i] += delta;
		}
	}

private:
	typedef boost::variate_generator<mt19937&, normal_distribution<float> > Sampler;

	float                             _sigma;
	boost::mt19937                    _rng;
	boost::normal_distribution<float> _norm_dist;
	boost::shared_ptr<Sampler>        _normal_sampler;
};


} // namespace Eugene
