/* This file is part of FlowCanvas.
 * Copyright (C) 2007 Dave Robillard <drobilla.net>
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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <libgnomecanvasmm.h>
#include "Connection.h"
#include "FlowCanvas.h"
#include "Connectable.h"

namespace LibFlowCanvas {
	

Connection::Connection(boost::shared_ptr<FlowCanvas>  canvas,
	                   boost::shared_ptr<Connectable> source,
	                   boost::shared_ptr<Connectable> dest,
                       uint32_t                       color)
: Gnome::Canvas::Bpath(*canvas->root()),
  _canvas(canvas),
  _source(source),
  _dest(dest),
  _color(color),
  _selected(false),
  _path(gnome_canvas_path_def_new())
{
	if (canvas->property_aa())
		property_width_units() = 0.75;
	else
		property_width_units() = 1.0;

	property_outline_color_rgba() = _color;
	property_cap_style() = (Gdk::CapStyle)GDK_CAP_ROUND;

	update_location();	
}


/** Updates the path of the connection to match it's ports if they've moved.
 */
void
Connection::update_location()
{
	boost::shared_ptr<Connectable> src = _source.lock();
	boost::shared_ptr<Connectable> dst = _dest.lock();
	
	if (!src || !dst)
		return;

	const double src_x = src->connection_point().get_x();
	const double src_y = src->connection_point().get_y();
	const double dst_x = dst->connection_point().get_x();
	const double dst_y = dst->connection_point().get_y();

	const double join_x = (src_x + dst_x)/2.0;
	const double join_y = (src_y + dst_y)/2.0;

	const double x_dist = fabs(dst_x - src_x)/4.0;

	// Path 1 (src_x, src_y) -> (join_x, join_y)
	// Control point 1
	const double src_x1 = src_x + std::max(x_dist, 40.0);
	const double src_y1 = src_y;
	// Control point 2
	const double src_x2 = (join_x + src_x1) / 2.0;
	const double src_y2 = (join_y + src_y1) / 2.0;

	// Path 2, (join_x, join_y) -> (dst_x, dst_y)
	// Control point 1
	const double dst_x1 = dst_x - std::max(x_dist, 40.0);
	const double dst_y1 = dst_y;
	// Control point 2
	const double dst_x2 = (join_x + dst_x1) / 2.0;
	const double dst_y2 = (join_y + dst_y1) / 2.0;

	// This was broken in libgnomecanvasmm with GTK 2.8.  Nice work, guys.  
	/*
	_path->reset();
	_path->moveto(src_x, src_y);
	_path->curveto(src_x1, src_y1, src_x2, src_y2, join_x, join_y);
	_path->curveto(dst_x2, dst_y2, dst_x1, dst_y1, dst_x, dst_y);
	set_bpath(_path);
	*/

	// Work around it w/ the C API
	gnome_canvas_path_def_reset(_path);
	gnome_canvas_path_def_moveto(_path, src_x, src_y);
	gnome_canvas_path_def_curveto(_path, src_x1, src_y1, src_x2, src_y2, join_x, join_y);
	gnome_canvas_path_def_curveto(_path, dst_x2, dst_y2, dst_x1, dst_y1, dst_x, dst_y);
	
	// Uncomment to see control point path as straight lines
	/*
	gnome_canvas_path_def_reset(_path);
	gnome_canvas_path_def_moveto(_path, src_x, src_y);
	gnome_canvas_path_def_lineto(_path, src_x1, src_y1);
	gnome_canvas_path_def_lineto(_path, src_x2, src_y2);
	gnome_canvas_path_def_lineto(_path, join_x, join_y);
	gnome_canvas_path_def_lineto(_path, dst_x2, dst_y2);
	gnome_canvas_path_def_lineto(_path, dst_x1, dst_y1);
	gnome_canvas_path_def_lineto(_path, dst_x, dst_y);
	*/

	GnomeCanvasBpath* c_obj = gobj();
	gnome_canvas_item_set(GNOME_CANVAS_ITEM(c_obj), "bpath", _path, NULL);
}


void
Connection::set_highlighted(bool b)
{
	if (b)
		property_outline_color_rgba() = 0xFF0000FF;
	else
		property_outline_color_rgba() = _color;
}


void
Connection::set_selected(bool selected)
{
	_selected = selected;

	if (selected) {
		property_dash() = _canvas.lock()->select_dash();
	} else {
		property_dash() = NULL;
	}
}


} // namespace LibFlowCanvas

