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

#ifndef PATCHAGEPATCHBAYAREA_H
#define PATCHAGEPATCHBAYAREA_H

#include <string>
#include <raul/SharedPtr.h>
#include <raul/WeakPtr.h>
#include <flowcanvas/FlowCanvas.h>

using namespace LibFlowCanvas;

class MachinaGUI;
class NodeView;


class MachinaCanvas : public FlowCanvas
{
public:
	MachinaCanvas(MachinaGUI* app, int width, int height);
	
	void connect_node(SharedPtr<NodeView> port1,
	                  SharedPtr<NodeView> port2);

	void disconnect_node(SharedPtr<NodeView> port1,
	                     SharedPtr<NodeView> port2);

	void build(SharedPtr<Machina::Machine> machine);

protected:
	bool canvas_event(GdkEvent* event);

	void node_clicked(WeakPtr<NodeView> item, GdkEventButton* ev);

private:
	SharedPtr<NodeView> create_node_view(SharedPtr<Machina::Node> node);

	MachinaGUI* _app;

	WeakPtr<NodeView> _last_clicked;
};


#endif // PATCHAGEPATCHBAYAREA_H
