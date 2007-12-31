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
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <algorithm>
#include <cassert>
#include <cmath>
#include <libgnomecanvasmm.h>
#include <flowcanvas/Connection.hpp>
#include <flowcanvas/Canvas.hpp>
#include <flowcanvas/Connectable.hpp>
#include <flowcanvas/Ellipse.hpp>

namespace FlowCanvas {
	

Connection::Connection(boost::shared_ptr<Canvas>      canvas,
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
  _handle_style(HANDLE_NONE),
  _handle(NULL)
{
	_bpath.property_width_units() = 1.0;
	set_color(color);

	update_location();	
	lower_to_bottom();
	raise(1); // raise above base rect
}


void
Connection::set_color(uint32_t color)
{
	_color = color;
	_bpath.property_outline_color_rgba() = _color;
	if (_handle) {
		if (_handle->text) {
			_handle->text->property_fill_color_rgba() = _color;
		}
		if (_handle->shape) {
			_handle->shape->property_fill_color_rgba() = 0x000000FF;
			_handle->shape->property_outline_color_rgba() = _color;
		}
	}
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
		
		if (_handle) {
			_handle->property_x() = src_x - dx/2.0;
			_handle->property_y() = src_y - dy/2.0;
			_handle->move(0, 0);
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

		double dx = fabs(dst_x - src_x);
		double dy = fabs(dst_y - src_y);

		// Path 1 (src_x, src_y) -> (join_x, join_y)
		// Control point 1
		const double src_x1 = src_x + (dx+dy)/5.0;//std::min(x_dist+y_dist, 40.0);
		const double src_y1 = src_y;
		// Control point 2
		const double src_x2 = (join_x + src_x1) / 2.0;
		const double src_y2 = (join_y + src_y1) / 2.0;

		// Path 2, (join_x, join_y) -> (dst_x, dst_y)
		// Control point 1
		const double dst_x1 = dst_x - (dx + dy)/5.0;//std::min(x_dist+y_dist, 40.0);
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
		/*gnome_canvas_path_def_reset(_path);
		gnome_canvas_path_def_moveto(_path, src_x, src_y);
		gnome_canvas_path_def_lineto(_path, src_x1, src_y1);
		gnome_canvas_path_def_lineto(_path, src_x2, src_y2);
		gnome_canvas_path_def_lineto(_path, join_x, join_y);
		gnome_canvas_path_def_lineto(_path, dst_x2, dst_y2);
		gnome_canvas_path_def_lineto(_path, dst_x1, dst_y1);
		gnome_canvas_path_def_lineto(_path, dst_x, dst_y);
		*/
		
		if (_show_arrowhead) {

			const double h  = sqrt(dx*dx + dy*dy);

			dx = dx / h * 10;
			dy = dy / h * 10;
			
			gnome_canvas_path_def_lineto(_path,
					dst_x - 12,
					dst_y - 4);

			gnome_canvas_path_def_moveto(_path, dst_x, dst_y);

			gnome_canvas_path_def_lineto(_path,
					dst_x - 12,
					dst_y + 4);
		}
	}

	GnomeCanvasBpath* c_obj = _bpath.gobj();
	gnome_canvas_item_set(GNOME_CANVAS_ITEM(c_obj), "bpath", _path, NULL);
}


/** Set label text displayed next to the edge.
 *
 * Passing the empty string will remove the label.
 */
void
Connection::set_label(const std::string& str)
{
	if (str != "") {
		if (!_handle)
			_handle = new Handle(*this);

		if (!_handle->text) {
			_handle->text = new Gnome::Canvas::Text(*_handle, 0, 0, str);
			_handle->text->property_size_set() = true;
			_handle->text->property_size() = 9000;
			_handle->text->property_weight_set() = true;
			_handle->text->property_weight() = 200;
			_handle->text->property_fill_color_rgba() = _color;
			_handle->text->show();
		} else {
			_handle->text->property_text() = str;
		}

		if (_handle->shape)
			show_handle(true);

		_handle->text->raise(1);
		update_location();
	} else if (_handle) {
		delete _handle->text;
		_handle->text = NULL;
	}
}


void
Connection::show_handle(bool show)
{
	if (show) {
		if (!_handle)
			_handle = new Handle(*this);
		
		double handle_width = 8.0;
		double handle_height = 8.0;
		if (_handle->text) {
			handle_width = _handle->text->property_text_width();
			handle_height = _handle->text->property_text_height();
		}

		// FIXME: slow
		delete _handle->shape;
		
		if (_handle_style != HANDLE_NONE) {
			if (_handle_style == HANDLE_RECT)
				_handle->shape = new Gnome::Canvas::Rect(*_handle,
						-handle_width/2.0 - 1.0, -handle_height/2.0,
						handle_width/2.0 + 1.0, handle_height/2.0);
			else// if (_handle_style == HANDLE_CIRCLE)
				_handle->shape = new Gnome::Canvas::Ellipse(*_handle,
						-handle_width/2.0 - 1.0, -handle_height/2.0,
						handle_width/2.0 + 1.0, handle_height/2.0);
		}

		_handle->shape->property_fill_color_rgba() = 0x000000FF;
		_handle->shape->property_outline_color_rgba() = _color;
		_handle->shape->show();
		_handle->show();

	} else {
		delete _handle;
		_handle = NULL;
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
	if (_handle && _handle->text) {
		_handle->text->property_size() = static_cast<int>(floor((double)9000.0f * z));
	}
}


} // namespace FlowCanvas

