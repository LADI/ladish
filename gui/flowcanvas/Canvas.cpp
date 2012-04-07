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
#include <cassert>
#include <cmath>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <boost/enable_shared_from_this.hpp>

#include "config.h"
#include "Canvas.hpp"
#include "Module.hpp"
#include "Port.hpp"

#ifdef HAVE_AGRAPH
#include <gvc.h>
#endif

using std::cerr;
using std::endl;
using std::list;
using std::string;
using std::vector;

namespace FlowCanvas {

sigc::signal<void, Gnome::Canvas::Item*> Canvas::signal_item_entered;
sigc::signal<void, Gnome::Canvas::Item*> Canvas::signal_item_left;

Canvas::Canvas(double width, double height)
	: _base_rect(*root(), 0, 0, width, height)
	, _select_rect(NULL)
	, _select_dash(NULL)
	, _zoom(1.0)
	, _width(width)
	, _height(height)
	, _drag_state(NOT_DRAGGING)
	, _direction(HORIZONTAL)
	, _remove_objects(true)
	, _locked(false)
{
	set_scroll_region(0.0, 0.0, width, height);
	set_center_scroll_region(true);

	_base_rect.property_fill_color_rgba() = 0x000000FF;
	//_base_rect.show();
	_base_rect.signal_event().connect(sigc::mem_fun(this, &Canvas::scroll_drag_handler));
	_base_rect.signal_event().connect(sigc::mem_fun(this, &Canvas::canvas_event));
	_base_rect.signal_event().connect(sigc::mem_fun(this, &Canvas::select_drag_handler));
	_base_rect.signal_event().connect(sigc::mem_fun(this, &Canvas::connection_drag_handler));

	set_dither(Gdk::RGB_DITHER_NORMAL); // NONE or NORMAL or MAX

	// Dash style for selected modules and selection box
	_select_dash = new ArtVpathDash();
	_select_dash->n_dash = 2;
	_select_dash->dash = art_new(double, 2);
	_select_dash->dash[0] = 5;
	_select_dash->dash[1] = 5;

	Glib::signal_timeout().connect(
		sigc::mem_fun(this, &Canvas::animate_selected), 300);
}


Canvas::~Canvas()
{
	destroy();
	art_free(_select_dash->dash);
	delete _select_dash;
}


void
Canvas::lock(bool l)
{
	_locked = l;
	if (l)
		_base_rect.property_fill_color_rgba() = 0x131415FF;
	else
		_base_rect.property_fill_color_rgba() = 0x000000FF;

}


void
Canvas::set_zoom(double pix_per_unit)
{
	if (_zoom == pix_per_unit)
		return;

	_zoom = pix_per_unit;
	set_pixels_per_unit(_zoom);

	for (ItemList::iterator m = _items.begin(); m != _items.end(); ++m)
		(*m)->zoom(_zoom);

	for (list<boost::shared_ptr<Connection> >::iterator c = _connections.begin(); c != _connections.end(); ++c)
		(*c)->zoom(_zoom);
}


void
Canvas::scroll_to_center()
{
	int win_width  = 0;
	int win_height = 0;

	Glib::RefPtr<Gdk::Window> win = get_window();
	if (win)
		win->get_size(win_width, win_height);

	scroll_to((int)((_width - win_width) / 2.0),
	          (int)((_height - win_height) / 2.0));
}


void
Canvas::zoom_full()
{
	if (_items.empty())
		return;

	int win_width, win_height;
	Glib::RefPtr<Gdk::Window> win = get_window();
	win->get_size(win_width, win_height);

	// Box containing all canvas items
	double left   = DBL_MAX;
	double right  = DBL_MIN;
	double top    = DBL_MIN;
	double bottom = DBL_MAX;

	for (ItemList::iterator m = _items.begin(); m != _items.end(); ++m) {
		const boost::shared_ptr<Item> mod = (*m);
		if (!mod)
			continue;

		if (mod->property_x() < left)
			left = mod->property_x();
		if (mod->property_x() + mod->width() > right)
			right = mod->property_x() + mod->width();
		if (mod->property_y() < bottom)
			bottom = mod->property_y();
		if (mod->property_y() + mod->height() > top)
			top = mod->property_y() + mod->height();
	}

	static const double pad = 8.0;

	const double new_zoom = std::min(
		((double)win_width / (double)(right - left + pad*2.0)),
		((double)win_height / (double)(top - bottom + pad*2.0)));

	set_zoom(new_zoom);

	int scroll_x, scroll_y;
	w2c(lrintf(left - pad), lrintf(bottom - pad), scroll_x, scroll_y);

	scroll_to(scroll_x, scroll_y);
}


void
Canvas::clear_selection()
{
	unselect_ports();

	for (list<boost::shared_ptr<Item> >::iterator m = _selected_items.begin(); m != _selected_items.end(); ++m)
		(*m)->set_selected(false);

	for (list<boost::shared_ptr<Connection> >::iterator c = _selected_connections.begin(); c != _selected_connections.end(); ++c)
		(*c)->set_selected(false);

	_selected_items.clear();
	_selected_connections.clear();
}


void
Canvas::unselect_connection(Connection* connection)
{
	for (ConnectionList::iterator i = _selected_connections.begin(); i != _selected_connections.end();) {
		if ((*i).get() == connection) {
			i = _selected_connections.erase(i);
		} else {
			++i;
		}
	}

	connection->set_selected(false);
}


/** Add a module to the current selection, and automagically select any connections
 * between selected modules */
void
Canvas::select_item(boost::shared_ptr<Item> m)
{
	assert(! m->selected());

	_selected_items.push_back(m);

	for (ConnectionList::iterator i = _connections.begin(); i != _connections.end(); ++i) {
		const boost::shared_ptr<Connection>  c = (*i);
		const boost::shared_ptr<Connectable> src = c->source().lock();
		const boost::shared_ptr<Connectable> dst = c->dest().lock();
		if (!src || !dst)
			continue;

		const boost::shared_ptr<Port> src_port
			= boost::dynamic_pointer_cast<Port>(src);
		const boost::shared_ptr<Port> dst_port
			= boost::dynamic_pointer_cast<Port>(dst);

		if (!src_port || !dst_port)
			continue;

		const boost::shared_ptr<Module> src_module = src_port->module().lock();
		const boost::shared_ptr<Module> dst_module = dst_port->module().lock();
		if (!src_module || !dst_module)
			continue;

		if ( !c->selected()) {
			if (src_module == m && dst_module->selected()) {
				c->set_selected(true);
				_selected_connections.push_back(c);
			} else if (dst_module == m && src_module->selected()) {
				c->set_selected(true);
				_selected_connections.push_back(c);
			}
		}
	}

	m->set_selected(true);
}


void
Canvas::unselect_item(boost::shared_ptr<Item> m)
{
	// Remove any connections that aren't selected anymore because this module isn't
	boost::shared_ptr<Connection> c;
	for (ConnectionList::iterator i = _selected_connections.begin(); i != _selected_connections.end() ; ) {
		c = (*i);

		if (boost::dynamic_pointer_cast<Item>(c->source().lock()) == m
				|| boost::dynamic_pointer_cast<Item>(c->dest().lock()) == m) {
			c->set_selected(false);
			i = _selected_connections.erase(i);
			continue;
		}

		boost::shared_ptr<Port> src_port = boost::dynamic_pointer_cast<Port>(c->source().lock());
		if (src_port && src_port->module().lock() == m) {
			c->set_selected(false);
			i = _selected_connections.erase(i);
			continue;
		}

		boost::shared_ptr<Port> dst_port = boost::dynamic_pointer_cast<Port>(c->dest().lock());
		if (dst_port && dst_port->module().lock() == m) {
			c->set_selected(false);
			i = _selected_connections.erase(i);
			continue;
		}

		++i;
	}

	// Remove the module
	for (list<boost::shared_ptr<Item> >::iterator i = _selected_items.begin(); i != _selected_items.end(); ++i) {
		if ((*i) == m) {
			_selected_items.erase(i);
			break;
		}
	}

	m->set_selected(false);
}


/** Removes all ports and connections and modules.
 */
void
Canvas::destroy()
{
	_remove_objects = false;

	_selected_items.clear();
	_selected_connections.clear();

	_connections.clear();

	_selected_ports.clear();
	_connect_port.reset();

	_items.clear();

	_remove_objects = true;
}


void
Canvas::unselect_ports()
{
	for (SelectedPorts::iterator i = _selected_ports.begin(); i != _selected_ports.end(); ++i)
		(*i)->set_selected(false);

	_selected_ports.clear();
	_last_selected_port.reset();
}


void
Canvas::select_port(boost::shared_ptr<Port> p, bool unique)
{
	if (unique)
		unselect_ports();
	p->set_selected(true);
	SelectedPorts::iterator i = find(_selected_ports.begin(), _selected_ports.end(), p);
	if (i == _selected_ports.end())
		_selected_ports.push_back(p);
	_last_selected_port = p;
}


void
Canvas::unselect_port(boost::shared_ptr<Port> p)
{
	SelectedPorts::iterator i = find(_selected_ports.begin(), _selected_ports.end(), p);
	if (i != _selected_ports.end())
		_selected_ports.erase(i);
	p->set_selected(false);
	if (_last_selected_port == p)
		_last_selected_port.reset();
}


void
Canvas::select_port_toggle(boost::shared_ptr<Port> p, int mod_state)
{
	if ((mod_state & GDK_CONTROL_MASK)) {
		if (p->selected())
			unselect_port(p);
		else
			select_port(p);
	} else if ((mod_state & GDK_SHIFT_MASK)) {
		boost::shared_ptr<Module> m = p->module().lock();
		if (_last_selected_port && m && _last_selected_port->module().lock() == m) {
			// Pivot around _last_selected_port in a single pass over module ports each click
			boost::shared_ptr<Port> old_last_selected = _last_selected_port;
			boost::shared_ptr<Port> first;
			bool done = false;
			const PortVector& ports = m->ports();
			for (size_t i = 0; i < ports.size(); ++i) {
				if (!first && !done && (ports[i] == _last_selected_port || ports[i] == p))
					first = ports[i];

				if (first && !done && ports[i]->is_input() == first->is_input())
					select_port(ports[i], false);
				else
					unselect_port(ports[i]);

				if (ports[i] != first && (ports[i] == old_last_selected || ports[i] == p))
					done = true;
			}
			_last_selected_port = old_last_selected;
		} else {
			if (p->selected())
				unselect_port(p);
			else
				select_port(p);
		}
	} else {
		if (p->selected())
			unselect_ports();
		else
			select_port(p, true);
	}
}


/** Sets the passed module's location to a reasonable default.
 */
void
Canvas::set_default_placement(boost::shared_ptr<Module> m)
{
	assert(m);

	// Simple cascade.  This will get more clever in the future.
	double x = ((_width / 2.0) + (_items.size() * 25));
	double y = ((_height / 2.0) + (_items.size() * 25));

	m->move_to(x, y);
}


void
Canvas::add_item(boost::shared_ptr<Item> m)
{
	if (m)
		_items.push_back(m);
}


/** Remove an item from the canvas, cutting all references.
 * Returns true if item was found (and removed).
 */
bool
Canvas::remove_item(boost::shared_ptr<Item> item)
{
	bool ret = false;

	// Remove from selection
	for (list<boost::shared_ptr<Item> >::iterator i = _selected_items.begin(); i != _selected_items.end(); ++i) {
		if ((*i) == item) {
			_selected_items.erase(i);
			break;
		}
	}

	// Remove children ports from selection if item is a module
	boost::shared_ptr<Module> module = boost::dynamic_pointer_cast<Module>(item);
	if (module) {
		for (PortVector::iterator i = module->ports().begin(); i != module->ports().end(); ++i) {
			unselect_port(*i);
		}
	}

	// Remove from items
	for (ItemList::iterator i = _items.begin(); i != _items.end(); ++i) {
		if (*i == item) {
			ret = true;
			_items.erase(i);
			break;
		}
	}

	// Remove any connections adjacent to this item
	boost::shared_ptr<Connection> c;
	for (ConnectionList::iterator i = _connections.begin(); i != _connections.end() ; ) {
		c = (*i);
		ConnectionList::iterator next = i;
		++next;

		const boost::shared_ptr<Port> src_port
			= boost::dynamic_pointer_cast<Port>(c->source().lock());
		const boost::shared_ptr<Port> dst_port
			= boost::dynamic_pointer_cast<Port>(c->dest().lock());

		if (boost::dynamic_pointer_cast<Item>(c->source().lock()) == item
				|| boost::dynamic_pointer_cast<Item>(c->dest().lock()) == item
				|| (src_port && src_port->module().lock() == item)
				|| (dst_port && dst_port->module().lock() == item))
			remove_connection(c);

		i = next;
	}

	return ret;
}


boost::shared_ptr<Connection>
Canvas::remove_connection(boost::shared_ptr<Connectable> item1,
                          boost::shared_ptr<Connectable> item2)
{
	boost::shared_ptr<Connection> ret;

	if (!_remove_objects)
		return ret;

	assert(item1);
	assert(item2);

	boost::shared_ptr<Connection> c = get_connection(item1, item2);
	if (!c) {
		cerr << "Couldn't find connection." << endl;
		return ret;
	} else {
		remove_connection(c);
		return c;
	}
}


/** Return whether there is a connection between item1 and item2.
 *
 * Note that connections are directed, so this may return false when there
 * is a connection between the two items (in the opposite direction).
 */
bool
Canvas::are_connected(boost::shared_ptr<const Connectable> tail,
                      boost::shared_ptr<const Connectable> head)
{
	for (ConnectionList::const_iterator c = _connections.begin(); c != _connections.end(); ++c) {
		const boost::shared_ptr<Connectable> src = (*c)->source().lock();
		const boost::shared_ptr<Connectable> dst = (*c)->dest().lock();

		if (src == tail && dst == head)
			return true;
	}

	return false;
}


/** Get the connection between two items.
 *
 * Note that connections are directed.
 * This will only return a connection from @a tail to @a head.
 */
boost::shared_ptr<Connection>
Canvas::get_connection(boost::shared_ptr<Connectable> tail,
                           boost::shared_ptr<Connectable> head) const
{
	for (ConnectionList::const_iterator i = _connections.begin(); i != _connections.end(); ++i) {
		const boost::shared_ptr<Connectable> src = (*i)->source().lock();
		const boost::shared_ptr<Connectable> dst = (*i)->dest().lock();

		if (src == tail && dst == head)
			return *i;
	}

	return boost::shared_ptr<Connection>();
}


bool
Canvas::add_connection(boost::shared_ptr<Connectable> src,
                       boost::shared_ptr<Connectable> dst,
                       uint32_t                       color)
{
	// Create (graphical) connection object
	boost::shared_ptr<Connection> c(new Connection(shared_from_this(), src, dst, color));
	src->add_connection(c);
	dst->add_connection(c);
	_connections.push_back(c);

	return true;
}


bool
Canvas::add_connection(boost::shared_ptr<Connection> c)
{
	const boost::shared_ptr<Connectable> src = c->source().lock();
	const boost::shared_ptr<Connectable> dst = c->dest().lock();

	if (src && dst) {
		src->add_connection(c);
		dst->add_connection(c);
		_connections.push_back(c);
		return true;
	} else {
		return false;
	}
}


void
Canvas::remove_connection(boost::shared_ptr<Connection> connection)
{
	if (!_remove_objects)
		return;

	unselect_connection(connection.get());

	ConnectionList::iterator i = find(_connections.begin(), _connections.end(), connection);

	if (i != _connections.end()) {
		const boost::shared_ptr<Connection> c = *i;

		const boost::shared_ptr<Connectable> src = c->source().lock();
		const boost::shared_ptr<Connectable> dst = c->dest().lock();

		if (src)
			src->remove_connection(c);

		if (dst)
			dst->remove_connection(c);

		_connections.erase(i);
	}
}


void
Canvas::selection_joined_with(boost::shared_ptr<Port> port)
{
	for (SelectedPorts::iterator i = _selected_ports.begin(); i != _selected_ports.end(); ++i)
		ports_joined(*i, port);
}


void
Canvas::join_selection()
{
	vector< boost::shared_ptr<Port> > inputs;
	vector< boost::shared_ptr<Port> > outputs;
	for (SelectedPorts::iterator i = _selected_ports.begin(); i != _selected_ports.end(); ++i) {
		if ((*i)->is_input())
			inputs.push_back(*i);
		else
			outputs.push_back(*i);
	}

	if (inputs.size() == 1) { // 1 -> n
		for (size_t i = 0; i < outputs.size(); ++i)
			ports_joined(inputs[0], outputs[i]);
	} else if (outputs.size() == 1) { // n -> 1
		for (size_t i = 0; i < inputs.size(); ++i)
			ports_joined(inputs[i], outputs[0]);
	} else { // n -> m
		size_t num_to_connect = std::min(inputs.size(), outputs.size());
		for (size_t i = 0; i < num_to_connect; ++i) {
			ports_joined(inputs[i], outputs[i]);
		}
	}
}


/** Called when two ports are 'toggled' (connected or disconnected)
 */
void
Canvas::ports_joined(boost::shared_ptr<Port> port1, boost::shared_ptr<Port> port2)
{
	if (port1 == port2)
		return;

	assert(port1);
	assert(port2);

	port1->set_highlighted(false);
	port2->set_highlighted(false);

	string src_mod_name, dst_mod_name, src_port_name, dst_port_name;

	boost::shared_ptr<Port> src_port;
	boost::shared_ptr<Port> dst_port;

	if (port2->is_input() && ! port1->is_input()) {
		src_port = port1;
		dst_port = port2;
	} else if ( ! port2->is_input() && port1->is_input()) {
		src_port = port2;
		dst_port = port1;
	} else {
		return;
	}

	if (are_connected(src_port, dst_port))
		disconnect(src_port, dst_port);
	else
		connect(src_port, dst_port);
}


/** Event handler for ports.
 *
 * These events can't be handled in the Port class because they have to do with
 * connections etc. which deal with multiple ports (ie _selected_port).  Ports
 * pass their events on to this function to get around this.
 */
bool
Canvas::port_event(GdkEvent* event, boost::weak_ptr<Port> weak_port)
{
	boost::shared_ptr<Port> port = weak_port.lock();
	if (!port)
		return false;

	static bool port_dragging = false;
	static bool control_dragging = false;
	static bool ignore_button_release = false;
	bool handled = true;

	switch (event->type) {

	case GDK_BUTTON_PRESS:
		if (event->button.button == 1) {
			boost::shared_ptr<Module> module = port->module().lock();
			if (module && _locked && port->is_input()) {
				if (port->is_toggled()) {
					port->toggle();
					ignore_button_release = true;
				} else {
					control_dragging = true;
					const double port_x = module->property_x() + port->property_x();
					float new_control = ((event->button.x - port_x) / (double)port->width());
					if (new_control < 0.0)
						new_control = 0.0;
					else if (new_control > 1.0)
						new_control = 1.0;

					new_control *= (port->control_max() - port->control_min());
					new_control += port->control_min();
					if (new_control < port->control_min())
						new_control = port->control_min();
					if (new_control > port->control_max())
						new_control = port->control_max();
					if (new_control != port->control_value())
						port->set_control(new_control);
				}
			} else {
				port_dragging = true;
			}
		} else if (event->button.button == 3) {
			port->popup_menu(event->button.button, event->button.time);
		} else {
			handled = false;
		}
		break;

	case GDK_MOTION_NOTIFY:
		if (control_dragging) {
			boost::shared_ptr<Module> module = port->module().lock();
			if (module) {
				const double port_x = module->property_x() + port->property_x();
				float new_control = ((event->button.x - port_x) / (double)port->width());
				if (new_control < 0.0)
					new_control = 0.0;
				else if (new_control > 1.0)
					new_control = 1.0;

				new_control *= (port->control_max() - port->control_min());
				new_control += port->control_min();
				assert(new_control >= port->control_min());
				assert(new_control <= port->control_max());

				if (new_control != port->control_value())
					port->set_control(new_control);

			}
		}
		break;

	case GDK_BUTTON_RELEASE:
		if (port_dragging) {
			if (_connect_port) { // dragging
				ports_joined(port, _connect_port);
				unselect_ports();
			} else {
				bool modded = event->button.state & (GDK_SHIFT_MASK|GDK_CONTROL_MASK);
				if (!modded && _last_selected_port && _last_selected_port->is_input() != port->is_input()) {
					selection_joined_with(port);
					unselect_ports();
				} else {
					select_port_toggle(port, event->button.state);
				}
			}
			port_dragging = false;
		} else if (control_dragging) {
			control_dragging = false;
		} else if (ignore_button_release) {
			ignore_button_release = false;
		} else {
			handled = false;
		}
		break;

	case GDK_ENTER_NOTIFY:
		signal_item_entered.emit(port.get());
		if (!control_dragging && !port->selected()) {
			port->set_highlighted(true);
			return true;
		}
		break;

	case GDK_LEAVE_NOTIFY:
		if (port_dragging) {
			_drag_state = CONNECTION;
			_connect_port = port;
			port_dragging = false;
			_base_rect.grab(
				GDK_BUTTON_PRESS_MASK|GDK_POINTER_MOTION_MASK|GDK_BUTTON_RELEASE_MASK,
				Gdk::Cursor(Gdk::CROSSHAIR), event->crossing.time);
		} else if (!control_dragging) {
			port->set_highlighted(false);
		}
		signal_item_left.emit(port.get());
		break;

	default:
		handled = false;
	}

	return handled;
}


bool
Canvas::canvas_event(GdkEvent* event)
{
	static const int scroll_increment = 10;
	int scroll_x, scroll_y;
	get_scroll_offsets(scroll_x, scroll_y);

	switch (event->type) {
	case GDK_KEY_PRESS:
		switch (event->key.keyval) {
		case GDK_Up:
			scroll_y -= scroll_increment;
			break;
		case GDK_Down:
			scroll_y += scroll_increment;
			break;
		case GDK_Left:
			scroll_x -= scroll_increment;
			break;
		case GDK_Right:
			scroll_x += scroll_increment;
			break;
		case GDK_Return:
			if (_selected_ports.size() > 1) {
				join_selection();
				clear_selection();
			}
			break;
		default:
            return false;
		}
		scroll_to(scroll_x, scroll_y);
		return true;
	default:
		return false;
	}
}


void
Canvas::on_parent_changed(Gtk::Widget* old_parent)
{
	_parent_event_connection.disconnect();
    if (get_parent())
		_parent_event_connection = get_parent()->signal_event().connect(
				sigc::mem_fun(*this, &Canvas::frame_event));
}


bool
Canvas::frame_event(GdkEvent* ev)
{
	bool handled = false;
	// Zoom
	if (ev->type == GDK_SCROLL && (ev->scroll.state & GDK_CONTROL_MASK)) {
		if (ev->scroll.direction == GDK_SCROLL_UP) {
			set_zoom(_zoom * 1.25);
			handled = true;
		} else if (ev->scroll.direction == GDK_SCROLL_DOWN) {
			set_zoom(_zoom * 0.75);
			handled = true;
		}
	}
	return handled;
}


bool
Canvas::scroll_drag_handler(GdkEvent* event)
{
	bool handled = true;

	static int    original_scroll_x = 0;
	static int    original_scroll_y = 0;
	static double origin_x = 0;
	static double origin_y = 0;
	static double scroll_offset_x = 0;
	static double scroll_offset_y = 0;
	static double last_x = 0;
	static double last_y = 0;

	bool first_motion = true;

	if (event->type == GDK_BUTTON_PRESS && event->button.button == 2) {
		_base_rect.grab(GDK_POINTER_MOTION_MASK|GDK_BUTTON_RELEASE_MASK,
			Gdk::Cursor(Gdk::FLEUR), event->button.time);
		get_scroll_offsets(original_scroll_x, original_scroll_y);
		scroll_offset_x = 0;
		scroll_offset_y = 0;
		origin_x = event->button.x_root;
		origin_y = event->button.y_root;
		//cerr << "Origin: (" << origin_x << "," << origin_y << ")\n";
		last_x = origin_x;
		last_y = origin_y;
		first_motion = true;
		_drag_state = SCROLL;

	} else if (event->type == GDK_MOTION_NOTIFY && _drag_state == SCROLL) {
		const double x        = event->motion.x_root;
		const double y        = event->motion.y_root;
		const double x_offset = last_x - x;
		const double y_offset = last_y - y;

		//cerr << "Coord: (" << x << "," << y << ")\n";
		//cerr << "Offset: (" << x_offset << "," << y_offset << ")\n";

		scroll_offset_x += x_offset;
		scroll_offset_y += y_offset;
		scroll_to(lrint(original_scroll_x + scroll_offset_x),
		          lrint(original_scroll_y + scroll_offset_y));
		last_x = x;
		last_y = y;
	} else if (event->type == GDK_BUTTON_RELEASE && _drag_state == SCROLL) {
		_base_rect.ungrab(event->button.time);
		_drag_state = NOT_DRAGGING;
	} else {
		handled = false;
	}

	return handled;
}


bool
Canvas::select_drag_handler(GdkEvent* event)
{
	boost::shared_ptr<Item> module;

	if (event->type == GDK_BUTTON_PRESS && event->button.button == 1) {
		assert(_select_rect == NULL);
		_drag_state = SELECT;
		if ( !(event->button.state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) )
			clear_selection();
		_select_rect = new Gnome::Canvas::Rect(*root(),
			event->button.x, event->button.y, event->button.x, event->button.y);
		_select_rect->property_fill_color_rgba() = 0x273344FF;
		_select_rect->property_outline_color_rgba() = 0xEEEEFFFF;
		_select_rect->property_width_units() = 0.5;
		_select_rect->lower_to_bottom();
		_base_rect.lower_to_bottom();
		_base_rect.grab(GDK_POINTER_MOTION_MASK|GDK_BUTTON_RELEASE_MASK,
				Gdk::Cursor(Gdk::ARROW), event->button.time);
		return true;
	} else if (event->type == GDK_MOTION_NOTIFY && _drag_state == SELECT) {
		assert(_select_rect);
		double x = event->button.x, y = event->button.y;

		if (event->motion.is_hint) {
			gint t_x;
			gint t_y;
			GdkModifierType state;
			gdk_window_get_pointer(event->motion.window, &t_x, &t_y, &state);
			x = t_x;
			y = t_y;
		}
		_select_rect->property_x2() = x;
		_select_rect->property_y2() = y;
		return true;
	} else if (event->type == GDK_BUTTON_RELEASE && _drag_state == SELECT) {
		// Select all modules within rect
		for (ItemList::iterator i = _items.begin(); i != _items.end(); ++i) {
			module = (*i);
			if (module->is_within(*_select_rect)) {
				if (module->selected())
					unselect_item(module);
				else
					select_item(module);
			}
		}

		_base_rect.ungrab(event->button.time);

		delete _select_rect;
		_select_rect = NULL;
		_drag_state = NOT_DRAGGING;
		return true;
	}
	return false;
}


/** Updates _select_dash for rotation effect, and updates any
  * selected item's borders (and the selection rectangle).
  */
bool
Canvas::animate_selected()
{
	static int i = 0;

	i = (i+1) % 10;

	_select_dash->offset = i;

	for (ItemList::iterator m = _selected_items.begin();
			m != _selected_items.end(); ++m)
		(*m)->select_tick();

	for (ConnectionList::iterator c = _selected_connections.begin();
			c != _selected_connections.end(); ++c)
		(*c)->select_tick();

	return true;
}


bool
Canvas::connection_drag_handler(GdkEvent* event)
{
	bool handled = true;

	// These are invisible, just used for making connections (while dragging)
	static boost::shared_ptr<Module> drag_module;
	static boost::shared_ptr<Port>   drag_port;

	static boost::shared_ptr<Connection> drag_connection;
	static boost::shared_ptr<Port>       snapped_port;

	static bool snapped = false;

	/*if (event->type == GDK_BUTTON_PRESS && event->button.button == 2) {
		_drag_state = SCROLL;
	} else */if (event->type == GDK_MOTION_NOTIFY && _drag_state == CONNECTION) {
		double x = event->button.x, y = event->button.y;

		if (event->motion.is_hint) {
			gint t_x;
			gint t_y;
			GdkModifierType state;
			gdk_window_get_pointer(event->motion.window, &t_x, &t_y, &state);
			x = t_x;
			y = t_y;
		}
		root()->w2i(x, y);

		if (!drag_connection) { // Havn't created the connection yet
			assert(drag_port == NULL);
			assert(_connect_port);

			drag_module = boost::shared_ptr<Module>(new Module(shared_from_this(), "", x, y));
			bool drag_port_is_input = true;
			if (_connect_port->is_input())
				drag_port_is_input = false;

			drag_port = boost::shared_ptr<Port>(new Port(drag_module, "", drag_port_is_input, _connect_port->color()));

			//drag_port->hide();
			drag_module->hide();

			drag_module->move_to(x, y);

			drag_port->property_x() = 0;
			drag_port->property_y() = 0;
			drag_port->_rect->property_x2() = 1;
			drag_port->_rect->property_y2() = 1;

			if (drag_port_is_input)
				drag_connection = boost::shared_ptr<Connection>(new Connection(
					shared_from_this(), _connect_port, drag_port,
					_connect_port->color() + 0x22222200));
			else
				drag_connection = boost::shared_ptr<Connection>(new Connection(
					shared_from_this(), drag_port, _connect_port,
					_connect_port->color() + 0x22222200));

			drag_connection->update_location();
		}

		if (snapped) {
			if (drag_connection)
				drag_connection->hide();

			boost::shared_ptr<Port> p = get_port_at(x, y);

			if (drag_connection)
				drag_connection->show();

			if (p) {
				boost::shared_ptr<Module> m = p->module().lock();
				if (m) {
					if (!p->selected()) {
						if (snapped_port)
							snapped_port->set_highlighted(false);
						p->set_highlighted(true);
						snapped_port = p;
					}
					drag_module->property_x() = m->property_x().get_value();
					drag_module->_module_box.property_x2() = m->_module_box.property_x2().get_value();
					drag_module->property_y() = m->property_y().get_value();
					drag_module->_module_box.property_y2() = m->_module_box.property_y2().get_value();
					drag_port->property_x() = p->property_x().get_value();
					drag_port->property_y() = p->property_y().get_value();
				}
			} else {  // off the port now, unsnap
				if (snapped_port)
					snapped_port->set_highlighted(false);
				snapped_port.reset();
				snapped = false;
				drag_module->property_x() = x;
				drag_module->property_y() = y;
				drag_port->property_x() = 0;
				drag_port->property_y() = 0;
				drag_port->_rect->property_x2() = 1;
				drag_port->_rect->property_y2() = 1;
			}
			drag_connection->update_location();
		} else { // not snapped to a port
			assert(drag_module);
			assert(drag_port);
			assert(_connect_port);

			// "Snap" to port, if we're on a port and it's the right direction
			if (drag_connection)
				drag_connection->hide();

			boost::shared_ptr<Port> p = get_port_at(x, y);

			if (drag_connection)
				drag_connection->show();

			if (p && p->is_input() != _connect_port->is_input()) {
				boost::shared_ptr<Module> m = p->module().lock();
				if (m) {
					p->set_highlighted(true);
					snapped_port = p;
					snapped = true;
					// Make drag module and port exactly the same size/loc as the snapped
					drag_module->move_to(m->property_x().get_value(), m->property_y().get_value());
					drag_module->set_width(m->width());
					drag_module->set_height(m->height());
					drag_port->property_x() = p->property_x().get_value();
					drag_port->property_y() = p->property_y().get_value();
					// Make the drag port as wide as the snapped port
					// so the connection coords are the same
					drag_port->_rect->property_x2() = p->_rect->property_x2().get_value();
					drag_port->_rect->property_y2() = p->_rect->property_y2().get_value();
				}
			} else {
				drag_module->property_x() = x;
				drag_module->property_y() = y - 7; // FIXME: s#7#cursor_height/2#
			}
			drag_connection->update_location();
		}
	} else if (event->type == GDK_BUTTON_RELEASE && _drag_state == CONNECTION) {
		_base_rect.ungrab(event->button.time);

		double x = event->button.x;
		double y = event->button.y;
		_base_rect.i2w(x, y);

		if (drag_connection)
			drag_connection->hide();

		boost::shared_ptr<Port> p = get_port_at(x, y);

		if (drag_connection)
			drag_connection->show();

		if (p) {
			if (p == _connect_port) {  // drag ended on same port it started on
				if (_selected_ports.empty()) {  // no active port, just activate (hilite) it
					select_port(_connect_port);
				} else {  // there is already an active port, connect it with this one
					selection_joined_with(_connect_port);
					unselect_ports();
					_connect_port.reset();
					snapped_port.reset();
				}
			} else {  // drag ended on different port
				//p->set_highlighted(false);
				ports_joined(_connect_port, p);
				unselect_ports();
				_connect_port.reset();
				snapped_port.reset();
			}
		}

		// Clean up dragging stuff

		if (_connect_port)
			_connect_port->set_highlighted(false);

		_drag_state = NOT_DRAGGING;
		drag_connection.reset();
		drag_module.reset();
		drag_port.reset();
		unselect_ports();
		snapped_port.reset();
		_connect_port.reset();
	} else {
		handled = false;
	}

	return handled;
}


boost::shared_ptr<Port>
Canvas::get_port_at(double x, double y)
{
	// Loop through every port and see if the item at these coordinates is that port
	// (if you're thinking this is slow, stupid, and disgusting, you're right)
	for (ItemList::const_iterator i = _items.begin(); i != _items.end(); ++i) {
		const boost::shared_ptr<Module> m
			= boost::dynamic_pointer_cast<Module>(*i);

		if (m && m->point_is_within(x, y))
			return m->port_at(x, y);

	}
	return boost::shared_ptr<Port>();
}


#ifdef HAVE_AGRAPH
class GVNodes : public std::map<boost::shared_ptr<Item>, Agnode_t*> {
public:
	GVNodes() : gvc(0), G(0) {}

	void cleanup() {
		gvFreeLayout(gvc, G);
		agclose (G);
		gvc = 0;
		G = 0;
	}

	GVC_t*    gvc;
	Agraph_t* G;
};
#else
class GVNodes : public std::map<boost::shared_ptr<Item>, void*> {};
#endif


GVNodes
Canvas::layout_dot(bool use_length_hints, const std::string& filename)
{
	GVNodes nodes;

#ifdef HAVE_AGRAPH
	/* FIXME: Are the strdup's here a leak?
	 * GraphViz documentation disagrees with function prototypes.
	 */

	GVC_t* gvc = gvContext();
	Agraph_t* G = agopen((char*)"g", AGDIGRAPH);

	nodes.gvc = gvc;
	nodes.G = G;

	if (_direction == HORIZONTAL)
		agraphattr(G, (char*)"rankdir", (char*)"LR");
	else
		agraphattr(G, (char*)"rankdir", (char*)"TD");

	unsigned id = 0;
	for (ItemList::const_iterator i = _items.begin(); i != _items.end(); ++i) {
		std::ostringstream ss;
		ss << "n" << id++;
		Agnode_t* node = agnode(G, strdup(ss.str().c_str()));
		if (boost::dynamic_pointer_cast<Module>(*i)) {
			ss.str("");
			ss << (*i)->width() / 96.0;
			agsafeset(node, (char*)"width", strdup(ss.str().c_str()), (char*)"");
			ss.str("");
			ss << (*i)->height() / 96.0;
			agsafeset(node, (char*)"height", strdup(ss.str().c_str()), (char*)"");
			agsafeset(node, (char*)"shape", (char*)"box", (char*)"");
			agsafeset(node, (char*)"label", (char*)(*i)->name().c_str(), (char*)"");
		} else {
			agsafeset(node, (char*)"width", (char*)"1.0", (char*)"");
			agsafeset(node, (char*)"height", (char*)"1.0", (char*)"");
			agsafeset(node, (char*)"shape", (char*)"ellipse", (char*)"");
			agsafeset(node, (char*)"label", (char*)(*i)->name().c_str(), (char*)"");
		}
		assert(node);
		nodes.insert(std::make_pair(*i, node));
	}

	for (ConnectionList::iterator i = _connections.begin(); i != _connections.end(); ++i) {
		const boost::shared_ptr<Connection> c = *i;

		boost::shared_ptr<Port> src_port = boost::dynamic_pointer_cast<Port>(c->source().lock());
		boost::shared_ptr<Port> dst_port = boost::dynamic_pointer_cast<Port>(c->dest().lock());
		boost::shared_ptr<Item> src_item = boost::dynamic_pointer_cast<Item>(c->source().lock());
		boost::shared_ptr<Item> dst_item = boost::dynamic_pointer_cast<Item>(c->dest().lock());

		GVNodes::iterator src_i = src_port
			? nodes.find(src_port->module().lock())
			: nodes.find(src_item);

		GVNodes::iterator dst_i = dst_port
			? nodes.find(dst_port->module().lock())
			: nodes.find(dst_item);

		assert(src_i != nodes.end() && dst_i != nodes.end());

		Agnode_t* src_node = src_i->second;
		Agnode_t* dst_node = dst_i->second;

		assert(src_node && dst_node);

		Agedge_t* edge = agedge(G, src_node, dst_node);

		if (use_length_hints && c->length_hint() != 0) {
			std::ostringstream len_ss;
			len_ss << c->length_hint();
			agsafeset(edge, (char*)"minlen", strdup(len_ss.str().c_str()), (char*)"1.0");
		}
	}

	// Add edges between partners to have them lined up as if they are connected
	for (GVNodes::iterator i = nodes.begin(); i != nodes.end(); ++i) {
		boost::shared_ptr<Item> partner = i->first->partner().lock();
		if (partner) {
			GVNodes::iterator p = nodes.find(partner);
			if (p != nodes.end())
				agedge(G, i->second, p->second);
		}
	}

	gvLayout(gvc, G, (char*)"dot");
	gvRender(gvc, G, (char*)"dot", fopen("/dev/null", "w"));

	if (filename != "") {
		FILE* fd = fopen(filename.c_str(), "w");
		gvRender(gvc, G, (char*)"dot", fd);
		fclose(fd);
	}
#endif

	return nodes;
}


void
Canvas::render_to_dot(const string& dot_output_filename)
{
#ifdef HAVE_AGRAPH
	GVNodes nodes = layout_dot(false, dot_output_filename);
	nodes.cleanup();
#endif
}


void
Canvas::arrange(bool use_length_hints, bool center)
{
#ifdef HAVE_AGRAPH
	GVNodes nodes = layout_dot(use_length_hints, "");

	double least_x=HUGE_VAL, least_y=HUGE_VAL, most_x=0, most_y=0;

	// Set numeric locale to POSIX for reading graphviz output with strtod
	char* locale = strdup(setlocale(LC_NUMERIC, NULL));
	setlocale(LC_NUMERIC, "POSIX");

	// Arrange to graphviz coordinates
	for (GVNodes::iterator i = nodes.begin(); i != nodes.end(); ++i) {
		const string pos   = agget(i->second, (char*)"pos");
		const string x_str = pos.substr(0, pos.find(","));
		const string y_str = pos.substr(pos.find(",")+1);
		const double x     = strtod(x_str.c_str(), NULL) * 1.25;
		const double y     = -strtod(y_str.c_str(), NULL) * 1.25;

		i->first->property_x() = x - i->first->width()/2.0;
		i->first->property_y() = y - i->first->height()/2.0;

		least_x = std::min(least_x, x);
		least_y = std::min(least_y, y);
		most_x  = std::max(most_x, x);
		most_y  = std::max(most_y, y);
	}

	// Reset numeric locale to original value
	setlocale(LC_NUMERIC, locale);
	free(locale);

	const double graph_width  = most_x - least_x;
	const double graph_height = most_y - least_y;

	//cerr << "CWH: " << _width << ", " << _height << endl;
	//cerr << "GWH: " << graph_width << ", " << graph_height << endl;

	if (graph_width + 10 > _width)
		resize(graph_width + 10, _height);

	if (graph_height + 10 > _height)
		resize(_width, graph_height + 10);

	nodes.cleanup();

	if (center) {
		move_contents_to_internal(
				_width / 2.0 - (graph_width / 2.0),
				_height / 2.0 - (graph_height / 2.0), least_x, least_y);
		scroll_to_center();
	} else {
		static const double border_width = 64.0;
		move_contents_to_internal(border_width, border_width, least_x, least_y);
		scroll_to(0, 0);
	}

	for (ItemList::const_iterator i = _items.begin(); i != _items.end(); ++i)
		(*i)->store_location();

#endif
}


void
Canvas::move_contents_to(double x, double y)
{
	double min_x=HUGE_VAL, min_y=HUGE_VAL;
	for (ItemList::const_iterator i = _items.begin(); i != _items.end(); ++i) {
		min_x = std::min(min_x, double((*i)->property_x()));
		min_y = std::min(min_y, double((*i)->property_y()));
	}
	move_contents_to_internal(x, y, min_x, min_y);
}


void
Canvas::move_contents_to_internal(double x, double y, double min_x, double min_y)
{
	for (ItemList::const_iterator i = _items.begin(); i != _items.end(); ++i)
		(*i)->move(x - min_x, y - min_y);
}


void
Canvas::resize(double width, double height)
{
	if (width != _width || height != _height) {
		_base_rect.property_x2() = _base_rect.property_x1() + width;
		_base_rect.property_y2() = _base_rect.property_y1() + height;
		_width = width;
		_height = height;
		set_scroll_region(0.0, 0.0, width, height);
	}
}


void
Canvas::resize_all_items()
{
	for (ItemList::const_iterator i = _items.begin(); i != _items.end(); ++i)
		(*i)->resize();
}


} // namespace FlowCanvas
