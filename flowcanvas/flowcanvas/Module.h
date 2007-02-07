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

#ifndef FLOWCANVAS_MODULE_H
#define FLOWCANVAS_MODULE_H

#include <string>
#include <map>
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <libgnomecanvasmm.h>
#include "Port.h"

using std::string; using std::multimap;

namespace LibFlowCanvas {
	
class FlowCanvas;


/** A named block (possibly) containing input and output ports.
 *
 * \ingroup FlowCanvas
 */
class Module : public Gnome::Canvas::Group
{
public:
	Module(boost::shared_ptr<FlowCanvas> canvas, const string& name, double x=0, double y=0, bool show_title=true);
	virtual ~Module();
	
	boost::weak_ptr<FlowCanvas> canvas() const { return _canvas; }
	const PortVector&           ports()  const { return _ports; }
	
	inline boost::shared_ptr<Port> get_port(const string& name) const;
	
	void                    add_port(boost::shared_ptr<Port> port);
	void                    remove_port(boost::shared_ptr<Port> port);
	boost::shared_ptr<Port> remove_port(const string& name);
	boost::shared_ptr<Port> port_at(double x, double y);

	bool point_is_within(double x, double y);

	void zoom(double z);
	void resize();
	
	void         move(double dx, double dy);
	virtual void move_to(double x, double y);

	virtual void load_location()  {}
	virtual void store_location() {}
	
	const string& name() const { return _name; }
	virtual void  set_name(const string& n);

	double width() { return _width; }
	void   set_width(double w);
	
	double height() { return _height; }
	void   set_height(double h);

	double border_width() const { return _border_width; }
	void   set_border_width(double w);

	bool selected() const { return _selected; }
	void set_selected(bool b);
	
	void set_highlighted(bool b);

	int num_ports()    const     { return _ports.size(); }
	int base_color()   const     { return 0x1F2A3CFF; }

protected:
	bool module_event(GdkEvent* event);
	
	bool is_within(const Gnome::Canvas::Rect& rect);

	virtual void on_double_click(GdkEventButton*) {}
	virtual void on_middle_click(GdkEventButton*) {}
	virtual void on_right_click(GdkEventButton*)  {}

	double _border_width;
	double _width;
	double _height;
	string _name;
	bool   _selected;
	bool   _title_visible;

	const boost::weak_ptr<FlowCanvas> _canvas;
	PortVector                        _ports;

	Gnome::Canvas::Rect _module_box;
	Gnome::Canvas::Text _canvas_title;

private:
	friend class Port;
	friend class FlowCanvas;
	friend class Connection;
	
	// For connection drawing
	double port_connection_point_offset(boost::shared_ptr<Port> port);
	double port_connection_points_range();
	

	struct PortComparator {
		PortComparator(const string& name) : _name(name) {}
		inline bool operator()(const boost::shared_ptr<Port> port)
			{ return (port && port->name() == _name); }
		const string& _name;
	};
};


typedef multimap<string,boost::shared_ptr<Module> > ModuleMap;


// Performance critical functions:


/** Find a port on this module.
 *
 * TODO: Make this faster.
 */
inline boost::shared_ptr<Port>
Module::get_port(const string& port_name) const
{
	PortComparator comp(port_name);
	PortVector::const_iterator i = std::find_if(_ports.begin(), _ports.end(), comp);
	return (i != _ports.end()) ? *i : boost::shared_ptr<Port>();
}


/** Returns whether or not the point @a x, @a y (world units) is within the module.
 */
inline bool
Module::point_is_within(double x, double y)
{
	return (x > property_x() && x < property_x() + _width
			&& y > property_y() && y < property_y() + _height);
}


} // namespace LibFlowCanvas

#endif // FLOWCANVAS_MODULE_H
