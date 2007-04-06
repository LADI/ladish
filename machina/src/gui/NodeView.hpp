/* This file is part of Machina.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Machina is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Machina is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef MACHINA_NODEVIEW_H
#define MACHINA_NODEVIEW_H

#include <flowcanvas/Ellipse.h>
#include "machina/Node.hpp"


class NodeView : public LibFlowCanvas::Ellipse {
public:
	NodeView(Gtk::Window*                         window,
	         SharedPtr<LibFlowCanvas::FlowCanvas> canvas,
	         SharedPtr<Machina::Node>             node,
	         const std::string&                   name,
	         double                               x,
	         double                               y);

	SharedPtr<Machina::Node> node() { return _node; }
	
	void show_label(bool show);

	void update_state();

private:
	void handle_click(GdkEventButton* ev);
	void on_double_click(GdkEventButton* ev);

	Gtk::Window*             _window;
	SharedPtr<Machina::Node> _node;
	uint32_t                 _old_color;
};


#endif // MACHINA_NODEVIEW_H
