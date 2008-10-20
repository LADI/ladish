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

#ifndef FLOWCANVAS_CANVAS_HPP
#define FLOWCANVAS_CANVAS_HPP

#include <list>
#include <boost/enable_shared_from_this.hpp>
#include <boost/utility.hpp>
#include <libgnomecanvasmm.h>
#include "flowcanvas/Connection.hpp"
#include "flowcanvas/Module.hpp"
#include "flowcanvas/Item.hpp"


/** FlowCanvas namespace, everything is defined under this.
 *
 * \ingroup FlowCanvas
 */
namespace FlowCanvas {
	
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
class Canvas : boost::noncopyable
             , public boost::enable_shared_from_this<Canvas>
             , public /*CANVASBASE*/Gnome::Canvas::CanvasAA
// (CANVASBASE is a hook for a sed script in configure.ac)
{
public:
	Canvas(double width, double height);
	virtual	~Canvas();

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

	void lock(bool l);
	bool locked() const { return _locked; }

	double get_zoom() { return _zoom; }
	void   set_zoom(double pix_per_unit);
	void   zoom_full();

	void render_to_dot(const std::string& filename);
	virtual void arrange(bool use_length_hints=false);

	double width() const  { return _width; }
	double height() const { return _height; }

	void resize(double width, double height);
	void resize_all_items();

	/** Dash applied to selected items.
	 * Set an object's property_dash() to this for the "rubber band" effect */
	ArtVpathDash* select_dash() { return _select_dash; }
	
	/** Make a connection.  Should be overridden by an implementation to do something. */
	virtual void connect(boost::shared_ptr<Connectable> /*tail*/,
	                     boost::shared_ptr<Connectable> /*head*/) {}
	
	/** Disconnect two ports.  Should be overridden by an implementation to do something */
	virtual void disconnect(boost::shared_ptr<Connectable> /*tail*/,
	                        boost::shared_ptr<Connectable> /*head*/) {}

protected:
	ItemList                                   _items;  ///< All items on this canvas
	ConnectionList                             _connections;  ///< All connections on this canvas
	std::list< boost::shared_ptr<Item> >       _selected_items;  ///< All currently selected modules
	std::list< boost::shared_ptr<Connection> > _selected_connections;  ///< All currently selected connections

	virtual bool canvas_event(GdkEvent* event);
	virtual bool frame_event(GdkEvent* ev);
	
private:

	friend class Module;
	bool port_event(GdkEvent* event, boost::weak_ptr<Port> port);

	void remove_connection(boost::shared_ptr<Connection> c);
	bool are_connected(boost::shared_ptr<const Connectable> tail,
	                   boost::shared_ptr<const Connectable> head);
	
	void select_port(boost::shared_ptr<Port> p, bool unique = false);
	void select_port_toggle(boost::shared_ptr<Port> p, int mod_state);
	void unselect_port(boost::shared_ptr<Port> p);
	void selection_joined_with(boost::shared_ptr<Port> port);
	void join_selection();

	boost::shared_ptr<Port> get_port_at(double x, double y);

	bool         scroll_drag_handler(GdkEvent* event);
	virtual bool select_drag_handler(GdkEvent* event);
	virtual bool connection_drag_handler(GdkEvent* event);
	
	void ports_joined(boost::shared_ptr<Port> port1, boost::shared_ptr<Port> port2);
	bool animate_selected();

	void scroll_to_center();
	void on_parent_changed(Gtk::Widget* old_parent);
	sigc::connection _parent_event_connection;

	typedef std::list< boost::shared_ptr<Port> > SelectedPorts;
	SelectedPorts _selected_ports; ///< Selected ports (hilited red)
	boost::shared_ptr<Port> _connect_port;  ///< Port for which a connection is being made (if applicable)
	boost::shared_ptr<Port> _last_selected_port;
	
	double _zoom;   ///< Current zoom level
	double _width;  
	double _height; 

	enum DragState { NOT_DRAGGING, CONNECTION, SCROLL, SELECT };
	DragState      _drag_state;
	
	bool _remove_objects; // flag to avoid removing objects from destructors when unnecessary
	bool _locked;

	Gnome::Canvas::Rect  _base_rect;   ///< Background
	Gnome::Canvas::Rect* _select_rect; ///< Rectangle for drag selection
	ArtVpathDash*        _select_dash; ///< Animated selection dash style
};


} // namespace FlowCanvas

#endif // FLOWCANVAS_CANVAS_HPP
