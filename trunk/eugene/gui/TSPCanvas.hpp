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
#include <string>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <flowcanvas/flowcanvas.hpp>
#include <eugene/TSP.hpp>

namespace Eugene {

class GA;

namespace GUI {


class TSPCanvas : public FlowCanvas::Canvas {
public:
	static boost::shared_ptr<TSPCanvas> load(boost::shared_ptr<TSP> tsp);
	
	void update(boost::shared_ptr<const TSP::GeneType> best);
	
private:
	TSPCanvas(double width, double height) : FlowCanvas::Canvas(width, height) {}

	std::vector< boost::shared_ptr<FlowCanvas::Ellipse> >    _nodes;
	std::vector< boost::shared_ptr<FlowCanvas::Connection> > _edges;
};


} // namespace GUI
} // namespace Eugene

