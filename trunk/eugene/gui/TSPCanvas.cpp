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

#include <flowcanvas/flowcanvas.hpp>
#include <eugene/GAImpl.hpp>
#include <eugene/Problem.hpp>
#include <eugene/TSP.hpp>
#include <eugene/Selection.hpp>
#include <eugene/Mutation.hpp>
#include <eugene/Crossover.hpp>
#include "TSPCanvas.hpp"
	
using namespace FlowCanvas;

namespace Eugene {
namespace GUI {
	
typedef GAImpl<TSP::GeneType> TSPGA;


boost::shared_ptr<TSPCanvas>
TSPCanvas::load(boost::shared_ptr<TSP> tsp)
{
	boost::shared_ptr<TSPCanvas> canvas(new TSPCanvas(2000, 2000));

	size_t city_count = 0;
	canvas->_nodes.resize(tsp->cities().size());

	for (TSP::Cities::const_iterator i = tsp->cities().begin(); i != tsp->cities().end(); ++i) {
		boost::shared_ptr<Ellipse> ellipse(new Ellipse(canvas, "", i->first+8, i->second+8, 6, 6, false));
		ellipse->set_border_color(0xFFFFFFFF);
		ellipse->set_base_color(0x333333FF);
		canvas->add_item(ellipse);
		ellipse->show();
		canvas->_nodes[city_count++] = ellipse;
	}

	return canvas;
}
		

void
TSPCanvas::update(boost::shared_ptr<const TSP::GeneType> best)
{
	_edges.resize(best->size(), boost::shared_ptr<Connection>());

	for (size_t i=0; i < best->size(); ++i) {
		assert(best->at(i) < _nodes.size());
		assert(best->at((i+1) % best->size()) < _nodes.size());
		assert(i < _edges.size());

		_edges[i].reset();

		const size_t c1 = (*best)[i];
		const size_t c2 = (*best)[(i+1) % best->size()];

		_edges[i] = boost::shared_ptr<Connection>(new Connection(shared_from_this(),
					_nodes[c1], _nodes[c2], 0xFFFFFFAA, false));

		_edges[i]->show();
	}
}


} // namespace GUI
} // namespace Eugene

