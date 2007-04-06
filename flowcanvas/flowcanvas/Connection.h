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

#ifndef FLOWCANVAS_CONNECTION_H
#define FLOWCANVAS_CONNECTION_H

#include <list>
#include <boost/weak_ptr.hpp>
#include <libgnomecanvasmm.h>
#include <libgnomecanvasmm/bpath.h>
#include <libgnomecanvasmm/path-def.h>

using std::list;

namespace LibFlowCanvas {

class FlowCanvas;
class Connectable;


/** A connection (line) between two canvas objects.
 *
 * \ingroup FlowCanvas
 */
class Connection : public Gnome::Canvas::Group
{
public:
	Connection(boost::shared_ptr<FlowCanvas>  canvas,
	           boost::shared_ptr<Connectable> source,
	           boost::shared_ptr<Connectable> dest,
	           uint32_t                       color,
			   bool                           show_arrow_head = false);

	virtual ~Connection() {}
	
	virtual void move(double /*dx*/, double /*dy*/)
	{ /* ignore, src/dst take care of it */ }
	
	virtual void zoom(double);

	bool flagged() const     { return _flag; }
	void set_flagged(bool b) { _flag = b; }
	
	bool selected() const { return _selected; }
	void set_selected(bool b);
	
	virtual double length_hint() const { return 0.0; }

	void set_label(const std::string& str);
	
	void set_highlighted(bool b);
	void raise_to_top();

	void select_tick();

	const boost::weak_ptr<Connectable> source() const { return _source; }
	const boost::weak_ptr<Connectable> dest() const   { return _dest; }

protected:
	friend class FlowCanvas;

	friend class Connectable;
	void update_location();
	
	const boost::weak_ptr<FlowCanvas>  _canvas;
	const boost::weak_ptr<Connectable> _source;
	const boost::weak_ptr<Connectable> _dest;
	uint32_t                           _color;
	bool                               _selected;
	bool                               _flag;
	bool                               _show_arrowhead;

	Gnome::Canvas::Bpath _bpath;
	//Glib::RefPtr<Gnome::Canvas::PathDef> _path;
	GnomeCanvasPathDef* _path;

	Gnome::Canvas::Text* _label;
};

typedef list<boost::shared_ptr<Connection> > ConnectionList;


} // namespace LibFlowCanvas

#endif // FLOWCANVAS_CONNECTION_H
