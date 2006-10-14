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
	Module(boost::shared_ptr<FlowCanvas> canvas, const string& name, double x=0, double y=0);
	virtual ~Module();
	
	boost::weak_ptr<FlowCanvas> canvas() const { return m_canvas; }
	const PortVector&           ports()  const { return m_ports; }
	
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
	
	const string& name() const { return m_name; }
	virtual void  set_name(const string& n);

	double width() { return m_width; }
	void   set_width(double w);
	
	double height() { return m_height; }
	void   set_height(double h);

	double border_width() const { return m_border_width; }
	void   set_border_width(double w);

	bool selected() const { return m_selected; }
	void set_selected(bool b);
	
	void set_highlighted(bool b);

	int num_ports()    const     { return m_ports.size(); }
	int base_color()   const     { return 0x1F2A3CFF; }

protected:
	bool module_event(GdkEvent* event);
	
	bool is_within(const Gnome::Canvas::Rect& rect);

	virtual void on_double_click(GdkEventButton*) {}
	virtual void on_middle_click(GdkEventButton*) {}
	virtual void on_right_click(GdkEventButton*)  {}

	double m_border_width;
	double m_width;
	double m_height;
	string m_name;
	bool   m_selected;

	const boost::weak_ptr<FlowCanvas> m_canvas;
	PortVector                        m_ports;

	Gnome::Canvas::Rect m_module_box;
	Gnome::Canvas::Text m_canvas_title;

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
	PortVector::const_iterator i = std::find_if(m_ports.begin(), m_ports.end(), comp);
	return (i != m_ports.end()) ? *i : boost::shared_ptr<Port>();
}


/** Returns whether or not the point @a x, @a y (world units) is within the module.
 */
inline bool
Module::point_is_within(double x, double y)
{
	return (x > property_x() && x < property_x() + m_width
			&& y > property_y() && y < property_y() + m_height);
}


} // namespace LibFlowCanvas

#endif // FLOWCANVAS_MODULE_H
