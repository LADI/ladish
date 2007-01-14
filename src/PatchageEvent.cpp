/* This file is part of Patchage.  Copyright (C) 2005 Dave Robillard.
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

#include "raul/SharedPtr.h"
#include "Patchage.h"
#include "PatchageFlowCanvas.h"
#include "PatchageModule.h"
#include "PatchageEvent.h"
#include "JackDriver.h"

void
PatchageEvent::execute()
{
	//cerr << "{ EXECUTING EVENT" << endl;

	jack_port_t* const jack_port = jack_port_by_id(_patchage->jack_driver()->client(), _port_id);
	const string full_name = jack_port_name(jack_port);
	const string module_name = full_name.substr(0, full_name.find(":"));
	const string port_name = full_name.substr(full_name.find(":")+1);

	SharedPtr<PatchageModule> module = PtrCast<PatchageModule>(
		_patchage->canvas()->get_module(module_name));

	if (_type == PORT_CREATION) {

		if (!module) {
			module = boost::shared_ptr<PatchageModule>(
					new PatchageModule(_patchage, module_name, InputOutput));
			module->load_location();
			module->store_location();
			_patchage->canvas()->add_module(module);
			module->show();
		}
	
		boost::shared_ptr<PatchagePort> port = PtrCast<PatchagePort>(
			module->get_port(port_name));
		if (!port) {
			port = _patchage->jack_driver()->create_port(module, jack_port);
			module->add_port(port);
			module->resize();
		}

	} else if (_type == PORT_DESTRUCTION) {

		if (!module) {
			cerr << "Unable to find module for port " << full_name << endl;
			return;
		}

		boost::shared_ptr<PatchagePort> port = PtrCast<PatchagePort>(
				module->remove_port(full_name.substr(full_name.find(":")+1)));

		if (!port)
			cerr << "Destroy port: Unable to find port " << full_name << endl;
		else {
			//cerr << "Destroyed port " << full_name << endl;
			port->hide(); 
			port.reset(); // FIXME: leak?
		}

	}

	if (module->num_ports() == 0) {
		_patchage->canvas()->remove_module(module->name()); // FIXME: slow
		module->hide();
		module.reset();
	}

	//cerr << "}" << endl << endl;
}
