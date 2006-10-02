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

#include <cassert>
#include <cstring>
#include <string>
#include <iostream>
#include <jack/jack.h>
#include "PatchageFlowCanvas.h"
#include "JackDriver.h"
#include "Patchage.h"
#include "PatchageModule.h"

using std::cerr; using std::endl;
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
	// Already connected
	if (m_client)
		return;
	
	jack_options_t options = (!launch_daemon) ? JackNoStartServer : JackNullOption;
	m_client = jack_client_open("Patchage", options, NULL);
	if (m_client == NULL) {
		m_app->status_message("[JACK] Unable to attach");
	} else {
		jack_on_shutdown(m_client, jack_shutdown_cb, this);
		jack_set_port_registration_callback(m_client, jack_port_registration_cb, this);
		jack_set_graph_order_callback(m_client, jack_graph_order_cb, this);
	
		m_is_dirty = true;
		
		jack_activate(m_client);
	
		signal_attached.emit();
		m_app->status_message("[JACK] Attached");
	}
}


void
JackDriver::detach() 
{
	if (m_client != NULL) {
		jack_deactivate(m_client);
		jack_client_close(m_client);
		m_client = NULL;
		signal_detached.emit();
		destroy_all_ports();
		m_app->status_message("[JACK] Detached");
	}
}


/** Destroy all JACK (canvas) ports.
 */
void
JackDriver::destroy_all_ports()
{
	ModuleMap modules = m_canvas->modules(); // copy
	for (ModuleMap::iterator m = modules.begin(); m != modules.end(); ++m) {
		PortVector ports = m->second->ports(); // copy
		for (PortVector::iterator p = ports.begin(); p != ports.end(); ++p) {
			boost::shared_ptr<PatchagePort> port = boost::dynamic_pointer_cast<PatchagePort>(*p);
			if (port && port->type() == JACK_AUDIO || port->type() == JACK_MIDI) {
				port.reset();
			}
		}

		if (m->second->ports().empty())
			m->second.reset();
	}
}


boost::shared_ptr<PatchagePort>
JackDriver::create_port(boost::shared_ptr<PatchageModule> parent, jack_port_t* port)
{
	const char* const type_str = jack_port_type(port);
	PortType port_type;
	if (!strcmp(type_str, JACK_DEFAULT_MIDI_TYPE)) {
		port_type = JACK_MIDI;
	} else if (!strcmp(type_str, JACK_DEFAULT_AUDIO_TYPE)) {
		port_type = JACK_AUDIO;
	} else {
		cerr << "WARNING: " << jack_port_name(port) << " has unknown type \'" << type_str << "\'" << endl;
		return boost::shared_ptr<PatchagePort>();
	}
	
	boost::shared_ptr<PatchagePort> ret(
		new PatchagePort(parent, port_type, jack_port_short_name(port),
			(jack_port_flags(port) & JackPortIsInput),
			m_app->state_manager()->get_port_color(port_type)));

	return ret;
}


/** Refresh all Jack audio ports/connections.
 * To be called from GTK thread only.
 */
void
JackDriver::refresh() 
{
	m_mutex.lock();

	if (m_client == NULL) {
		// Shutdown
		if (m_is_dirty) {
			signal_detached.emit();
			destroy_all_ports();
		}
		m_is_dirty = false;
		m_mutex.unlock();
		return;
	}

	const char** ports;
	jack_port_t* port;

	ports = jack_get_ports(m_client, NULL, NULL, 0); // get all existing ports

	string client1_name;
	string port1_name;
	string client2_name;
	string port2_name;

	// Add all ports
	if (ports)
	for (int i=0; ports[i]; ++i) {
		port = jack_port_by_name(m_client, ports[i]);
		client1_name = ports[i];
		client1_name = client1_name.substr(0, client1_name.find(":"));

		ModuleType type = InputOutput;
		//if (m_app->state_manager()->get_module_split(client1_name)
		//		|| jack_port_flags(port) & JackPortIsTerminal)
		if (m_app->state_manager()->get_module_split(client1_name,
					(jack_port_flags(port) & JackPortIsTerminal))) {
			if (jack_port_flags(port) & JackPortIsInput) {
				type = Input;
			} else {
				type = Output;
			}
		}

		boost::shared_ptr<PatchageModule> m = m_canvas->find_module(client1_name, type);

		if (!m) {
			m = boost::shared_ptr<PatchageModule>(new PatchageModule(m_app, client1_name, type));
			m->load_location();
			m->store_location();
			m_app->canvas()->add_module(m);
		}

		// FIXME: leak?  jack docs don't say
		const char* const type_str = jack_port_type(port);
		PortType port_type;
		if (!strcmp(type_str, JACK_DEFAULT_MIDI_TYPE)) {
			port_type = JACK_MIDI;
		} else if (!strcmp(type_str, JACK_DEFAULT_AUDIO_TYPE)) {
			port_type = JACK_AUDIO;
		} else {
			cerr << "WARNING: " << ports[i] << " has unknown type \'" << type_str << "\'" << endl;
			continue;
		}

		if (!m->get_port(jack_port_short_name(port))) {
			m->add_port(create_port(m, port));
		}

		m->resize();
	}
	
	
	// Remove any since-removed ports
	for (list<string>::iterator i = m_removed_ports.begin(); i != m_removed_ports.end(); ++i) {
		const string module_name = (*i).substr(0, i->find(":"));
		const string port_name = (*i).substr(i->find(":")+1);
		
		for (ModuleMap::iterator m = m_canvas->modules().begin(); m != m_canvas->modules().end(); ++m) {
			if (m->second->name() == module_name)
				m->second->remove_port(port_name);
		}
	}


	// Add all connections
	if (ports)
	for (int i=0; ports[i]; ++i) {
		port = jack_port_by_name(m_client, ports[i]);
		const char** connected_ports = jack_port_get_all_connections(m_client, port);

		if (connected_ports) {
			for (int j=0; connected_ports[j]; ++j) {
				client1_name = ports[i];
				port1_name = client1_name.substr(client1_name.find(':')+1);
				client1_name = client1_name.substr(0, client1_name.find(':'));

				client2_name = connected_ports[j];
				port2_name = client2_name.substr(client2_name.find(':')+1);
				client2_name = client2_name.substr(0, client2_name.find(':'));

				boost::shared_ptr<Port> port1 = m_canvas->get_port(client1_name, port1_name);
				boost::shared_ptr<Port> port2 = m_canvas->get_port(client2_name, port2_name);

				if (port1 && port2) {
					boost::shared_ptr<Connection> existing = m_canvas->get_connection(port1, port2);
					if (existing) {
						existing->set_flagged(false);
					} else {
						m_canvas->add_connection(port1, port2);
					}
				}
			}
			
			free(connected_ports);
		}
	}

	free(ports);
	undirty();

	m_mutex.unlock();
}


/** Connects two Jack audio ports.
 * To be called from GTK thread only.
 * \return Whether connection succeeded.
 */
bool
JackDriver::connect(boost::shared_ptr<PatchagePort> src_port, boost::shared_ptr<PatchagePort> dst_port)
{
	if (m_client == NULL)
		return false;
	
	int result = jack_connect(m_client, src_port->full_name().c_str(), dst_port->full_name().c_str());
	
	if (result == 0)
		m_app->status_message(string("[JACK] Connected ")
			+ src_port->full_name() + " -> " + dst_port->full_name());
	else
		m_app->status_message(string("[JACK] Unable to connect ")
			+ src_port->full_name() + " -> " + dst_port->full_name());
	
	return (!result);
}


/** Disconnects two Jack audio ports.
 * To be called from GTK thread only.
 * \return Whether disconnection succeeded.
 */
bool
JackDriver::disconnect(boost::shared_ptr<PatchagePort> const src_port, boost::shared_ptr<PatchagePort> const dst_port)
{
	if (m_client == NULL)
		return false;

	int result = jack_disconnect(m_client, src_port->full_name().c_str(), dst_port->full_name().c_str());
	
	if (result == 0)
		m_app->status_message(string("[JACK] Disconnected ")
			+ src_port->full_name() + " -> " + dst_port->full_name());
	else
		m_app->status_message(string("[JACK] Unable to disconnect ")
			+ src_port->full_name() + " -> " + dst_port->full_name());
	
	return (!result);
}


void
JackDriver::jack_port_registration_cb(jack_port_id_t port_id, int /*registered*/, void* jack_driver) 
{
	assert(jack_driver);
	JackDriver* me = reinterpret_cast<JackDriver*>(jack_driver);
	assert(me->m_client);

	jack_port_t* const port = jack_port_by_id(me->m_client, port_id);
	const string full_name = jack_port_name(port);

	//(me->m_mutex).lock();

	/*if (registered) {
		me->m_added_ports.push_back(full_name);
	} else {
		me->m_removed_ports.push_back(full_name);
	}*/

	me->m_is_dirty = true;
	
	//(me->m_mutex).unlock();
}


int
JackDriver::jack_graph_order_cb(void* jack_driver) 
{
	assert(jack_driver);
	JackDriver* me = reinterpret_cast<JackDriver*>(jack_driver);
	assert(me->m_client);
	
	//(me->m_mutex).lock();
	me->m_is_dirty = true;
	//(me->m_mutex).unlock();

	return 0;
}


void
JackDriver::jack_shutdown_cb(void* jack_driver)
{
	assert(jack_driver);
	JackDriver* me = reinterpret_cast<JackDriver*>(jack_driver);
	assert(me->m_client);

	//(me->m_mutex).lock();
	me->m_client = NULL;
	me->m_is_dirty = true;
	//(me->m_mutex).unlock();
}


