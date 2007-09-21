/* This file is part of Patchage.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
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
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef PATCHAGEMODULE_H
#define PATCHAGEMODULE_H

#include <string>
#include <libgnomecanvasmm.h>
#ifdef HAVE_ALSA
#include <alsa/asoundlib.h>
#endif
#include <flowcanvas/Canvas.hpp>
#include <flowcanvas/Module.hpp>
#include "PatchageCanvas.hpp"
#include "StateManager.hpp"
#include "PatchagePort.hpp"

using std::string; using std::list;

using namespace FlowCanvas;

class PatchageModule : public Module
{
public:
	PatchageModule(Patchage* app, const string& title, ModuleType type, double x=0, double y=0)
	: Module(app->canvas(), title, x, y),
	  _app(app),
	  _type(type)
	{
		_menu = new Gtk::Menu();
		Gtk::Menu::MenuList& items = _menu->items();
		if (type == InputOutput) {
			items.push_back(Gtk::Menu_Helpers::MenuElem("Split",
				sigc::mem_fun(this, &PatchageModule::split)));
		} else {
			items.push_back(Gtk::Menu_Helpers::MenuElem("Join",
				sigc::mem_fun(this, &PatchageModule::join)));
		}
		items.push_back(Gtk::Menu_Helpers::MenuElem("Disconnect All",
			sigc::mem_fun(this, &PatchageModule::menu_disconnect_all)));

		//signal_clicked.connect(sigc::mem_fun(this, &PatchageModule::on_click));
	}

	virtual ~PatchageModule() { delete _menu; }
	
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
		boost::shared_ptr<Canvas> canvas = _canvas.lock();
		if (!canvas)
			return;

		Coord loc = _app->state_manager()->get_module_location(_name, _type);

		//cerr << "******" << _name << " MOVING TO (" << loc.x << "," << loc.y << ")" << endl;

		if (loc.x != -1)
			move_to(loc.x, loc.y);
		else
			move_to((canvas->width()/2) - 100 + rand() % 400,
			         (canvas->height()/2) - 100 + rand() % 400);
	}

	void split() {
		assert(_type == InputOutput);
		_app->state_manager()->set_module_split(_name, true);
		_app->refresh();
	}

	void join() {
		assert(_type != InputOutput);
		_app->state_manager()->set_module_split(_name, false);
		_app->refresh();
	}
		
	virtual void store_location() {
		Coord loc = { property_x().get_value(), property_y().get_value() };
		_app->state_manager()->set_module_location(_name, _type, loc);
	}
	
	virtual void show_dialog() {}
	
	virtual void menu_disconnect_all() {
		for (PortVector::iterator p = _ports.begin(); p != _ports.end(); ++p)
			(*p)->disconnect_all();
	}

	ModuleType type() { return _type; }

protected:
	Patchage*  _app;
	ModuleType _type;
};


#endif // PATCHAGEMODULE_H
