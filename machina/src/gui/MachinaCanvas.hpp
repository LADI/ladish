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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef PATCHAGEPATCHBAYAREA_H
#define PATCHAGEPATCHBAYAREA_H

#include <string>
#include <raul/SharedPtr.h>
#include <raul/WeakPtr.h>
#include <flowcanvas/FlowCanvas.h>

using namespace LibFlowCanvas;

class MachinaGUI;

class MachinaCanvas : public FlowCanvas
{
public:
	MachinaCanvas(MachinaGUI* _app, int width, int height);
	
	void connect(boost::shared_ptr<Connectable> port1,
	             boost::shared_ptr<Connectable> port2);

	void disconnect(boost::shared_ptr<Connectable> port1,
	                boost::shared_ptr<Connectable> port2);

	void status_message(const string& msg);

protected:
	bool canvas_event(GdkEvent* event);

	void item_selected(SharedPtr<Item> item);
	void item_clicked(SharedPtr<Item> item, GdkEventButton* ev);

private:
	MachinaGUI* _app;

	WeakPtr<Connectable> _last_selected;
};


#endif // PATCHAGEPATCHBAYAREA_H
