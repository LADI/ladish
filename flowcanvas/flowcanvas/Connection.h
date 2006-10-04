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

#ifndef FLOWCANVAS_CONNECTION_H
#define FLOWCANVAS_CONNECTION_H

#include <list>
#include <boost/weak_ptr.hpp>
#include <libgnomecanvasmm.h>
#include <libgnomecanvasmm/bpath.h>
#include <libgnomecanvasmm/path-def.h>
#include "Port.h"

using std::list;

namespace LibFlowCanvas {

class FlowCanvas;


/** A connection (line) between two Ports.
 *
 * \ingroup FlowCanvas
 */
class Connection : public Gnome::Canvas::Bpath
{
public:
	Connection(boost::shared_ptr<FlowCanvas> canvas,
	           boost::shared_ptr<Port>       source,
	           boost::shared_ptr<Port>       dest);

	virtual ~Connection() {}
	
	bool flagged() const     { return m_flag; }
	void set_flagged(bool b) { m_flag = b; }
	
	bool selected() const { return m_selected; }
	void set_selected(bool b);
	
	void set_highlighted(bool b);
	
	const boost::weak_ptr<Port> source() const { return m_source; }
	const boost::weak_ptr<Port> dest() const   { return m_dest; }

private:
	friend class FlowCanvas;
	friend class Port;

	void update_location();
	
	const boost::weak_ptr<FlowCanvas> m_canvas;
	const boost::weak_ptr<Port>       m_source;
	const boost::weak_ptr<Port>       m_dest;
	int                               m_color;
	bool                              m_selected;
	bool                              m_flag;

	//Glib::RefPtr<Gnome::Canvas::PathDef> m_path;
	GnomeCanvasPathDef* m_path;
};

typedef list<boost::shared_ptr<Connection> > ConnectionList;


} // namespace LibFlowCanvas

#endif // FLOWCANVAS_CONNECTION_H
