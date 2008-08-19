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

#include <iostream>
#include <cmath>
#include <boost/weak_ptr.hpp>
#include <libgnomecanvasmm.h>
#include <flowcanvas/Port.hpp>
#include <flowcanvas/Module.hpp>
#include <flowcanvas/Canvas.hpp>

using namespace std;

#define SELECTED_COLOR 0xFF0000FF

namespace FlowCanvas {
	

/** Contruct a Port on an existing module.
 *
 * A reference to @a module is not retained (only a weak_ptr is stored).
 */
Port::Port(boost::shared_ptr<Module> module, const string& name, bool is_input, uint32_t color)
	: Gnome::Canvas::Group(*module.get(), 0, 0)
	, _module(module)
	, _name(name)
	, _is_input(is_input)
	, _color(color)
	, _selected(false)
	, _toggled(false)
	, _control_value(0.0f)
	, _control_min(0.0f)
	, _control_max(1.0f)
	, _control_rect(NULL)
	, _menu(NULL)
{
	const float z = module->canvas().lock()->get_zoom();
	// Create label first (and find size)
	_label = new Gnome::Canvas::Text(*this, 0, 0, _name);
	if (z != 1.0)
		zoom(z);
		
	const double text_width = _label->property_text_width();
	_width = text_width + 6.0;
	_height = _label->property_text_height();
	_label->property_x() = text_width / 2.0 + 3.0;
	_label->property_y() = (_height / 2.0) - 1.0;
	/* WARNING: Doing this makes things extremely slow!
	_label->property_size() = PORT_LABEL_SIZE;
	_label->property_weight() = 200; */
	_label->property_fill_color_rgba() = 0xFFFFFFFF;

	// Create rect to enclose label
	_rect = new Gnome::Canvas::Rect(*this, 0, 0, _width, _height);
	set_border_width(0.0);
	_rect->property_fill_color_rgba() = color;
	
	_label->raise_to_top();
}


Port::~Port()
{
	delete _label;
	delete _rect;
	delete _control_rect;
}


void
Port::show_control()
{
	if (!_control_rect) {
		_control_rect = new Gnome::Canvas::Rect(*this, 0, 0, 0, _height);
		_control_rect->property_outline_color_rgba() = 0xFFFFFCC;
		_control_rect->property_fill_color_rgba() = 0xFFFFFF55;
		_control_rect->show();
	}
}


void
Port::hide_control()
{
	delete _control_rect;
	_control_rect = NULL;
}


/** Set the value for this port's control slider to display.
 */ 
void
Port::set_control(float value, bool signal)
{
	if (!_control_rect)
		return;
	
	if (_toggled) {
		if (value != 0.0)
			value = _control_max;
		else
			value = _control_min;
	}

	//cerr << _name << ".set_control(" << value << "): " << _control_min << " .. " << _control_max
	//		<< " -> ";
	if (value < _control_min)
		_control_min = value;
	if (value > _control_max)
		_control_max = value;

	if (_control_max == _control_min)
		_control_max = _control_min + 1.0;

	if (isinf(_control_max))
		_control_max = FLT_MAX;

	int inf = isinf(value);
	if (inf == -1)
		value = _control_min;
	else if (inf == 1)
		value = _control_max;

	const double w = (value - _control_min) / (_control_max - _control_min) * _width;
	if (isnan(w)) {
		cerr << "WARNING (" << _name << "): Control value is NaN" << endl;
		return;
	}

	//cerr << w << " / " << _width << endl;
	
	_control_rect->property_x2() = _control_rect->property_x1() + w;
	if (signal && _control_value == value)
		signal = false;

	_control_value = value;

	if (signal)
		signal_control_changed.emit(_control_value);
}


void
Port::toggle(bool signal)
{
	if (_control_value == 0.0f)
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
	if (_control_rect)
		_control_rect->property_width_units() = w;
}


void
Port::set_name(const string& n)
{
	if (_name != n) {
		_name = n;

		// Reposition label
		_label->property_text() = _name;
		const double text_width = _label->property_text_width();
		_width = text_width + 6.0;
		_height = _label->property_text_height();
		_rect->property_x2() = _width;	
		_rect->property_y2() = _height;
		if (_control_rect) {
			_control_rect->property_x2() = _control_rect->property_x1() + (_control_value * _width);
			_control_rect->property_y2() = _height;
		}
		_label->property_x() = text_width / 2 + 1;
		_label->property_y() = _height / 2;

		signal_renamed.emit();
	}
}


void
Port::zoom(float z)
{
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
Port::set_selected(bool b)
{
	_selected = b;
	set_fill_color((b ? SELECTED_COLOR : _color));
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
		_rect->property_fill_color_rgba() = (_selected ? SELECTED_COLOR : _color);
		_rect->property_outline_color_rgba() = _color;
	}
}
	

// Returns the world-relative coordinates of where a connection line
// should attach if this is it's source
Gnome::Art::Point
Port::src_connection_point()
{
	double x = (is_input()) ? _rect->property_x1()-1.0 : _rect->property_x2()+1.0;
	double y = _rect->property_y1() + _height / 2.0;
	
	i2w(x, y); // convert to world-relative coords
	
	return Gnome::Art::Point(x, y);
}


// Returns the world-relative coordinates of where a connection line
// should attach if this is it's dest
Gnome::Art::Point
Port::dst_connection_point(const Gnome::Art::Point& src)
{
	double x = (is_input()) ? _rect->property_x1()-1.0 : _rect->property_x2()+1.0;
	double y = _rect->property_y1() + _height / 2.0;
	
	i2w(x, y); // convert to world-relative coords
	
	return Gnome::Art::Point(x, y);
}


void
Port::set_width(double w)
{
	_rect->property_x2() = _rect->property_x2() + (w - _width);
	_width = w;
	set_control(_control_value, false);
}


} // namespace FlowCanvas
