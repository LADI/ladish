/* This file is part of FlowCanvas.  Copyright (C) 2005 Dave Robillard.
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
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Connection.h"
#include <algorithm>
#include <cassert>
#include <math.h>
#include <libgnomecanvasmm.h>
#include "FlowCanvas.h"

namespace LibFlowCanvas {
	

Connection::Connection(boost::shared_ptr<FlowCanvas> canvas,
	                   boost::shared_ptr<Port>       source,
	                   boost::shared_ptr<Port>       dest)
: Gnome::Canvas::Bpath(*canvas->root()),
  m_canvas(canvas),
  m_source(source),
  m_dest(dest),
  m_selected(false),
  m_path(gnome_canvas_path_def_new())
{
	assert(source->is_output());
	assert(dest->is_input());
	
	m_color = source->color() + 0x22222200;
	if (canvas->property_aa())
		property_width_units() = 0.75;
	else
		property_width_units() = 1.0;

	property_outline_color_rgba() = m_color;
	property_cap_style() = (Gdk::CapStyle)GDK_CAP_ROUND;

	update_location();	
}


/** Updates the path of the connection to match it's ports if they've moved.
 */
void
Connection::update_location()
{
	boost::shared_ptr<Port> src = m_source.lock();
	boost::shared_ptr<Port> dst = m_dest.lock();
	
	if (!src || !dst)
		return;

	boost::shared_ptr<Module> src_mod = src->module().lock();
	boost::shared_ptr<Module> dst_mod = dst->module().lock();

	if (!src_mod || !dst_mod)
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
	m_path->reset();
	m_path->moveto(src_x, src_y);
	m_path->curveto(src_x1, src_y1, src_x2, src_y2, join_x, join_y);
	m_path->curveto(dst_x2, dst_y2, dst_x1, dst_y1, dst_x, dst_y);
	set_bpath(m_path);
	*/

	// Work around it w/ the C API
	gnome_canvas_path_def_reset(m_path);
	gnome_canvas_path_def_moveto(m_path, src_x, src_y);
	gnome_canvas_path_def_curveto(m_path, src_x1, src_y1, src_x2, src_y2, join_x, join_y);
	gnome_canvas_path_def_curveto(m_path, dst_x2, dst_y2, dst_x1, dst_y1, dst_x, dst_y);
	
	// Uncomment to see control point path as straight lines
	/*
	gnome_canvas_path_def_reset(m_path);
	gnome_canvas_path_def_moveto(m_path, src_x, src_y);
	gnome_canvas_path_def_lineto(m_path, src_x1, src_y1);
	gnome_canvas_path_def_lineto(m_path, src_x2, src_y2);
	gnome_canvas_path_def_lineto(m_path, join_x, join_y);
	gnome_canvas_path_def_lineto(m_path, dst_x2, dst_y2);
	gnome_canvas_path_def_lineto(m_path, dst_x1, dst_y1);
	gnome_canvas_path_def_lineto(m_path, dst_x, dst_y);
	*/

	GnomeCanvasBpath* c_obj = gobj();
	gnome_canvas_item_set(GNOME_CANVAS_ITEM(c_obj), "bpath", m_path, NULL);
}


void
Connection::set_highlighted(bool b)
{
	if (b)
		property_outline_color_rgba() = 0xFF0000FF;
	else
		property_outline_color_rgba() = m_color;
}


void
Connection::set_selected(bool selected)
{
	m_selected = selected;

	if (selected) {
		property_dash() = m_canvas.lock()->select_dash();
	} else {
		property_dash() = NULL;
	}
}


} // namespace LibFlowCanvas

