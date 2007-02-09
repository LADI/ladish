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

#ifndef FLOWCANVAS_ITEM_H
#define FLOWCANVAS_ITEM_H

#include <string>
#include <map>
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <libgnomecanvasmm.h>
#include "Port.h"

using std::string; using std::multimap;

namespace LibFlowCanvas {
	
class FlowCanvas;


/** An item on the canvas.
 *
 * \ingroup FlowCanvas
 */
class Item : public Gnome::Canvas::Group
{
public:
	Item(boost::shared_ptr<FlowCanvas> canvas,
	     const string&                 name,
	     double                        x,
	     double                        y,
	     uint32_t                      color);

	virtual ~Item() {}
	
	bool selected() const { return _selected; }
	virtual void set_selected(bool s);

	virtual void select_tick() = 0;

	virtual void move(double dx, double dy) = 0;

	virtual void zoom(double) {}
	boost::weak_ptr<FlowCanvas> canvas() const { return _canvas; }
	
	double width() const { return _width; }
	virtual void set_width(double w) = 0;
	
	double height() const { return _height; }
	virtual void set_height(double h) = 0;

	virtual void resize() = 0;

	bool        is_within(const Gnome::Canvas::Rect& rect) const;
	inline bool point_is_within(double x, double y) const;

	const string& name() const              { return _name; }
	virtual void  set_name(const string& n) { _name = n; }

	uint32_t     base_color() const         { return _color; }
	virtual void set_base_color(uint32_t c) { _color = c; }

	sigc::signal<void> signal_pointer_entered;
	sigc::signal<void> signal_pointer_exited;
	sigc::signal<void> signal_selected;
	sigc::signal<void> signal_unselected;
	
	sigc::signal<void, GdkEventButton*> signal_clicked;
	sigc::signal<void, GdkEventButton*> signal_double_clicked;

	sigc::signal<void, double, double> signal_dragged;
	sigc::signal<void, double, double> signal_dropped;

protected:
	
	virtual void on_drag(double dx, double dy);
	virtual void on_click(GdkEventButton*);
	virtual void on_double_click(GdkEventButton*);

	const boost::weak_ptr<FlowCanvas> _canvas;
	
	bool item_event(GdkEvent* event);

	std::string _name;
	double      _width;
	double      _height;
	uint32_t    _color;
	bool        _selected;
};


typedef std::multimap<std::string, boost::shared_ptr<Item> > ItemMap;


/** Returns whether or not the point @a x, @a y (world units) is within the item.
 */
inline bool
Item::point_is_within(double x, double y) const
{
	return (x > property_x() && x < property_x() + _width
			&& y > property_y() && y < property_y() + _height);
}


inline bool
Item::is_within(const Gnome::Canvas::Rect& rect) const
{
	const double x1 = rect.property_x1();
	const double y1 = rect.property_y1();
	const double x2 = rect.property_x2();
	const double y2 = rect.property_y2();

	if (x1 < x2 && y1 < y2) {
		return (property_x() > x1
			&& property_y() > y1
			&& property_x() + width() < x2
			&& property_y() + height() < y2);
	} else if (x2 < x1 && y2 < y1) {
		return (property_x() > x2
			&& property_y() > y2
			&& property_x() + width() < x1
			&& property_y() + height() < y1);
	} else if (x1 < x2 && y2 < y1) {
		return (property_x() > x1
			&& property_y() > y2
			&& property_x() + width() < x2
			&& property_y() + height() < y1);
	} else if (x2 < x1 && y1 < y2) {
		return (property_x() > x2
			&& property_y() > y1
			&& property_x() + width() < x1
			&& property_y() + height() < y2);
	} else {
		return false;
	}
}

} // namespace LibFlowCanvas

#endif // FLOWCANVAS_ITEM_H

