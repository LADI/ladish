/* This file is part of FlowCanvas.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * FlowCanvas is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * FlowCanvas is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/shared_ptr.hpp>
#include "Item.h"
#include "FlowCanvas.h"

namespace LibFlowCanvas {


Item::Item(boost::shared_ptr<FlowCanvas> canvas,
     const string&                 name,
     double                        x,
     double                        y,
     uint32_t                      color)
	: Gnome::Canvas::Group(*canvas->root(), x, y)
	, _canvas(canvas)
	, _name(name)
	, _width(1)
	, _height(1)
	, _color(color)
	, _selected(false)
{}


void
Item::set_selected(bool s)
{
	_selected = s;

	if (s)
		signal_selected.emit();
	else
		signal_unselected.emit();
}

} // namespace LibFlowCanvas
