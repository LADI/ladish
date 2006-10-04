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

#ifndef FLOWCANVAS_FLOWCANVAS_H
#define FLOWCANVAS_FLOWCANVAS_H

#include <string>
#include <list>
#include <boost/enable_shared_from_this.hpp>
#include <libgnomecanvasmm.h>
#include "Connection.h"
#include "Module.h"

using std::string;
using std::list;


/** FlowCanvas namespace, everything is defined under this.
 *
 * \ingroup FlowCanvas
 */
namespace LibFlowCanvas {
	
class Port;
class Module;


/** \defgroup FlowCanvas Canvas widget for dataflow systems.
 *
 * A generic dataflow widget using libgnomecanvas.
 */


/** The 'master' canvas widget which contains all other objects.
 *
 * Applications must override some virtual methods to make the widget actually
 * do anything (ie connect).
 *
 * \ingroup FlowCanvas
 */
class FlowCanvas : public boost::enable_shared_from_this<FlowCanvas>
                 , public /*CANVASBASE*/Gnome::Canvas::CanvasAA
// (CANVASBASE is a hook for a sed script in configure.ac)
{
public:
	FlowCanvas(double width, double height);
	virtual	~FlowCanvas();

	void destroy();
		
	void                      add_module(boost::shared_ptr<Module> m);
	boost::shared_ptr<Module> remove_module(const string& name);
	boost::shared_ptr<Module> get_module(const string& name);
	
	boost::shared_ptr<Port> get_port(const string& module_name,
                                     const string& port_name);

	boost::shared_ptr<Connection> get_connection(const boost::shared_ptr<Port> src,
	                                             const boost::shared_ptr<Port> dst);
	
	bool add_connection(boost::shared_ptr<Port> port1,
	                    boost::shared_ptr<Port> port2);
	
	bool add_connection(boost::shared_ptr<Connection> connection);
	
	boost::shared_ptr<Connection> remove_connection(boost::shared_ptr<Port> port1,
	                                                boost::shared_ptr<Port> port2);
	
	void destroy_all_connections();
	
	void flag_all_connections();
	void destroy_all_flagged_connections();

	void set_default_placement(boost::shared_ptr<Module> m);
	
	void clear_selection();
	void select_module(const string& name);
	void select_module(boost::shared_ptr<Module> module);
	void unselect_ports();
	void unselect_module(const string& name);
	void unselect_module(boost::shared_ptr<Module> module);
	void unselect_connection(Connection* c);
	
	ModuleMap&                            modules()              { return m_modules; }
	list<boost::shared_ptr<Module> >&     selected_modules()     { return m_selected_modules; }
	list<boost::shared_ptr<Connection> >& selected_connections() { return m_selected_connections; }

	double get_zoom() { return m_zoom; }
	void   set_zoom(double pix_per_unit);
	void   zoom_full();

	double width() const  { return m_width; }
	double height() const { return m_height; }

	
	/** Dash applied to selected items.
	 * Set an object's property_dash() to this for the "rubber band" effect */
	ArtVpathDash* const select_dash() { return m_select_dash; }
	
	/** Make a connection.  Should be overridden by an implementation to do something. */
	virtual void connect(boost::shared_ptr<Port> src_port, boost::shared_ptr<Port> dst_port) = 0;
	
	/** Disconnect two ports.  Should be overridden by an implementation to do something */
	virtual void disconnect(boost::shared_ptr<Port> src_port, boost::shared_ptr<Port> dst_port) = 0;

protected:
	ModuleMap		                     m_modules;              ///< All modules on this canvas
	ConnectionList	                     m_connections;          ///< All connections on this canvas
	list<boost::shared_ptr<Module> >     m_selected_modules;     ///< All currently selected modules
	list<boost::shared_ptr<Connection> > m_selected_connections; ///< All currently selected connections

	virtual bool canvas_event(GdkEvent* event);
	
private:
	// Prevent copies (undefined)
	FlowCanvas(const FlowCanvas& copy);
	FlowCanvas& operator=(const FlowCanvas& other);

	friend class Module;
	bool rename_module(const string& old_name, const string& new_name);

	friend class Connection;
	void        remove_connection(boost::shared_ptr<Connection> c);

	friend class Port;
	virtual bool port_event(GdkEvent* event, boost::weak_ptr<Port> port);

	bool are_connected(boost::shared_ptr<const Port> port1, boost::shared_ptr<const Port> port2);
	void selected_port(boost::shared_ptr<Port> p);
	boost::shared_ptr<Port> selected_port() { return m_selected_port; }
	
	boost::shared_ptr<Port> get_port_at(double x, double y);

	//bool scroll_drag_handler(GdkEvent* event);
	virtual bool select_drag_handler(GdkEvent* event);
	virtual bool connection_drag_handler(GdkEvent* event);
	
	void ports_joined(boost::shared_ptr<Port> port1, boost::shared_ptr<Port> port2);
	bool animate_selected();

	boost::shared_ptr<Port> m_selected_port; ///< Selected port (hilited red from clicking once)
	boost::shared_ptr<Port> m_connect_port;  ///< Port for which a connection is being made (if applicable)
	
	double m_zoom;   ///< Current zoom level
	double m_width;  
	double m_height; 

	enum DragState { NOT_DRAGGING, CONNECTION, SCROLL, SELECT };
	DragState      m_drag_state;
	
	bool m_remove_objects; // flag to avoid removing objects from destructors when unnecessary

	Gnome::Canvas::Rect  m_base_rect;   ///< Background
	Gnome::Canvas::Rect* m_select_rect; ///< Rectangle for drag selection
	ArtVpathDash*        m_select_dash; ///< Animated selection dash style
};


} // namespace LibFlowCanvas

#endif // FLOWCANVAS_FLOWCANVAS_H
