/* This file is part of Patchage.  Copyright (C) 2005 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
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
#include <jack/jack.h>
#include <string>
#include "Driver.h"
class Patchage;
class PatchageFlowCanvas;
class PatchagePort;

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

	bool connect(const PatchagePort* const src_port, const PatchagePort* const dst_port);
	bool disconnect(const PatchagePort* const src_port, const PatchagePort* const dst_port);
	/*bool connect(const string& src_module_name, const string& src_port_name,
                 const string& dst_module_name,  const string& dest_port_name);
	
	bool disconnect(const string& src_module_name, const string& src_port_name,
                    const string& dst_module_name,  const string& dest_port_name);*/
	
private:
	Patchage*             m_app;
	PatchageFlowCanvas*   m_canvas;

	jack_client_t* m_client;

	static void jack_port_registration_cb(jack_port_id_t port_id, int registered, void* controller);
	static int  jack_graph_order_cb(void* controller);
	static void jack_shutdown_cb(void* controller);
};


#endif // JACKDRIVER_H
