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

#ifndef FLOWCANVAS_FLOWCANVAS_H
#define FLOWCANVAS_FLOWCANVAS_H

#include <list>
#include <boost/enable_shared_from_this.hpp>
#include <boost/utility.hpp>
#include <libgnomecanvasmm.h>
#include "Connection.h"
#include "Module.h"
#include "Item.h"

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
class FlowCanvas : boost::noncopyable
                 , public boost::enable_shared_from_this<FlowCanvas>
                 , public /*CANVASBASE*/Gnome::Canvas::CanvasAA
// (CANVASBASE is a hook for a sed script in configure.ac)
{
public:
	FlowCanvas(double width, double height);
	virtual	~FlowCanvas();

	void destroy();
		
	void add_item(boost::shared_ptr<Item> i);
	bool remove_item(boost::shared_ptr<Item> i);

	boost::shared_ptr<Connection>
	get_connection(boost::shared_ptr<Connectable> tail,
	               boost::shared_ptr<Connectable> head) const;
	
	bool add_connection(boost::shared_ptr<Connectable> tail,
	                    boost::shared_ptr<Connectable> head,
	                    uint32_t                       color);
	
	bool add_connection(boost::shared_ptr<Connection> connection);
	
	boost::shared_ptr<Connection> remove_connection(boost::shared_ptr<Connectable> tail,
	                                                boost::shared_ptr<Connectable> head);
	
	void destroy_all_connections();
	
	void flag_all_connections();
	void destroy_all_flagged_connections();

	void set_default_placement(boost::shared_ptr<Module> m);
	
	void clear_selection();
	void select_item(boost::shared_ptr<Item> item);
	void unselect_ports();
	void unselect_item(boost::shared_ptr<Item> item);
	void unselect_connection(Connection* c);
	
	ItemList&       items()                { return _items; }
	ItemList&       selected_items()       { return _selected_items; }
	ConnectionList& connections()          { return _connections; }
	ConnectionList& selected_connections() { return _selected_connections; }

	double get_zoom() { return _zoom; }
	void   set_zoom(double pix_per_unit);
	void   zoom_full();

	void render_to_dot(const string& filename);
	void arrange();

	double width() const  { return _width; }
	double height() const { return _height; }

	void resize(double width, double height);

	/** Dash applied to selected items.
	 * Set an object's property_dash() to this for the "rubber band" effect */
	ArtVpathDash* const select_dash() { return _select_dash; }
	
	/** Make a connection.  Should be overridden by an implementation to do something. */
	virtual void connect(boost::shared_ptr<Connectable> /*tail*/,
	                     boost::shared_ptr<Connectable> /*head*/) {}
	
	/** Disconnect two ports.  Should be overridden by an implementation to do something */
	virtual void disconnect(boost::shared_ptr<Connectable> /*tail*/,
	                        boost::shared_ptr<Connectable> /*head*/) {}

protected:
	ItemList                             _items;  ///< All items on this canvas
	ConnectionList                       _connections;  ///< All connections on this canvas
	list<boost::shared_ptr<Item> >       _selected_items;  ///< All currently selected modules
	list<boost::shared_ptr<Connection> > _selected_connections;  ///< All currently selected connections

	virtual bool canvas_event(GdkEvent* event);
	
private:
	friend class Module;
	friend class Item;

	friend class Connection;
	void        remove_connection(boost::shared_ptr<Connection> c);

	friend class Port;
	virtual bool port_event(GdkEvent* event, boost::weak_ptr<Port> port);

	bool are_connected(boost::shared_ptr<const Connectable> tail,
	                   boost::shared_ptr<const Connectable> head);
	
	void selected_port(boost::shared_ptr<Port> p);
	boost::shared_ptr<Port> selected_port() { return _selected_port; }
	
	boost::shared_ptr<Port> get_port_at(double x, double y);

	//bool scroll_drag_handler(GdkEvent* event);
	virtual bool select_drag_handler(GdkEvent* event);
	virtual bool connection_drag_handler(GdkEvent* event);
	
	void ports_joined(boost::shared_ptr<Port> port1, boost::shared_ptr<Port> port2);
	bool animate_selected();

	void scroll_to_center();

	boost::shared_ptr<Port> _selected_port; ///< Selected port (hilited red from clicking once)
	boost::shared_ptr<Port> _connect_port;  ///< Port for which a connection is being made (if applicable)
	
	double _zoom;   ///< Current zoom level
	double _width;  
	double _height; 

	enum DragState { NOT_DRAGGING, CONNECTION, SCROLL, SELECT };
	DragState      _drag_state;
	
	bool _remove_objects; // flag to avoid removing objects from destructors when unnecessary

	Gnome::Canvas::Rect  _base_rect;   ///< Background
	Gnome::Canvas::Rect* _select_rect; ///< Rectangle for drag selection
	ArtVpathDash*        _select_dash; ///< Animated selection dash style
};


} // namespace LibFlowCanvas

#endif // FLOWCANVAS_FLOWCANVAS_H
