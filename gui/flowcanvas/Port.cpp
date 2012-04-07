/* This file is part of FlowCanvas.
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
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
#include <cmath>
#include <iostream>
#include <list>
#include <string>

#include <boost/weak_ptr.hpp>

#include <libgnomecanvasmm.h>

#include "Canvas.hpp"
#include "Module.hpp"
#include "Port.hpp"

using std::cerr;
using std::endl;
using std::string;
using std::list;

static const uint32_t PORT_SELECTED_COLOR   = 0xFF0000FF;
static const uint32_t PORT_EMPTY_PORT_BREADTH = 16;
static const uint32_t PORT_EMPTY_PORT_DEPTH = 32;

namespace FlowCanvas {


/** Contruct a Port on an existing module.
 *
 * A reference to @a module is not retained (only a weak_ptr is stored).
 */
Port::Port(boost::shared_ptr<Module> module, const string& name, bool is_input, uint32_t color)
	: Gnome::Canvas::Group(*module.get(), 0, 0)
	, _module(module)
	, _name(name)
	, _label(NULL)
	, _rect(NULL)
	, _menu(NULL)
	, _control(NULL)
	, _color(color)
	, _is_input(is_input)
	, _selected(false)
	, _toggled(false)

{
	boost::shared_ptr<Canvas> canvas = module->canvas().lock();

	// Create label first and zoom (to find size correctly)
	if (canvas->direction() == Canvas::HORIZONTAL)
		_label = new Gnome::Canvas::Text(*this, 0, 0, _name);
	else
		_label = NULL;

	const double z = canvas->get_zoom();
	zoom(z);

	if (_label) {
		show_label(true);
	} else {
		if (canvas->direction() == Canvas::HORIZONTAL) {
			_width  = module->empty_port_depth() * z;
			_height = module->empty_port_breadth() * z;
		} else {
			_width  = module->empty_port_breadth() * z;
			_height = module->empty_port_depth() * z;
		}
	}

	// Create rect to enclose label
	_rect = new Gnome::Canvas::Rect(*this, 0, 0, _width, _height);
	set_border_width(0.0);
	_rect->property_fill_color_rgba() = color;
	_rect->property_outline_color_rgba() = color;

	if (_label)
		_label->raise_to_top();
}


Port::~Port()
{
	delete _label;
	delete _rect;
	delete _control;
}


void
Port::show_control()
{
	if (!_control) {
		Gnome::Canvas::Rect* rect = new Gnome::Canvas::Rect(*this, 0.5, 0.5, 0.0, _height - 0.5);
		//rect->property_outline_color_rgba() = 0xFFFFFF45;
		rect->property_width_pixels() = 0;
		rect->property_fill_color_rgba() = 0xFFFFFF80;
		rect->show();
		_control = new Control(rect);
	}
}


void
Port::hide_control()
{
	delete _control;
	_control = NULL;
}


/** Set the value for this port's control slider to display.
 */
void
Port::set_control(float value, bool signal)
{
	if (!_control)
		return;

	if (_toggled) {
		if (value != 0.0)
			value = _control->max;
		else
			value = _control->min;
	}

	if (value < _control->min)
		_control->min = value;
	if (value > _control->max)
		_control->max = value;

	if (_control->max == _control->min)
		_control->max = _control->min + 1.0;

	if (std::isinf(_control->max))
		_control->max = FLT_MAX;

	int inf = std::isinf(value);
	if (inf == -1)
		value = _control->min;
	else if (inf == 1)
		value = _control->max;

	const double w = (value - _control->min) / (_control->max - _control->min) * _width;
	if (std::isnan(w)) {
		cerr << "WARNING (" << _name << "): Control value is NaN" << endl;
		return;
	}

	//cerr << w << " / " << _width << endl;

	_control->rect->property_x2() = _control->rect->property_x1() + std::max(0.0, w-1.0);
	if (signal && _control->value == value)
		signal = false;

	_control->value = value;

	if (signal)
		signal_control_changed.emit(_control->value);
}


void
Port::set_control_min(float min)
{
	if (!_control)
		return;
	
	_control->min = min;
	set_control(_control->value, false);
}


void
Port::set_control_max(float max)
{
	if (!_control)
		return;

	_control->max = max;
	set_control(_control->value, false);
}


void
Port::toggle(bool signal)
{
	if (_control->value == 0.0f)
		set_control(1.0f, signal);
	else
		set_control(0.0f, signal);
}


/** Set the border width of the port's rectangle.
 *
 * Do NOT directly set the width_units property on the rect, use this function.
 */
void
Port::set_border_width(double w)
{
	_border_width = w;
	_rect->property_width_units() = w;
}


double
Port::natural_width() const
{
	if (_label)
		return _label->property_text_width();
	else
		return PORT_EMPTY_PORT_DEPTH; // Used by Canvas::resize_horiz only
}


void
Port::set_name(const string& n)
{
	if (_label && _name != n) {
		_name = n;

		// Reposition label
		_label->property_text() = _name;
		const double text_width = _label->property_text_width();
		_width = text_width + 6.0;
		_height = _label->property_text_height();
		_rect->property_x2() = _width;
		_rect->property_y2() = _height;
		if (_control) {
			_control->rect->property_x2() = _control->rect->property_x1() + (_control->value * (_width-1));
			_control->rect->property_y2() = _height - 0.5;
		}
		_label->property_x() = (_width / 2.0) - 3.0;
		_label->property_y() = (_height / 2.0);

		signal_renamed.emit();
	}
}


void
Port::zoom(float z)
{
	if (_label)
		_label->property_size() = static_cast<int>(floor(8000.0f * z));
}


void
Port::create_menu()
{
	// Derived classes may just override this
	_menu = new Gtk::Menu();
	_menu->items().push_back(Gtk::Menu_Helpers::MenuElem(
		"Disconnect All", sigc::mem_fun(this, &Port::disconnect_all)));
	_menu->signal_selection_done().connect(sigc::mem_fun(this, &Port::on_menu_hide));
}


void
Port::set_menu(Gtk::Menu* m)
{
	delete _menu;
	_menu = m;
	_menu->signal_selection_done().connect(sigc::mem_fun(this, &Port::on_menu_hide));
}


void
Port::on_menu_hide()
{
	set_highlighted(false);
}


void
Port::disconnect_all()
{
	boost::shared_ptr<Module> module = _module.lock();
	if (!module)
		return;

	list<boost::weak_ptr<Connection> > connections = _connections; // copy
	for (list<boost::weak_ptr<Connection> >::iterator i = connections.begin(); i != connections.end(); ++i) {
		boost::shared_ptr<Connection> c = (*i).lock();
		if (c) {
			module->canvas().lock()->disconnect(c->source().lock(), c->dest().lock());
		}
	}

	_connections.clear();
}


void
Port::show_label(bool b)
{
	boost::shared_ptr<Module> m = module().lock();
	boost::shared_ptr<Canvas> canvas = (m ? m->canvas().lock() : boost::shared_ptr<Canvas>());

	if (!canvas)
		return;

	if (b) {
		// Create label first then zoom (to find size correctly)
		if (!_label)
			_label = new Gnome::Canvas::Text(*this, 0, 0, _name);

		zoom(canvas->get_zoom());

		const double text_width = _label->property_text_width();
		_width = text_width + 6.0;
		_height = _label->property_text_height();
		set_width(_width);
		set_height(_height);
		_label->property_x() = (_width / 2.0) - 3.0;
		_label->property_y() = (_height / 2.0);
		_label->property_fill_color_rgba() = 0xFFFFFFFF;

		_label->raise_to_top();
	} else {
		delete _label;
		_label = NULL;
		if (canvas->direction() == Canvas::HORIZONTAL) {
			_width  = PORT_EMPTY_PORT_DEPTH;
			_height = PORT_EMPTY_PORT_BREADTH;
		} else {
			_width  = PORT_EMPTY_PORT_BREADTH;
			_height = PORT_EMPTY_PORT_DEPTH;
		}
		set_width(_width);
		set_height(_height);
		if (_rect)
			_rect->raise_to_top();
	}

	if (_rect)
		_rect->property_x2() = _rect->property_x1() + _width;
}


void
Port::set_selected(bool b)
{
	_selected = b;
	set_fill_color((b ? PORT_SELECTED_COLOR : _color));
}


void
Port::set_highlighted(bool b, bool highlight_parent, bool highlight_connections, bool raise_connections)
{
	if (highlight_parent) {
		boost::shared_ptr<Module> module = _module.lock();
		if (module)
			module->set_highlighted(b);
	}

	if (highlight_connections) {
		for (Connections::iterator i = _connections.begin(); i != _connections.end(); ++i) {
			boost::shared_ptr<Connection> connection = (*i).lock();
			if (connection) {
				connection->set_highlighted(b);
				if (raise_connections && b)
					connection->raise_to_top();
			}
		}
	}

	if (b) {
		/*raise_to_top();
		_rect->raise_to_top();
		_label.raise_to_top();*/
		_rect->property_fill_color_rgba() = _color + 0x33333300;
		_rect->property_outline_color_rgba() = _color + 0x33333300;
	} else {
		_rect->property_fill_color_rgba() = (_selected ? PORT_SELECTED_COLOR : _color);
		_rect->property_outline_color_rgba() = _color;
	}
}


// Returns the world-relative coordinates of where a connection line
// should attach if this is it's source
Gnome::Art::Point
Port::src_connection_point()
{
	bool horizontal = true;
	boost::shared_ptr<Module> m = module().lock();
	if (m) {
		boost::shared_ptr<Canvas> canvas = m->canvas().lock();
		if (canvas)
			horizontal = (canvas->direction() == Canvas::HORIZONTAL);
	}

	double x, y;

	if (horizontal) {
		x = (is_input()) ? _rect->property_x1() : _rect->property_x2();
		y = _rect->property_y1() + _height / 2.0;
	} else {
		x = _rect->property_x1() + _width / 2.0;
		y = (is_input()) ? _rect->property_y1() : _rect->property_y2();
	}

	i2w(x, y); // convert to world-relative coords

	return Gnome::Art::Point(x, y);
}


// Returns the world-relative coordinates of where a connection line
// should attach if this is it's dest
Gnome::Art::Point
Port::dst_connection_point(const Gnome::Art::Point& src)
{
	return src_connection_point();
}


void
Port::set_width(double w)
{
	if (_rect)
		_rect->property_x2() = _rect->property_x2() + (w - _width);
	_width = w;
	if (_control)
		set_control(_control->value, false);
}


void
Port::set_height(double h)
{
	if (_rect)
		_rect->property_y2() = _rect->property_y1() + h;
	if (_control)
		_control->rect->property_y2() = _control->rect->property_y1() + h - 0.5;
	_height = h;
}


Gnome::Art::Point
Port::connection_point_vector(double dx, double dy)
{
	bool horizontal = true;
	boost::shared_ptr<Module> m = module().lock();
	if (m) {
		boost::shared_ptr<Canvas> canvas = m->canvas().lock();
		if (canvas)
			horizontal = (canvas->direction() == Canvas::HORIZONTAL);
	}

	if (horizontal) {
		return Gnome::Art::Point(dx, 0);
	} else {
		return Gnome::Art::Point(0, dy);
	}
}


} // namespace FlowCanvas
