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

#include CONFIG_H_PATH
#include "common.hpp"
#include "PatchageCanvas.hpp"
#include "Patchage.hpp"
#include "PatchageModule.hpp"
#include "PatchagePort.hpp"

PatchageCanvas::PatchageCanvas(Patchage* app, int width, int height)
	: FlowCanvas::Canvas(width, height)
  	, _app(app)
{
}


boost::shared_ptr<Item>
PatchageCanvas::get_item(const string& name)
{
	ItemList::iterator m = _items.begin();

	for ( ; m != _items.end(); ++m)
		if ((*m)->name() == name)
			break;

	return (m == _items.end()) ? boost::shared_ptr<Item>() : *m;
}


boost::shared_ptr<PatchageModule>
PatchageCanvas::find_module(const string& name, ModuleType type)
{
	for (ItemList::iterator m = _items.begin(); m != _items.end(); ++m) {
		boost::shared_ptr<PatchageModule> pm = boost::dynamic_pointer_cast<PatchageModule>(*m);
		if (pm && pm->name() == name && (pm->type() == type || pm->type() == InputOutput)) {
			return pm;
		}
	}

	return boost::shared_ptr<PatchageModule>();
}


void
PatchageCanvas::connect(boost::shared_ptr<Connectable> port1, boost::shared_ptr<Connectable> port2)
{
	_app->connect(boost::dynamic_pointer_cast<PatchagePort>(port1), boost::dynamic_pointer_cast<PatchagePort>(port2));
}


void
PatchageCanvas::disconnect(boost::shared_ptr<Connectable> port1, boost::shared_ptr<Connectable> port2)
{
	_app->disconnect(boost::dynamic_pointer_cast<PatchagePort>(port1), boost::dynamic_pointer_cast<PatchagePort>(port2));
}


void
PatchageCanvas::status_message(const string& msg)
{
	_app->status_msg(string("[Canvas] ").append(msg));
}


boost::shared_ptr<PatchagePort>
PatchageCanvas::get_port(const string& node_name, const string& port_name)
{
	for (ItemList::iterator i = _items.begin(); i != _items.end(); ++i) {
		const boost::shared_ptr<Item> item = *i;
		const boost::shared_ptr<Module> module
			= boost::dynamic_pointer_cast<Module>(item);
		if (!module)
			continue;
		const boost::shared_ptr<Port> port = module->get_port(port_name);
		if (module->name() == node_name && port)
			return dynamic_pointer_cast<PatchagePort>(port);
	}
	
	return boost::shared_ptr<PatchagePort>();
}

boost::shared_ptr<PatchagePort>
PatchageCanvas::lookup_port_by_a2j_jack_port_name(
	const char * a2j_jack_port_name)
{
	for (ItemList::iterator i = _items.begin(); i != _items.end(); ++i)
	{
		const boost::shared_ptr<Item> item = *i;
		const boost::shared_ptr<Module> module = boost::dynamic_pointer_cast<Module>(item);
		if (!module)
			continue;

		PortVector ports = module->ports(); // copy
		for (PortVector::iterator p = ports.begin(); p != ports.end(); ++p)
		{
			boost::shared_ptr<PatchagePort> port = boost::dynamic_pointer_cast<PatchagePort>(*p);
			if (port->is_a2j_mapped && port->a2j_jack_port_name == a2j_jack_port_name)
			{
				return dynamic_pointer_cast<PatchagePort>(port);
			}
		}
	}
	
	return boost::shared_ptr<PatchagePort>();
}
