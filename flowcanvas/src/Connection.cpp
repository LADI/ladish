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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <libgnomecanvasmm.h>
#include "Connection.h"
#include "FlowCanvas.h"
#include "Connectable.h"
#include "Ellipse.h"

namespace LibFlowCanvas {
	

Connection::Connection(boost::shared_ptr<FlowCanvas>  canvas,
	                   boost::shared_ptr<Connectable> source,
	                   boost::shared_ptr<Connectable> dest,
                       uint32_t                       color,
                       bool                           show_arrowhead)
: Gnome::Canvas::Group(*canvas->root()),
  _canvas(canvas),
  _source(source),
  _dest(dest),
  _color(color),
  _selected(false),
  _flag(false),
  _show_arrowhead(show_arrowhead),
  _bpath(*this),
  _path(gnome_canvas_path_def_new()),
  _label(NULL)
{
	if (canvas->property_aa())
		_bpath.property_width_units() = 0.75;
	else
		_bpath.property_width_units() = 1.0;

	_bpath.property_outline_color_rgba() = _color;
	_bpath.property_cap_style() = (Gdk::CapStyle)GDK_CAP_ROUND;

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

	bool straight = (boost::dynamic_pointer_cast<Ellipse>(src)
	              || boost::dynamic_pointer_cast<Ellipse>(dst));

	const Gnome::Art::Point src_point = src->src_connection_point();
	const Gnome::Art::Point dst_point = dst->dst_connection_point(src_point);

	const double src_x = src_point.get_x();
	const double src_y = src_point.get_y();
	const double dst_x = dst_point.get_x();
	const double dst_y = dst_point.get_y();

	if (straight) {

		gnome_canvas_path_def_reset(_path);
		gnome_canvas_path_def_moveto(_path, src_x, src_y);
		gnome_canvas_path_def_lineto(_path, dst_x, dst_y);
		double dx = src_x - dst_x;
		double dy = src_y - dst_y;	
		
		if (_label) {
			_label->property_x() = src_x - dx/2.0;
			_label->property_y() = src_y - dy/2.0;
		}
		
		if (_show_arrowhead) {

			const double h  = sqrt(dx*dx + dy*dy);

			dx = dx / h * 10;
			dy = dy / h * 10;

			gnome_canvas_path_def_lineto(_path,
					dst_x + dx - dy/1.5,
					dst_y + dy + dx/1.5);

			gnome_canvas_path_def_moveto(_path, dst_x, dst_y);

			gnome_canvas_path_def_lineto(_path,
					dst_x + dx + dy/1.5,
					dst_y + dy - dx/1.5);
		}

	} else {

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

		// libgnomecanvasmm + GTK 2.8 screwed up the Path API; use the C one.
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
	}

	GnomeCanvasBpath* c_obj = _bpath.gobj();
	gnome_canvas_item_set(GNOME_CANVAS_ITEM(c_obj), "bpath", _path, NULL);
}


/** Set label text displayed next to the edge.
 *
 * Passing the empty string will remove the label.
 */
void
Connection::set_label(const string& str)
{
	if (str != "") {
		if (!_label) {
			_label = new Gnome::Canvas::Text(*this, 50, 50, str);
			_label->property_size_set() = true;
			_label->property_size() = 9000;
			_label->property_weight_set() = true;
			_label->property_weight() = 200;
			_label->property_fill_color_rgba() = _color;
		} else {
			_label->property_text() = str;
		}
		_label->show();
		update_location();
	} else {
		delete _label;
		_label = NULL;
	}
}


void
Connection::set_highlighted(bool b)
{
	if (b)
		_bpath.property_outline_color_rgba() = 0xFF0000FF;
	else
		_bpath.property_outline_color_rgba() = _color;
}


void
Connection::set_selected(bool selected)
{
	_selected = selected;

	if (selected) {
		_bpath.property_dash() = _canvas.lock()->select_dash();
	} else {
		_bpath.property_dash() = NULL;
	}
}


/** Overloaded Gnome::Canvas::Item::raise_to_top to ensure src and dst
 * are still above connections (to hide the part behind connected Ellipses).
 */
void
Connection::raise_to_top()
{
	Gnome::Canvas::Item::raise_to_top();
	
	// Raise source above us
	boost::shared_ptr<Item> item = boost::dynamic_pointer_cast<Item>(_source.lock());
	if (item)
		item->raise_to_top();

	// Raise dest above us
	item = boost::dynamic_pointer_cast<Item>(_dest.lock());
	if (item)
		item->raise_to_top();

	/* Raise the roof
	       \o/ 
	        |
	       / \ 
	*/
}


void
Connection::select_tick()
{
	_bpath.property_dash() = _canvas.lock()->select_dash();
}

	
void
Connection::zoom(double z)
{
	if (_label) {
		_label->property_size() = static_cast<int>(floor((double)9000.0f * z));
	}
}


} // namespace LibFlowCanvas

