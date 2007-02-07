/* This file is part of Patchage.  Copyright (C) 2004 Dave Robillard.
 * 
 * Patchage is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Patchage is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PATCHAGEMODULE_H
#define PATCHAGEMODULE_H

#include <string>
#include <libgnomecanvasmm.h>
#ifdef HAVE_ALSA
#include <alsa/asoundlib.h>
#endif
#include <flowcanvas/FlowCanvas.h>
#include <flowcanvas/Module.h>
#include "PatchageFlowCanvas.h"
#include "StateManager.h"
#include "PatchagePort.h"

using std::string; using std::list;

using namespace LibFlowCanvas;

class PatchageModule : public Module
{
public:
	PatchageModule(Patchage* app, const string& title, ModuleType type, double x=0, double y=0)
	: Module(app->canvas(), title, x, y),
	  _app(app),
	  _type(type)
	{
		Gtk::Menu::MenuList& items = _menu.items();
		if (type == InputOutput) {
			items.push_back(Gtk::Menu_Helpers::MenuElem("Split",
				sigc::mem_fun(this, &PatchageModule::split)));
		} else {
			items.push_back(Gtk::Menu_Helpers::MenuElem("Join",
				sigc::mem_fun(this, &PatchageModule::join)));
		}
		items.push_back(Gtk::Menu_Helpers::MenuElem("Disconnect All",
			sigc::mem_fun(this, &PatchageModule::menu_disconnect_all)));
	}

	virtual ~PatchageModule() { }
	
	/*virtual void add_patchage_port(const string& port_name, bool is_input, PortType type)
	{
		new PatchagePort(this, type, port_name, is_input, _app->state_manager()->get_port_color(type));

		resize();
	}

	virtual void add_patchage_port(const string& port_name, bool is_input, PortType type, const snd_seq_addr_t addr)
	{
		PatchagePort* port = new PatchagePort(this, type, port_name, is_input,
			_app->state_manager()->get_port_color(type));

		port->alsa_addr(addr);

		resize();
	}*/


	virtual void load_location() {
		Coord loc = _app->state_manager()->get_module_location(_name, _type);

		//cerr << "******" << _name << " MOVING TO (" << loc.x << "," << loc.y << ")" << endl;

		if (loc.x != -1)
			move_to(loc.x, loc.y);
		else
			move_to((_canvas.lock()->width()/2) - 100 + rand() % 400,
			         (_canvas.lock()->height()/2) - 100 + rand() % 400);
	}

	void split() {
		assert(_type == InputOutput);
		_app->state_manager()->set_module_split(_name, true);
		_app->queue_refresh();
	}

	void join() {
		assert(_type != InputOutput);
		_app->state_manager()->set_module_split(_name, false);
		_app->queue_refresh();
	}
		
	virtual void store_location() {
		Coord loc = { property_x().get_value(), property_y().get_value() };
		_app->state_manager()->set_module_location(_name, _type, loc);
	}
	
	virtual void show_dialog() {}
	virtual void on_right_click(GdkEventButton* ev) { _menu.popup(ev->button, ev->time); }
	virtual void menu_disconnect_all() {
		for (PortVector::iterator p = _ports.begin(); p != _ports.end(); ++p)
			(*p)->disconnect_all();
	}

	ModuleType type() { return _type; }

protected:
	Patchage*  _app;
	Gtk::Menu  _menu;
	ModuleType _type;
};


#endif // PATCHAGEMODULE_H
