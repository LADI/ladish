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

#include "raul/SharedPtr.h"
#include "Patchage.h"
#include "PatchageCanvas.h"
#include "PatchageModule.h"
#include "PatchageEvent.h"
#include "JackDriver.h"

	
SharedPtr<PatchagePort>
PatchageEvent::find_port(const PortRef& ref)
{
	if (ref.type == ALSA_MIDI) {
		return _patchage->canvas()->find_port(&ref.id.alsa);
	} else {
		jack_port_t* jack_port = NULL;
		if (_patchage->jack_driver()->client())
			jack_port = jack_port_by_id(_patchage->jack_driver()->client(), ref.id.jack);

		if (!jack_port)
			return boost::shared_ptr<PatchagePort>();

		const string full_name = jack_port_name(jack_port);
		const string module_name = full_name.substr(0, full_name.find(":"));
		const string port_name = full_name.substr(full_name.find(":")+1);

		SharedPtr<PatchageModule> module = PtrCast<PatchageModule>(
				_patchage->canvas()->get_item(module_name));
		
		if (module)
			return PtrCast<PatchagePort>(module->get_port(port_name));
		else
			return boost::shared_ptr<PatchagePort>();
	}
			
	return boost::shared_ptr<PatchagePort>();
}


void
PatchageEvent::execute()
{
	//cerr << "{ EXECUTING EVENT" << endl;

	if (_type == PORT_CREATION) {
		jack_port_t* jack_port = NULL;
		if (_patchage->jack_driver()->client())
			jack_port = jack_port_by_id(_patchage->jack_driver()->client(), _port_1.id.jack);

		if (!jack_port)
			return;

		const string full_name = jack_port_name(jack_port);
		const string module_name = full_name.substr(0, full_name.find(":"));
		const string port_name = full_name.substr(full_name.find(":")+1);

		SharedPtr<PatchageModule> module = _patchage->canvas()->find_module(module_name,
				(jack_port_flags(jack_port) & JackPortIsInput) ? Input : Output);
		
		if (!module) {
			module = SharedPtr<PatchageModule>(
					new PatchageModule(_patchage, module_name, InputOutput));
			module->load_location();
			module->store_location();
			_patchage->canvas()->add_item(module);
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

		SharedPtr<PatchagePort> port = find_port(_port_1);

		if (port) {
			SharedPtr<PatchageModule> module = PtrCast<PatchageModule>(port->module().lock());
			assert(module);

			//SharedPtr<PatchagePort> removed_port = PtrCast<PatchagePort>(
					module->remove_port(port);
			//assert(removed_port == port);
			if (module->num_ports() == 0) {
				_patchage->canvas()->remove_item(module);
				module.reset();
			}
		} else {
			cerr << "Unable to find port to destroy" << endl;
		}

	} else if (_type == CONNECTION) {
		
		SharedPtr<PatchagePort> port_1 = find_port(_port_1);
		SharedPtr<PatchagePort> port_2 = find_port(_port_2);
		
		if (port_1 && port_2)
			_patchage->canvas()->add_connection(port_1, port_2, port_1->color() + 0x22222200);
		else
			cerr << "Unable to find port to connect" << endl;

	} else if (_type == DISCONNECTION) {
		
		SharedPtr<PatchagePort> port_1 = find_port(_port_1);
		SharedPtr<PatchagePort> port_2 = find_port(_port_2);

		if (port_1 && port_2)
			_patchage->canvas()->remove_connection(port_1, port_2);
		else
			cerr << "Unable to find port to disconnect" << endl;
	}

	//cerr << "}" << endl << endl;
}
