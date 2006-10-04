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

#ifndef FLOWCANVAS_PORT_H
#define FLOWCANVAS_PORT_H

#include <string>
#include <list>
#include <vector>
#include <libgnomecanvasmm.h>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

using std::string; using std::list; using std::vector;

namespace LibFlowCanvas {
	
class Connection;
class Module;


static const int PORT_LABEL_SIZE = 8000; // in thousandths of a point


/** A port on a Module.
 *
 * This is a group that contains both the label and rectangle for a port.
 *
 * \ingroup FlowCanvas
 */
class Port : public Gnome::Canvas::Group
{
public:
	Port(boost::shared_ptr<Module> module, const string& name, bool is_input, int color);
	virtual ~Port();
	
	void add_connection(boost::shared_ptr<Connection> c);
	void remove_connection(boost::shared_ptr<Connection> c);
	void move_connections();
	void raise_connections();
	void disconnect_all();
	
	Gnome::Art::Point connection_point();

	boost::weak_ptr<Module>             module() const { return m_module; }
	list<boost::weak_ptr<Connection> >& connections()  { return m_connections; }
	
	void set_fill_color(uint32_t c) { m_rect.property_fill_color_rgba() = c; }
	
	void set_highlighted(bool b);
	
	void zoom(float z);

	void popup_menu(guint button, guint32 activate_time) {
		m_menu.popup(button, activate_time);
	}

	double width() const { return m_width; }
	void   set_width(double w);
	
	double border_width() const { return m_border_width; }
	void   set_border_width(double w);

	const string& name() const { return m_name; }
	virtual void  set_name(const string& n);
	
	bool   is_input()  const { return m_is_input; }
	bool   is_output() const { return !m_is_input; }
	int    color()     const { return m_color; }
	double height()    const { return m_height; }

	bool operator==(const string& name) { return (m_name == name); }

	sigc::signal<void, string> signal_renamed;

protected:
	friend class FlowCanvas;

	boost::weak_ptr<Module> m_module;
	string                  m_name;
	bool                    m_is_input;
	double                  m_width;
	double                  m_height;
	double                  m_border_width;
	int                     m_color;
	
	list<boost::weak_ptr<Connection> > m_connections; // needed for dragging
	
	Gnome::Canvas::Text m_label;
	Gnome::Canvas::Rect m_rect;
	Gtk::Menu           m_menu;
};

typedef vector<boost::shared_ptr<Port> > PortVector;


} // namespace LibFlowCanvas

#endif // FLOWCANVAS_PORT_H
