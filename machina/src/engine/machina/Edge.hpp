/* This file is part of Machina.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Machina is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Machina is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef MACHINA_EDGE_HPP
#define MACHINA_EDGE_HPP

#include <list>
#include <boost/utility.hpp>
#include <raul/WeakPtr.h>
#include <raul/SharedPtr.h>
#include <raul/DoubleBuffer.h>
#include "types.hpp"
#include "Action.hpp"

namespace Machina {

class Node;

class Edge : boost::noncopyable {
public:

	Edge(WeakPtr<Node> src, SharedPtr<Node> dst)
		: _probability(1.0f)
		, _src(src)
		, _dst(dst)
	{}

	WeakPtr<Node>   src() { return _src; }
	SharedPtr<Node> dst() { return _dst; }

	void set_src(WeakPtr<Node> src)   { _src = src; }
	void set_dst(SharedPtr<Node> dst) { _dst = dst; }

	inline float probability()            { return _probability.get(); }
	inline void  set_probability(float p) { _probability.set(p); }

private:
	Raul::DoubleBuffer<float> _probability;
	
	WeakPtr<Node>   _src;
	SharedPtr<Node> _dst;
};


} // namespace Machina

#endif // MACHINA_EDGE_HPP
