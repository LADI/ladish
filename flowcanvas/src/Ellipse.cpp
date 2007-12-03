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

#include <functional>
#include <list>
#include <algorithm>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <flowcanvas/Item.hpp>
#include <flowcanvas/Ellipse.hpp>
#include <flowcanvas/Canvas.hpp>
using std::string;

namespace FlowCanvas {

static const uint32_t ELLIPSE_FILL_COLOUR           = 0x131415FF;
static const uint32_t ELLIPSE_HILITE_FILL_COLOUR    = 0x252627FF;
static const uint32_t ELLIPSE_OUTLINE_COLOUR        = 0xBBBBBBFF;
static const uint32_t ELLIPSE_HILITE_OUTLINE_COLOUR = 0xFFFFFFFF;
static const uint32_t ELLIPSE_TITLE_COLOUR          = 0xFFFFFFFF;


/** Construct a Ellipse
 *
 * Note you must call resize() at some point or the module will look ridiculous.
 * This it to avoid unecessary text measuring and resizing, which is insanely
 * expensive.
 *
 * If @a name is the empty string, the space where the title would usually be
 * is not created (eg the module will be shorter).
 */
Ellipse::Ellipse(boost::shared_ptr<Canvas> canvas,
                 const string&             name,
                 double                    x,
                 double                    y,
                 double                    x_radius,
                 double                    y_radius,
                 bool                      show_title)
	: Item(canvas, name, x, y, ELLIPSE_FILL_COLOUR)
	, _title_visible(show_title)
	, _ellipse(*this, -x_radius, -y_radius, x_radius, y_radius)
	, _label(NULL)
{
	if (name != "")
		_label = Gtk::manage(new Gnome::Canvas::Text(*this, 0, 0, name));

	_ellipse.property_fill_color_rgba() = ELLIPSE_FILL_COLOUR;
	_ellipse.property_outline_color_rgba() = ELLIPSE_OUTLINE_COLOUR;
	_border_color = ELLIPSE_OUTLINE_COLOUR;
	
	if (canvas->property_aa())
		set_border_width(0.5);
	else
		set_border_width(1.0);

	if (_label) {
		if (show_title) {
			_label->property_size_set() = true;
			_label->property_size() = 9000;
			_label->property_weight_set() = true;
			_label->property_weight() = 400;
			_label->property_fill_color_rgba() = ELLIPSE_TITLE_COLOUR;
		} else {
			_label->hide();
		}
	}

	set_width(x_radius * 2.0);
	set_height(y_radius * 2.0);
}


Ellipse::~Ellipse()
{
}


Gnome::Art::Point
Ellipse::dst_connection_point(const Gnome::Art::Point& src)
{
	const double c_x   = property_x();
	const double c_y   = property_y();
	const double src_x = src.get_x();
	const double src_y = src.get_y();

	const double dx = src.get_x() - c_x;
	const double dy = src.get_y() - c_y;
	const double h  = sqrt(dx*dx + dy*dy);

	const double theta = asin(dx/(h + DBL_EPSILON));

	const double y_mod = (c_y < src_y) ? 1 : -1;

	const double ret_h = h - _width/2.0;
	const double ret_x = src_x - sin(theta) * ret_h;
	const double ret_y = src_y - cos(theta) * ret_h * y_mod;

	return Gnome::Art::Point(ret_x, ret_y);
}


/** Set the border width of the module.
 *
 * Do NOT directly set the width_units property on the rect, use this function.
 */
void
Ellipse::set_border_width(double w)
{
	_border_width = w;
	_ellipse.property_width_units() = w;
}


void
Ellipse::zoom(double z)
{
	if (_label)
		_label->property_size() = static_cast<int>(floor((double)9000.0f * z));
}


void
Ellipse::set_highlighted(bool b)
{
	if (b) {
		_ellipse.property_fill_color_rgba() = ELLIPSE_HILITE_FILL_COLOUR;
		_ellipse.property_outline_color_rgba() = ELLIPSE_HILITE_OUTLINE_COLOUR;
	} else {
		_ellipse.property_fill_color_rgba() = _color;
		_ellipse.property_outline_color_rgba() = ELLIPSE_OUTLINE_COLOUR;
	}
}


void
Ellipse::set_selected(bool selected)
{
	Item::set_selected(selected);
	assert(_selected == selected);

	boost::shared_ptr<Canvas> canvas = _canvas.lock();
	if (!canvas)
		return;
		
	if (selected) {
		_ellipse.property_fill_color_rgba() = ELLIPSE_HILITE_FILL_COLOUR;
		_ellipse.property_outline_color_rgba() = ELLIPSE_HILITE_OUTLINE_COLOUR;
		_ellipse.property_dash() = canvas->select_dash();
	} else {
		_ellipse.property_fill_color_rgba() = _color;
		_ellipse.property_outline_color_rgba() = ELLIPSE_OUTLINE_COLOUR;
		_ellipse.property_dash() = NULL;
	}
}


bool
Ellipse::is_within(const Gnome::Canvas::Rect& rect)
{
	const double x1 = rect.property_x1();
	const double y1 = rect.property_y1();
	const double x2 = rect.property_x2();
	const double y2 = rect.property_y2();

	if (x1 < x2 && y1 < y2) {
		return (property_x() > x1
			&& property_y() > y1
			&& property_x() + width() < x2
			&& property_y() + height() < y2);
	} else if (x2 < x1 && y2 < y1) {
		return (property_x() > x2
			&& property_y() > y2
			&& property_x() + width() < x1
			&& property_y() + height() < y1);
	} else if (x1 < x2 && y2 < y1) {
		return (property_x() > x1
			&& property_y() > y2
			&& property_x() + width() < x2
			&& property_y() + height() < y1);
	} else if (x2 < x1 && y1 < y2) {
		return (property_x() > x2
			&& property_y() > y1
			&& property_x() + width() < x1
			&& property_y() + height() < y2);
	} else {
		return false;
	}
}


void
Ellipse::set_width(double w)
{
	_width = w;
//	_ellipse.property_x2() = _ellipse.property_x1() + w;
}


void
Ellipse::set_height(double h)
{
	_height = h;
//	_ellipse.property_y2() = _ellipse.property_y1() + h;
}


/** Move relative to current location.
 *
 * @param dx distance to move along x axis (in world units)
 * @param dy distance to move along y axis (in world units)
 */
void
Ellipse::move(double dx, double dy)
{
	boost::shared_ptr<Canvas> canvas = _canvas.lock();
	if (!canvas)
		return;

	double new_x = property_x() + dx;
	double new_y = property_y() + dy;
	
	if (new_x < 0)
		dx = property_x() * -1;
	else if (new_x + _width > canvas->width())
		dx = canvas->width() - property_x() - _width;
	
	if (new_y < 0)
		dy = property_y() * -1;
	else if (new_y + _height > canvas->height())
		dy = canvas->height() - property_y() - _height;

	Gnome::Canvas::Group::move(dx, dy);

	move_connections();
}


/** Move to the specified absolute coordinate on the canvas.
 *
 * @param x x coordinate to move to (in world units)
 * @param y y coordinate to move to (in world units)
 */
void
Ellipse::move_to(double x, double y)
{
	boost::shared_ptr<Canvas> canvas = _canvas.lock();
	if (!canvas)
		return;

	assert(canvas->width() > 0);
	assert(canvas->height() > 0);

	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x + _width > canvas->width()) x = canvas->width() - _width - 1;
	if (y + _height > canvas->height()) y = canvas->height() - _height - 1;
		
	assert(x >= 0);
	assert(x + _width < canvas->width());
	assert(y >= 0);
	assert(y + _height < canvas->height());

	property_x() = x;
	property_y() = y;
	Gnome::Canvas::Group::move(0, 0);

	move_connections();
}


void
Ellipse::set_name(const string& str)
{
	if (str != "") {
		if (!_label)
			_label = new Gnome::Canvas::Text(*this, 0, 0, str);

		_label->property_size_set() = true;
		_label->property_size() = 9000;
		_label->property_weight_set() = true;
		_label->property_weight() = 200;
		_label->property_fill_color_rgba() = ELLIPSE_TITLE_COLOUR;
		_label->property_text() = str;
		_label->show();
	} else {
		delete _label;
		_label = NULL;
	}
}


/** Resize the module to fit its contents best.
 */
void
Ellipse::resize()
{
#if 0
	double widest_in = 0.0;
	double widest_out = 0.0;
	
	// The amount of space between a port edge and the module edge (on the
	// side that the port isn't right on the edge).
	double hor_pad = 5.0;
	if (!_title_visible)
		hor_pad = 15.0; // leave more room for something to grab for dragging

	boost::shared_ptr<Port> p;
	
	// Find widest in/out ports
	for (PortVector::iterator i = _ports.begin(); i != _ports.end(); ++i) {
		p = (*i);
		assert(p);
		if (p->is_input() && p->width() > widest_in)
			widest_in = p->width();
		else if (p->is_output() && p->width() > widest_out)
			widest_out = p->width();
	}
	
	// Make sure module is wide enough for ports
	set_width(std::max(widest_in, widest_out) + hor_pad + border_width()*2.0);
	
	// Make sure module is wide enough for title
	if (_title_visible && _label.property_text_width() + 8.0 > _width)
		set_width(_label.property_text_width() + 8.0);

	// Set height to contain ports and title
	double height_base = 2;
	if (_title_visible)
		height_base += 2 + _label.property_text_height();

	double h = height_base;
	if (_ports.size() > 0)
		h += _ports.size() * ((*_ports.begin())->height()+2.0);

	if (!_title_visible && _ports.size() > 0)
		h += 0.5;

	set_height(h);
	
	// Move ports to appropriate locations
	
	double y;
	int i = 0;
	for (PortVector::iterator pi = _ports.begin(); pi != _ports.end(); ++pi, ++i) {
		boost::shared_ptr<Port> p = (*pi);

		y = height_base + (i * (p->height() + 2.0));
		if (p->is_input()) {
			p->set_width(widest_in);
			p->property_x() = 0.0;
			p->property_y() = y;
		} else {
			p->set_width(widest_out);
			p->property_x() = _width - p->width() - 0.0;
			p->property_y() = y;
		}
	}

	// Center title
	_label.property_x() = _width/2.0;

	// Update connection locations if we've moved/resized
	for (PortVector::iterator pi = _ports.begin(); pi != _ports.end(); ++pi, ++i) {
		(*pi)->move_connections();
	}
	#endif

	// Make things actually move to their new locations (?!)
	move(0, 0);
}


void
Ellipse::select_tick()
{
	_ellipse.property_dash() = _canvas.lock()->select_dash();
}


void
Ellipse::add_connection(boost::shared_ptr<Connection> c)
{
	Connectable::add_connection(c);
	raise_to_top();
}

	
void
Ellipse::set_border_color(uint32_t c)
{
	_border_color = c;
	_ellipse.property_outline_color_rgba() = _border_color;
}


void
Ellipse::set_default_base_color()
{
	_color = ELLIPSE_FILL_COLOUR;
	_ellipse.property_fill_color_rgba() = _color;
}

void
Ellipse::set_base_color(uint32_t c)
{
	_color = c;
	_ellipse.property_fill_color_rgba() = _color;
}


} // namespace FlowCanvas
