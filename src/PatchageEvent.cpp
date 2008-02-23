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
#include <raul/SharedPtr.hpp>
#include "Patchage.hpp"
#include "PatchageCanvas.hpp"
#include "PatchageModule.hpp"
#include "PatchageEvent.hpp"
#include "Driver.hpp"

using namespace std;


void
PatchageEvent::execute(Patchage* patchage)
{
	if (_type == REFRESH) {
		patchage->refresh();
	} else if (_type == CLIENT_CREATION) {
		// No empty modules (for now)
		free(_str);
		_str = NULL;
	} else if (_type == CLIENT_DESTRUCTION) {
		SharedPtr<PatchageModule> module = PtrCast<PatchageModule>(
				patchage->canvas()->find_module(_str, InputOutput));

		if (module) {
			patchage->canvas()->remove_item(module);
			module.reset();
		}

		free(_str);
		_str = NULL;

	} else if (_type == PORT_CREATION) {
		
		if ( ! _driver->create_port_view(patchage, _port_1)) {
			cerr << "Unable to create port view (already exists?" << endl;
		}

	} else if (_type == PORT_DESTRUCTION) {

		SharedPtr<PatchagePort> port = _driver->find_port_view(patchage, _port_1);

		if (port) {
			SharedPtr<PatchageModule> module = PtrCast<PatchageModule>(port->module().lock());
			assert(module);

			module->remove_port(port);
			port.reset();
			
			// No empty modules (for now)
			if (module->num_ports() == 0) {
				patchage->canvas()->remove_item(module);
				module.reset();
			} else {
				module->resize();
			}

		} else {
			cerr << "Unable to find port to destroy" << endl;
		}

	} else if (_type == CONNECTION) {
		
		SharedPtr<PatchagePort> port_1 = _driver->find_port_view(patchage, _port_1);
		SharedPtr<PatchagePort> port_2 = _driver->find_port_view(patchage, _port_2);
		
		if (port_1 && port_2)
			patchage->canvas()->add_connection(port_1, port_2, port_1->color() + 0x22222200);
		else
			cerr << "Unable to find port to connect" << endl;

	} else if (_type == DISCONNECTION) {
		
		SharedPtr<PatchagePort> port_1 = _driver->find_port_view(patchage, _port_1);
		SharedPtr<PatchagePort> port_2 = _driver->find_port_view(patchage, _port_2);

		if (port_1 && port_2)
			patchage->canvas()->remove_connection(port_1, port_2);
		else
			cerr << "Unable to find port to disconnect" << endl;
	}

	//cerr << "}" << endl << endl;
}

