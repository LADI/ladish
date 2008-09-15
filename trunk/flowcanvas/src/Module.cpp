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
#include <flowcanvas/Module.hpp>
#include <flowcanvas/Canvas.hpp>

using namespace std;

namespace FlowCanvas {

static const uint32_t MODULE_FILL_COLOUR           = 0x1E2224FF;
static const uint32_t MODULE_HILITE_FILL_COLOUR    = 0x2E3436FF;
static const uint32_t MODULE_OUTLINE_COLOUR        = 0x93978FFF;
static const uint32_t MODULE_HILITE_OUTLINE_COLOUR = 0xEEEEECFF;
static const uint32_t MODULE_TITLE_COLOUR          = 0xFFFFFFFF;


/** Construct a Module
 *
 * Note you must call resize() at some point or the module will look ridiculous.
 * This it to avoid unecessary text measuring and resizing, which is insanely
 * expensive.
 *
 * If @a name is the empty string, the space where the title would usually be
 * is not created (eg the module will be shorter).
 */
Module::Module(boost::shared_ptr<Canvas> canvas, const string& name, double x, double y, bool show_title)
	: Item(canvas, name, x, y, MODULE_FILL_COLOUR)
	, _border_width(1.0)
	, _title_visible(show_title)
	, _embed_width(0)
	, _embed_height(0)
	, _icon_size(16)
	, _widest_input(0)
	, _widest_output(0)
	, _module_box(*this, 0, 0, 0, 0) // w, h set later
	, _canvas_title(*this, 0, 8, name) // x set later
	, _stacked_border(NULL)
	, _icon_box(NULL)
	, _embed_container(NULL)
	, _embed_item(NULL)
{
	_module_box.property_fill_color_rgba() = MODULE_FILL_COLOUR;
	_module_box.property_outline_color_rgba() = MODULE_OUTLINE_COLOUR;
	_module_box.property_width_units() = _border_width;

	_border_color = MODULE_OUTLINE_COLOUR;

	if (show_title) {
		/* WARNING: Doing this makes things extremely slow!
		_canvas_title.property_size_set() = true;
		_canvas_title.property_size() = 9000;
		_canvas_title.property_weight_set() = true;
		_canvas_title.property_weight() = 400; */
		if (canvas->get_zoom() != 1.0)
			zoom(canvas->get_zoom());
		_canvas_title.property_fill_color_rgba() = MODULE_TITLE_COLOUR;
	} else {
		_canvas_title.hide();
	}

	set_width(10.0);
	set_height(10.0);

	signal_pointer_entered.connect(sigc::bind(sigc::mem_fun(this,
		&Module::set_highlighted), true));
	signal_pointer_exited.connect(sigc::bind(sigc::mem_fun(this,
		&Module::set_highlighted), false));
	signal_dropped.connect(sigc::mem_fun(this,
		&Module::on_drop));
}


Module::~Module()
{
	delete _stacked_border;
	delete _icon_box;
}


bool
Module::on_event(GdkEvent* event)
{
	if (event->type == GDK_KEY_PRESS || event->type == GDK_KEY_RELEASE) {
		boost::shared_ptr<Canvas> canvas = _canvas.lock();
		if (canvas)
			canvas->canvas_event(event);
	}

	return Item::on_event(event);
}


/** Set the border width of the module.
 *
 * Do NOT directly set the width_units property on the rect, use this function.
 */
void
Module::set_border_width(double w)
{
	_border_width = w;
	_module_box.property_width_units() = w;
	if (_stacked_border)
		_stacked_border->property_width_units() = w;
}


void
Module::set_stacked_border(bool b)
{
	if (b && !_stacked_border) {
		_stacked_border = new Gnome::Canvas::Rect(*this, 4.0, 4.0, _width + 4.0, _height + 4.0);
		_stacked_border->property_fill_color_rgba() = _color;
		_stacked_border->property_outline_color_rgba() = MODULE_OUTLINE_COLOUR;
		_stacked_border->property_width_units() = _border_width;
		_stacked_border->lower_to_bottom();
		_stacked_border->show();
	} else if (b) {
		_stacked_border->show();
	} else {
		delete _stacked_border;
		_stacked_border = NULL;
	}
}


void
Module::set_icon(const Glib::RefPtr<Gdk::Pixbuf>& icon)
{
	if (_icon_box) {
		delete _icon_box;
		_icon_box = 0;
	}

	if (icon) {
		_icon_box = new Gnome::Canvas::Pixbuf(*this, 8, 10, icon);
		double scale = _icon_size / (icon->get_width() > icon->get_height() ?
				icon->get_width() : icon->get_height());
		_icon_box->affine_relative(Gnome::Art::AffineTrans::scaling(scale));
		_icon_box->show();
	}
	resize();
}


void
Module::on_drop(double new_x, double new_y)
{
	store_location();
}


void
Module::zoom(double z)
{
	_canvas_title.property_size() = static_cast<int>(floor(8000.0f * z));
	for (PortVector::iterator p = _ports.begin(); p != _ports.end(); ++p)
		(*p)->zoom(z);
}


void
Module::set_highlighted(bool b)
{
	if (b) {
		_module_box.property_fill_color_rgba() = MODULE_HILITE_FILL_COLOUR;
		_module_box.property_outline_color_rgba() = MODULE_HILITE_OUTLINE_COLOUR;
	} else {
		_module_box.property_fill_color_rgba() = _color;
		_module_box.property_outline_color_rgba() = MODULE_OUTLINE_COLOUR;
	}
}


void
Module::set_selected(bool selected)
{
	Item::set_selected(selected);
	assert(_selected == selected);

	boost::shared_ptr<Canvas> canvas = _canvas.lock();
	if (!canvas)
		return;

	if (selected) {
		_module_box.property_outline_color_rgba() = MODULE_HILITE_OUTLINE_COLOUR;
		_module_box.property_dash() = canvas->select_dash();
	} else {
		_module_box.property_fill_color_rgba() = _color;
		_module_box.property_outline_color_rgba() = MODULE_OUTLINE_COLOUR;
		_module_box.property_dash() = NULL;
	}
}


/** Get the port on this module at world coordinate @a x @a y.
 */
boost::shared_ptr<Port>
Module::port_at(double x, double y)
{
	x -= property_x();
	y -= property_y();

	for (PortVector::iterator p = _ports.begin(); p != _ports.end(); ++p) {
		boost::shared_ptr<Port> port = *p;
		if (x > port->property_x() && x < port->property_x() + port->width()
				&& y > port->property_y() && y < port->property_y() + port->height()) {
			return port;
		} 
	}

	return boost::shared_ptr<Port>();
}


boost::shared_ptr<Port>
Module::remove_port(const string& name)
{
	boost::shared_ptr<Port> ret = get_port(name);

	if (ret)
		remove_port(ret);
	
	return ret;
}


void
Module::remove_port(boost::shared_ptr<Port> port)
{
	PortVector::iterator i = std::find(_ports.begin(), _ports.end(), port);

	if (i != _ports.end()) {
		_ports.erase(i);
		
		// Find new widest input or output, if necessary
		if (port->is_input() && port->width() >= _widest_input) {
			_widest_input = 0;
			for (PortVector::iterator i = _ports.begin(); i != _ports.end(); ++i) {
				const boost::shared_ptr<Port> p = (*i);
				if (p->is_input() && p->width() >= _widest_input)
					_widest_input = p->width();
			}
		} else if (port->is_output() && port->width() >= _widest_output) {
			_widest_output = 0;
			for (PortVector::iterator i = _ports.begin(); i != _ports.end(); ++i) {
				const boost::shared_ptr<Port> p = (*i);
				if (p->is_output() && p->width() >= _widest_output)
					_widest_output = p->width();
			}
		}

		resize();

	} else {
		std::cerr << "Unable to find port " << port->name() << " to remove." << std::endl;
	}
}


void
Module::set_width(double w)
{
	const bool growing = (w > _width);

	_width = w;
	_module_box.property_x2() = _module_box.property_x1() + w;
	if (_stacked_border)
		_stacked_border->property_x2() = _stacked_border->property_x1() + w;

	if (growing)
		fit_canvas();
}


void
Module::set_height(double h)
{
	const bool growing = (h > _height);

	_height = h;
	_module_box.property_y2() = _module_box.property_y1() + h;
	if (_stacked_border)
		_stacked_border->property_y2() = _stacked_border->property_y1() + h;

	if (growing)
		fit_canvas();
}


/** Move relative to current location.
 *
 * @param dx distance to move along x axis (in world units)
 * @param dy distance to move along y axis (in world units)
 */
void
Module::move(double dx, double dy)
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

	// Deal with moving the connection lines
	for (PortVector::iterator p = _ports.begin(); p != _ports.end(); ++p)
		(*p)->move_connections();
}


/** Move to the specified absolute coordinate on the canvas.
 *
 * @param x x coordinate to move to (in world units)
 * @param y y coordinate to move to (in world units)
 */
void
Module::move_to(double x, double y)
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
	assert(y >= 0);

	if (x + _width >= canvas->width() || y + _height >= canvas->height()) {
		double x1, y1, x2, y2;
		canvas->get_scroll_region(x1, y1, x2, y2);
		canvas->set_scroll_region(x1, y1, max(x2, x + _width), max(y2, y + _height));
	}

	property_x() = x;
	property_y() = y;
	
	// Actually move (stupid gnomecanvas)
	move(0, 0);
	
	// Update any connection line positions
	for (PortVector::iterator p = _ports.begin(); p != _ports.end(); ++p)
		(*p)->move_connections();
}


void
Module::set_name(const string& n)
{
	if (_name != n) {
		string old_name = _name;
		_name = n;
		_canvas_title.property_text() = _name;
		if (_title_visible)
			resize();
	}
}


/** Add a port to this module.
 *
 * A reference to p is held until remove_port is called to remove it.  Note
 * that the module will not be resized (for performance reasons when adding
 * many ports in succession), so you must explicitly call resize() after this
 * for the module to look at all sensible.
 */
void
Module::add_port(boost::shared_ptr<Port> p)
{
	PortVector::const_iterator i = std::find(_ports.begin(), _ports.end(), p);
	if (i != _ports.end()) // already added
		return;            // so do nothing
		
	if (p->is_input() && p->natural_width() > _widest_input)
		_widest_input = p->width();
	else if (p->is_output() && p->natural_width() > _widest_output)
		_widest_output = p->width();
	
	_ports.push_back(p);
	
	boost::shared_ptr<Canvas> canvas = _canvas.lock();
	if (canvas)
		p->signal_event().connect(
			sigc::bind(sigc::mem_fun(canvas.get(), &Canvas::port_event), p));
	
	p->signal_renamed.connect(sigc::mem_fun(this, &Module::resize));
}

	

/** Embed a widget on the module.
 *
 * Resize may need to be called after this to ensure the module
 * displays correctly.
 */
void
Module::embed(Gtk::Container* widget)
{
	if (!widget) {
		delete _embed_item;
		_embed_item = NULL;
		_embed_width = 0;
		_embed_height = 0;
		return;
	} else {
		_embed_container = manage(widget);
	}

	_embed_container->set_border_width(2);
	_embed_container->show_all();

	const double y = 4 + _canvas_title.property_text_height();
	delete _embed_item;
	_embed_item = new Gnome::Canvas::Widget(*this, 2.0, y, *_embed_container);
	_embed_item->show();

	Gtk::Requisition r = _embed_container->size_request();
	embed_size_request(&r, true);

	_embed_item->raise_to_top();

	_embed_container->signal_size_request().connect(sigc::bind(
				sigc::mem_fun(this, &Module::embed_size_request), false));
}

	
void
Module::embed_size_request(Gtk::Requisition* r, bool force)
{
	if (!force && _embed_width == r->width && _embed_height == r->height)
		return;

	_embed_width = r->width;
	_embed_height = r->height;
	
	resize();

	Gtk::Allocation allocation;
	allocation.set_width(r->width + 4);
	allocation.set_height(r->height + 4);

	_embed_container->size_allocate(allocation);
	_embed_item->property_width() = r->width - 4;
	_embed_item->property_height() = r->height;
}


/** Resize the module to fit its contents best.
 */
void
Module::resize()
{
	// The amount of space between a port edge and the module edge (on the
	// side that the port isn't right on the edge).
	const double hor_pad = (_title_visible ? 10.0 : 20.0);
	
	double width = (_title_visible
		? _canvas_title.property_text_width() + 10.0
		: 1.0);
	
	if (_icon_box)
		width += _icon_size + 2;

	// Title is wide, put inputs and outputs beside each other
	bool horiz = (_widest_input + _widest_output + 10.0 < std::max(width, _embed_width));
	
	// Fit ports to module (or vice-versa)
	double widest_in  = _widest_input;
	double widest_out = _widest_output;
	double expand_w = (horiz ? (width / 2.0) : width) - hor_pad;
	widest_in  = (_embed_item ? _widest_input  : std::max(_widest_input,  expand_w));
	widest_out = (_embed_item ? _widest_output : std::max(_widest_output, expand_w));

	const double widest = std::max(widest_in, widest_out);
	const double title_height = _canvas_title.property_text_height();

	double above_w   = std::max(width, widest + hor_pad);
	double between_w = std::max(width, widest_in + widest_out + _embed_width);
	
	above_w = std::max(above_w, _embed_width);

	// Basic height contains title, icon
	double header_height = 2;
	if (_title_visible)
		header_height += 2 + title_height;
	if (_icon_box && _icon_size > title_height)
		header_height += _icon_size - title_height;

	double height = header_height;

	double above_h = 0.0f;
	if (_ports.size() > 0)
		above_h += _ports.size() * ((*_ports.begin())->height()+2.0);

	//double between_h = std::max(above_h, _embed_height);
	above_h += _embed_height;
	
	/*cerr << above_w << "x" << above_h << "(" << above_w * above_h << ") ? "
			<< between_w << "x" << between_h << "(" << between_w * between_h << ")" << endl;*/
	
	// Decide where to place embedded widget if necessary)
	enum { BETWEEN, ABOVE } embed_pos = ABOVE;
	//if (above_w * above_h >= between_w * between_h) { // minimize area
	if (_embed_width < _embed_height * 2.0) {
		embed_pos = BETWEEN;
		width = between_w;
		if (_embed_item)
			_embed_item->property_x() = widest_in;
	} else {
		width = above_w;
		if (_embed_item)
			_embed_item->property_x() = 0.0f;
	}

	if (!_title_visible) {
		if (_ports.size() > 0)
			height += 0.5;
		if (_widest_input == 0.0 || _widest_output == 0.0f)
			width += 10.0;
	}

	width += _border_width * 2.0;
	
	// Actually set width and height
	set_width(width);
	
	// Offset ports below embedded widget
	if (embed_pos == ABOVE) {
		header_height += _embed_height;
	}
	
	// Move ports to appropriate locations
	int i = 0;
	bool last_was_input = false;
	double y = 0.0;
	double h = 0.0;
	for (PortVector::iterator pi = _ports.begin(); pi != _ports.end(); ++pi) {
		const boost::shared_ptr<Port> p = (*pi);
		h = p->height();

		if (p->is_input()) {
			y = header_height + (i * (h + 2.0));
			++i;
			p->set_width(widest_in);
			p->property_x() = 0.5;
			p->property_y() = y;
			last_was_input = true;
		} else {
			if (!horiz || !last_was_input) {
				y = header_height + (i * (h + 2.0));
				++i;
			}
			p->set_width(widest_out);
			p->property_x() = _width - p->width() - 0.5;
			p->property_y() = y;
			last_was_input = false;
		}
		
		(*pi)->move_connections();
	}
	
	set_height(y + h + 2.0);

	// Center title
	if (_icon_box)
		_canvas_title.property_x() = _icon_size + (_width - _icon_size + 1)/2.0;
	else
		_canvas_title.property_x() = _width/2.0;

	// Make things actually move to their new locations (?!)
	move(0, 0);
}

	
/** Expand the canvas if the module is outside bounds.
 */
void
Module::fit_canvas()
{
	boost::shared_ptr<Canvas> canvas = _canvas.lock();
	if (canvas) {
		double canvas_width = canvas->width();
		double canvas_height = canvas->height();
	
		canvas_width = max(canvas_width, property_x() + _width + 5.0);
		canvas_height = max(canvas_height, property_y() + _height + 5.0);

		canvas->resize(canvas_width, canvas_height);
	}
}


void
Module::select_tick()
{
	boost::shared_ptr<Canvas> canvas = _canvas.lock();
	if (canvas)
		_module_box.property_dash() = canvas->select_dash();
}

	
void
Module::set_border_color(uint32_t c)
{
	_border_color = c;
	_module_box.property_outline_color_rgba() = _border_color;
}


void
Module::set_default_base_color()
{
	_color = MODULE_FILL_COLOUR;
	_module_box.property_fill_color_rgba() = _color;
	if (_stacked_border)
		_stacked_border->property_fill_color_rgba() = _color;
}


void
Module::set_base_color(uint32_t c)
{
	_color = c;
	_module_box.property_fill_color_rgba() = _color;
	if (_stacked_border)
		_stacked_border->property_fill_color_rgba() = _color;
}


} // namespace FlowCanvas

