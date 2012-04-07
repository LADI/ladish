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

#ifndef FLOWCANVAS_MODULE_HPP
#define FLOWCANVAS_MODULE_HPP

#include <string>
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <libgnomecanvasmm.h>
#include "Port.hpp"
#include "Item.hpp"

namespace FlowCanvas {

class Canvas;


/** A named block (possibly) containing input and output ports.
 *
 * \ingroup FlowCanvas
 */
class Module : public Item
{
public:
	Module(boost::shared_ptr<Canvas> canvas,
	       const std::string&        name,
	       double                    x = 0,
	       double                    y = 0,
	       bool                      show_title = true,
	       bool                      show_port_labels = true);

	virtual ~Module();

	const PortVector& ports() const { return _ports; }
	PortVector&       ports()       { return _ports; }

	inline boost::shared_ptr<Port> get_port(const std::string& name) const;

	void                    add_port(boost::shared_ptr<Port> port);
	void                    remove_port(boost::shared_ptr<Port> port);
	boost::shared_ptr<Port> port_at(double x, double y);

	void zoom(double z);
	void resize();

	bool show_port_labels(bool b) { return _show_port_labels; }
	void set_show_port_labels(bool b);

	virtual void move(double dx, double dy);
	virtual void move_to(double x, double y);

	virtual void set_name(const std::string& n);

	double border_width() const { return _border_width; }
	void   set_border_width(double w);

	void select_tick();
	void set_selected(bool b);

	void set_highlighted(bool b);
	void set_border_color(uint32_t c);
	void set_base_color(uint32_t c);
	void set_default_base_color();
	void set_stacked_border(bool b);
	void set_icon(const Glib::RefPtr<Gdk::Pixbuf>& icon);

	size_t num_ports() const { return _ports.size(); }

	double empty_port_breadth() const;
	double empty_port_depth() const;

protected:
	virtual bool on_event(GdkEvent* ev);

	virtual void set_width(double w);
	virtual void set_height(double h);

	void fit_canvas();
	void measure_ports();
	void resize_horiz();
	void resize_vert();

	void port_renamed() { _port_renamed = true; }

	void embed(Gtk::Container* widget);

	PortVector _ports;

	Gnome::Canvas::Rect    _module_box;
	Gnome::Canvas::Text    _canvas_title;
	Gnome::Canvas::Rect*   _stacked_border;
	Gnome::Canvas::Pixbuf* _icon_box;
	Gtk::Container*        _embed_container;
	Gnome::Canvas::Widget* _embed_item;

	double _border_width;
	double _embed_width;
	double _embed_height;
	double _icon_size;
	double _widest_input;
	double _widest_output;
	double _title_width;
	double _title_height;
	bool   _title_visible    :1;
	bool   _port_renamed     :1;
	bool   _show_port_labels :1;

private:
	friend class Canvas;

	struct PortComparator {
		explicit PortComparator(const std::string& name) : _name(name) {}
		inline bool operator()(const boost::shared_ptr<Port> port)
			{ return (port && port->name() == _name); }
		const std::string& _name;
	};

	void embed_size_request(Gtk::Requisition* req, bool force);
};



// Performance critical functions:


/** Find a port on this module. */
inline boost::shared_ptr<Port>
Module::get_port(const std::string& port_name) const
{
	PortComparator comp(port_name);
	PortVector::const_iterator i = std::find_if(_ports.begin(), _ports.end(), comp);
	return (i != _ports.end()) ? *i : boost::shared_ptr<Port>();
}


} // namespace FlowCanvas

#endif // FLOWCANVAS_MODULE_HPP
