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

#include <cassert>
#include <cstring>
#include <string>
#include <iostream>
#include <jack/jack.h>
#include "PatchageFlowCanvas.h"
#include "JackDriver.h"
#include "Patchage.h"
#include "PatchageModule.h"

using std::cerr; using std::endl; using std::cout;
using std::string;

using namespace LibFlowCanvas;


JackDriver::JackDriver(Patchage* app)
: m_app(app),
  m_canvas(app->canvas()),
  m_client(NULL)
{
}


JackDriver::~JackDriver() 
{
	detach();
}


/** Connect to Jack.
 */
void
JackDriver::attach(bool launch_daemon) 
{
	cout << "Connecting to Jack... ";
	cout.flush();

	if (m_client != NULL) {
		cout << "already connected.  " << endl;
		return;
	}
	
	jack_options_t options = (!launch_daemon) ? JackNoStartServer : JackNullOption;
	m_client = jack_client_open("Patchage", options, NULL);
	if (m_client == NULL) {
		cout << "Failed" << endl;
	} else {
		jack_set_port_registration_callback(m_client, jack_port_registration_cb, this);
		jack_set_graph_order_callback(m_client, jack_graph_order_cb, this);
		jack_on_shutdown(m_client, jack_shutdown_cb, this);
	
		jack_activate(m_client);
		
		cout << "Connected" << endl;

		m_canvas->destroy();

		m_is_dirty = true;
	}
}


void
JackDriver::detach() 
{
	if (m_client != NULL) {
		jack_deactivate(m_client);
		jack_client_close(m_client);
		m_client = NULL;
		cout << "Disconnected from Jack" << endl;
	}
}


/** Refresh all Jack audio ports/connections.
 * To be called from GTK thread only.
 */
void
JackDriver::refresh() 
{
	if (m_client == NULL)
		return;
	
	const char** ports;
	jack_port_t* port;
	
	ports = jack_get_ports(m_client, NULL, NULL, 0); // get all existing ports

	string client1_name;
	string port1_name;
	string client2_name;
	string port2_name;
	PatchageModule* m = NULL;
	
	// Add all ports
	if (ports != NULL)
	for (int i=0; ports[i]; ++i) {
		port = jack_port_by_name(m_client, ports[i]);
		client1_name = ports[i];
		client1_name = client1_name.substr(0, client1_name.find(":"));

		ModuleType type = InputOutput;
		//if (m_app->state_manager()->get_module_split(client1_name)
		//		|| jack_port_flags(port) & JackPortIsTerminal) {
		if (m_app->state_manager()->get_module_split(client1_name,
				(jack_port_flags(port) & JackPortIsTerminal))) {
			if (jack_port_flags(port) & JackPortIsInput) {
				type = Input;
			} else {
				type = Output;
			}
		}

		m = (PatchageModule*)m_canvas->find_module(client1_name, type);

		if (m == NULL) {
			m = new PatchageModule(m_app, client1_name, type);
			m->load_location();
			m->store_location();
			m_canvas->add_module(m);
		}
		
		// FIXME: leak?  jack docs don't say
		const char* const type_str = jack_port_type(port);
		PortType port_type = JACK_AUDIO;
		if (!strcmp(type_str, JACK_DEFAULT_MIDI_TYPE))
			port_type = JACK_MIDI;
		else if (strcmp(type_str, JACK_DEFAULT_AUDIO_TYPE))
			throw "Unknown JACK port type?";

		m->add_patchage_port(jack_port_short_name(port),
			(jack_port_flags(port) & JackPortIsInput),
			port_type);
	}
	
	// Add all connections
	if (ports != NULL) {
		for (int i=0; ports[i]; ++i) {
			port = jack_port_by_name(m_client, ports[i]);
			const char** connected_ports = jack_port_get_all_connections(m_client, port);
			
			if (connected_ports != NULL)
			for (int j=0; connected_ports[j]; ++j) {
				client1_name = ports[i];
				port1_name = client1_name.substr(client1_name.find(':')+1);
				client1_name = client1_name.substr(0, client1_name.find(':'));
				
				client2_name = connected_ports[j];
				port2_name = client2_name.substr(client2_name.find(':')+1);
				client2_name = client2_name.substr(0, client2_name.find(':'));
				
				m_canvas->add_connection(client1_name, port1_name, client2_name, port2_name);
			}
			free(connected_ports);
		}		
		free(ports);
	}

	undirty();
}


/** Connects two Jack audio ports.
 * To be called from GTK thread only.
 * \return Whether connection succeeded.
 */
bool
JackDriver::connect(const PatchagePort* const src_port, const PatchagePort* const dst_port)
{
	if (m_client == NULL)
		return false;
	
	int result = jack_connect(m_client, src_port->full_name().c_str(), dst_port->full_name().c_str());
	
	string msg;
	
	if (result == 0) {
		msg = "Successfully connected jack ports";
	} else {
		msg = "Unable to connect ";
		msg += src_port->full_name() + " -> " + dst_port->full_name();
	}
	m_app->status_message(msg);
	
	return (!result);
}


/** Disconnects two Jack audio ports.
 * To be called from GTK thread only.
 * \return Whether disconnection succeeded.
 */
bool
JackDriver::disconnect(const PatchagePort* const src_port, const PatchagePort* const dst_port)
{
	if (m_client == NULL)
		return false;

	int result = jack_disconnect(m_client, src_port->full_name().c_str(), dst_port->full_name().c_str());
	
	string msg;
	
	if (result == 0) {
		msg = "Successfully disconnected jack ports";
	} else {
		msg = "Unable to disconnect ";
		msg += src_port->full_name() + " -> " + dst_port->full_name();
	}
	m_app->status_message(msg);
	
	return (!result);
}


void
JackDriver::jack_port_registration_cb(jack_port_id_t port_id, int registered, void* me) 
{
	assert(me != NULL);
	assert(((JackDriver*)me)->m_client != NULL);
	((JackDriver*)me)->m_is_dirty = true;
}


int
JackDriver::jack_graph_order_cb(void* me) 
{
	assert(me != NULL);
	assert(((JackDriver*)me)->m_client != NULL);
	((JackDriver*)me)->m_is_dirty = true;

	return 0;
}


void
JackDriver::jack_shutdown_cb(void* me)
{
	assert(me != NULL);
	((JackDriver*)me)->m_client = NULL;
	((JackDriver*)me)->m_is_dirty = true;
}


