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

#ifndef JACKDRIVER_H
#define JACKDRIVER_H

#include <iostream>
#include <string>
#include <boost/shared_ptr.hpp>
#include <jack/jack.h>
#include <raul/Mutex.h>
#include "Driver.h"
class Patchage;
class PatchageFlowCanvas;
class PatchagePort;
class PatchageModule;

using std::string;


/** Handles all externally driven functionality, registering ports etc.
 *
 * Jack callbacks and connect methods and things like that live here.
 * Right now just for jack ports, but that will change...
 */
class JackDriver : public Driver
{
public:
	JackDriver(Patchage* app);
	~JackDriver();
	
	void attach(bool launch_daemon);
	void detach();

	bool is_attached() const { return (m_client != NULL); }
	void refresh();

	bool connect(boost::shared_ptr<PatchagePort> src,
	             boost::shared_ptr<PatchagePort> dst);

	bool disconnect(boost::shared_ptr<PatchagePort> src,
	                boost::shared_ptr<PatchagePort> dst);
	
private:
	Patchage*             m_app;

	jack_client_t* m_client;

	Mutex m_mutex;

	list<string> m_added_ports;
	list<string> m_removed_ports;

	boost::shared_ptr<PatchagePort> create_port(boost::shared_ptr<PatchageModule> parent,
		jack_port_t* port);

	void destroy_all_ports();

	static void jack_port_registration_cb(jack_port_id_t port_id, int registered, void* controller);
	static int  jack_graph_order_cb(void* controller);
	static void jack_shutdown_cb(void* controller);
};


#endif // JACKDRIVER_H
