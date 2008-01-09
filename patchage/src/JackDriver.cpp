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

#include <cassert>
#include <cstring>
#include <string>
#include <set>
#include <iostream>
#include CONFIG_H_PATH
#include <jack/jack.h>
#include <jack/statistics.h>
#include <jack/thread.h>
#include <raul/SharedPtr.hpp>
#include "PatchageCanvas.hpp"
#include "PatchageEvent.hpp"
#include "JackDriver.hpp"
#include "Patchage.hpp"
#include "PatchageModule.hpp"

using namespace std;
using namespace FlowCanvas;


JackDriver::JackDriver(Patchage* app)
	: Driver(128)
	, _app(app)
	, _client(NULL)
	, _is_activated(false)
	, _xruns(0)
	, _xrun_delay(0)
{
	_last_pos.frame = 0;
	_last_pos.valid = (jack_position_bits_t)0;
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
	if (_client)
		return;
	
	jack_set_error_function(error_cb);

	jack_options_t options = (!launch_daemon) ? JackNoStartServer : JackNullOption;
	_client = jack_client_open("Patchage", options, NULL);
	if (_client == NULL) {
		_app->status_message("[JACK] Unable to create client");
		_is_activated = false;
	} else {
		jack_client_t* const client = _client;

		jack_set_error_function(error_cb);
		jack_on_shutdown(client, jack_shutdown_cb, this);
		jack_set_port_registration_callback(client, jack_port_registration_cb, this);
		jack_set_port_connect_callback(client, jack_port_connect_cb, this);
		jack_set_graph_order_callback(client, jack_graph_order_cb, this);
		jack_set_buffer_size_callback(client, jack_buffer_size_cb, this);
		jack_set_xrun_callback(client, jack_xrun_cb, this);
	
		_buffer_size = jack_get_buffer_size(client);

		if (!jack_activate(client)) {
			_is_activated = true;
			signal_attached.emit();
			_app->status_message("[JACK] Attached");
		} else {
			_app->status_message("[JACK] ERROR: Failed to attach");
			_is_activated = false;
		}
	}
}


void
JackDriver::detach() 
{
	if (_client) {
		jack_deactivate(_client);
		jack_client_close(_client);
		_mutex.lock();
		_client = NULL;
		_mutex.unlock();
		destroy_all_ports();
		_is_activated = false;
		signal_detached.emit();
		_app->status_message("[JACK] Detached");
	}
}


/** Destroy all JACK (canvas) ports.
 */
void
JackDriver::destroy_all_ports()
{
	ItemList modules = _app->canvas()->items(); // copy
	for (ItemList::iterator m = modules.begin(); m != modules.end(); ++m) {
		SharedPtr<Module> module = PtrCast<Module>(*m);
		if (!module)
			continue;
		PortVector ports = module->ports(); // copy
		for (PortVector::iterator p = ports.begin(); p != ports.end(); ++p) {
			boost::shared_ptr<PatchagePort> port = boost::dynamic_pointer_cast<PatchagePort>(*p);
			if (port && port->type() == JACK_AUDIO || port->type() == JACK_MIDI) {
				module->remove_port(port);
				port->hide();
			}
		}

		if (module->ports().empty())
			_app->canvas()->remove_item(module);
		else
			module->resize();
	}
}


boost::shared_ptr<PatchagePort>
JackDriver::create_port(boost::shared_ptr<PatchageModule> parent, jack_port_t* port)
{
	const char* const type_str = jack_port_type(port);
	PortType port_type;

	if (!strcmp(type_str, JACK_DEFAULT_AUDIO_TYPE)) {
		port_type = JACK_AUDIO;
		//cerr << "TYPE: AUDIO\n";
#ifdef HAVE_JACK_MIDI
	} else if (!strcmp(type_str, JACK_DEFAULT_MIDI_TYPE)) {
		port_type = JACK_MIDI;
		//cerr << "TYPE: MIDI\n";
#endif
	} else {
		cerr << "WARNING: " << jack_port_name(port) << " has unknown type \'" << type_str << "\'" << endl;
		return boost::shared_ptr<PatchagePort>();
	}
	
	//cerr << "Port " << jack_port_name(port) << " type: " << type_str << " = " << (int)port_type << endl;
	
	boost::shared_ptr<PatchagePort> ret(
		new PatchagePort(parent, port_type, jack_port_short_name(port),
			(jack_port_flags(port) & JackPortIsInput),
			_app->state_manager()->get_port_color(port_type)));

	return ret;
}


void
JackDriver::shutdown()
{
	destroy_all_ports();
	signal_detached.emit();
}


/** Refresh all Jack audio ports/connections.
 * To be called from GTK thread only.
 */
void
JackDriver::refresh() 
{
	const char** ports;
	jack_port_t* port;

	// Jack can take _client away from us at any time throughout here :/
	// Shortest locks possible is the best solution I can figure out
	
	_mutex.lock();
	
	if (_client == NULL) {
		_mutex.unlock();
		shutdown();
		return;
	}

	ports = jack_get_ports(_client, NULL, NULL, 0); // get all existing ports
	
	_mutex.unlock();

	string client1_name;
	string port1_name;
	string client2_name;
	string port2_name;
	
	// Add all ports
	if (ports)
	for (int i=0; ports[i]; ++i) {
		_mutex.lock();
		if (!_client) {
			_mutex.unlock();
			shutdown();
			return;
		}
		port = jack_port_by_name(_client, ports[i]);
		_mutex.unlock();

		client1_name = ports[i];
		client1_name = client1_name.substr(0, client1_name.find(":"));

		ModuleType type = InputOutput;
		if (_app->state_manager()->get_module_split(client1_name,
					(jack_port_flags(port) & JackPortIsTerminal))) {
			if (jack_port_flags(port) & JackPortIsInput) {
				type = Input;
			} else {
				type = Output;
			}
		}

		boost::shared_ptr<PatchageModule> m = _app->canvas()->find_module(client1_name, type);

		if (!m) {
			m = boost::shared_ptr<PatchageModule>(new PatchageModule(_app, client1_name, type));
			m->load_location();
			_app->canvas()->add_item(m);
		}

		// FIXME: leak?  jack docs don't say
		const char* const type_str = jack_port_type(port);
		PortType port_type;

		if (!strcmp(type_str, JACK_DEFAULT_AUDIO_TYPE)) {
			port_type = JACK_AUDIO;
#ifdef HAVE_JACK_MIDI
		} else if (!strcmp(type_str, JACK_DEFAULT_MIDI_TYPE)) {
			port_type = JACK_MIDI;
#endif
		} else {
			cerr << "WARNING: " << ports[i] << " has unknown type \'" << type_str << "\'" << endl;
			continue;
		}

		if (!m->get_port(jack_port_short_name(port))) {
			m->add_port(create_port(m, port));
		}
	}
	
	// Add all connections
	if (ports) {
	
	_mutex.lock();
	
	if (!_client) {
		_mutex.unlock();
		shutdown();
		return;
	}

	for (int i=0; ports[i]; ++i) {

		port = jack_port_by_name(_client, ports[i]);
		const char** connected_ports = jack_port_get_all_connections(_client, port);
		
		if (connected_ports) {
			for (int j=0; connected_ports[j]; ++j) {
				client1_name = ports[i];
				port1_name = client1_name.substr(client1_name.find(':')+1);
				client1_name = client1_name.substr(0, client1_name.find(':'));

				client2_name = connected_ports[j];
				port2_name = client2_name.substr(client2_name.find(':')+1);
				client2_name = client2_name.substr(0, client2_name.find(':'));

				boost::shared_ptr<Port> port1
					= _app->canvas()->get_port(client1_name, port1_name);
				boost::shared_ptr<Port> port2
					= _app->canvas()->get_port(client2_name, port2_name);

				if (!port1 || !port2)
					continue;

				boost::shared_ptr<Port> src;
				boost::shared_ptr<Port> dst;

				if (port1->is_output() && port2->is_input()) {
					src = port1;
					dst = port2;
				} else {
					src = port2;
					dst = port1;
				}

				if (src && dst && !src->is_connected_to(dst))
					_app->canvas()->add_connection(src, dst, port1->color() + 0x22222200);
			}
			
			free(connected_ports);
		}
	}
		
	_mutex.unlock();
	
	}

	free(ports);
}


/** Connects two Jack audio ports.
 * To be called from GTK thread only.
 * \return Whether connection succeeded.
 */
bool
JackDriver::connect(boost::shared_ptr<PatchagePort> src_port, boost::shared_ptr<PatchagePort> dst_port)
{
	if (_client == NULL)
		return false;
	
	int result = jack_connect(_client, src_port->full_name().c_str(), dst_port->full_name().c_str());
	
	if (result == 0)
		_app->status_message(string("[JACK] Connected ")
			+ src_port->full_name() + " -> " + dst_port->full_name());
	else
		_app->status_message(string("[JACK] Unable to connect ")
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
	if (_client == NULL)
		return false;

	int result = jack_disconnect(_client, src_port->full_name().c_str(), dst_port->full_name().c_str());
	
	if (result == 0)
		_app->status_message(string("[JACK] Disconnected ")
			+ src_port->full_name() + " -> " + dst_port->full_name());
	else
		_app->status_message(string("[JACK] Unable to disconnect ")
			+ src_port->full_name() + " -> " + dst_port->full_name());
	
	return (!result);
}


void
JackDriver::update_time()
{
}


void
JackDriver::jack_port_registration_cb(jack_port_id_t port_id, int registered, void* jack_driver) 
{
	assert(jack_driver);
	JackDriver* me = reinterpret_cast<JackDriver*>(jack_driver);
	assert(me->_client);

	jack_port_t* const port = jack_port_by_id(me->_client, port_id);
	const string full_name = jack_port_name(port);
	
	jack_reset_max_delayed_usecs(me->_client);

	if (registered) {
		me->_events.push(PatchageEvent(PatchageEvent::PORT_CREATION, port_id));
	} else {
		me->_events.push(PatchageEvent(PatchageEvent::PORT_DESTRUCTION, port_id));
	}
}


void
JackDriver::jack_port_connect_cb(jack_port_id_t src, jack_port_id_t dst, int connect, void* jack_driver) 
{
	assert(jack_driver);
	JackDriver* me = reinterpret_cast<JackDriver*>(jack_driver);
	assert(me->_client);

	jack_reset_max_delayed_usecs(me->_client);

	if (connect) {
		me->_events.push(PatchageEvent(PatchageEvent::CONNECTION, src, dst));
	} else {
		me->_events.push(PatchageEvent(PatchageEvent::DISCONNECTION, src, dst));
	}
}


int
JackDriver::jack_graph_order_cb(void* jack_driver) 
{
	assert(jack_driver);
	JackDriver* me = reinterpret_cast<JackDriver*>(jack_driver);
	assert(me->_client);
	
	jack_reset_max_delayed_usecs(me->_client);
	
	return 0;
}


int
JackDriver::jack_buffer_size_cb(jack_nframes_t buffer_size, void* jack_driver) 
{
	assert(jack_driver);
	JackDriver* me = reinterpret_cast<JackDriver*>(jack_driver);
	assert(me->_client);
	
	jack_reset_max_delayed_usecs(me->_client);

	//(me->_mutex).lock();
	me->_buffer_size = buffer_size;
	me->reset_xruns();
	me->reset_delay();
	//(me->_mutex).unlock();

	return 0;
}


int
JackDriver::jack_xrun_cb(void* jack_driver)
{
	assert(jack_driver);
	JackDriver* me = reinterpret_cast<JackDriver*>(jack_driver);
	assert(me->_client);

	//(me->_mutex).lock();
	me->_xruns++;
	me->_xrun_delay = jack_get_xrun_delayed_usecs(me->_client);
	jack_reset_max_delayed_usecs(me->_client);

	//cerr << "** XRUN Delay = " << me->_xrun_delay << endl;

	//(me->_mutex).unlock();
	
	return 0;
}


void
JackDriver::jack_shutdown_cb(void* jack_driver)
{
	assert(jack_driver);
	JackDriver* me = reinterpret_cast<JackDriver*>(jack_driver);
	assert(me->_client);

	jack_reset_max_delayed_usecs(me->_client);

	me->_mutex.lock();
	me->_client = NULL;
	me->_mutex.unlock();
}


void
JackDriver::error_cb(const char* msg)
{
	cerr << "JACK ERROR: " << msg << endl;
}


jack_nframes_t
JackDriver::buffer_size()
{
	if (_is_activated)
		return _buffer_size;
	else
		return jack_get_buffer_size(_client);
}


void
JackDriver::reset_xruns()
{
	_xruns = 0;
	_xrun_delay = 0;
}


bool
JackDriver::set_buffer_size(jack_nframes_t size)
{
	if (buffer_size() == size) {
		return true;
	}

	if (!_client) {
		_buffer_size = size;
		return true;
	}
	
	if (jack_set_buffer_size(_client, size)) {
		_app->status_message("[JACK] ERROR: Unable to set buffer size");
		return false;
	} else {
		return true;
	}
}

