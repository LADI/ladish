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
#include <jack/statistics.h>
#include <jack/thread.h>
#include "raul/JackDriver.h"

using std::cerr; using std::endl;
using std::string;


JackDriver::JackDriver()
//: m_app(app)
: m_client(NULL)
, m_is_activated(false)
, m_xruns(0)
, m_xrun_delay(0)
{
	m_last_pos.frame = 0;
	m_last_pos.valid = (jack_position_bits_t)0;
}


JackDriver::~JackDriver() 
{
	detach();
}


/** Connect to Jack.
 */
void
JackDriver::attach(const string& client_name) 
{
	// Already connected
	if (m_client)
		return;
	
	jack_set_error_function(error_cb);

	m_client = jack_client_open(client_name.c_str(), JackNullOption, NULL);
	if (m_client == NULL) {
		//m_app->status_message("[JACK] Unable to create client");
		m_is_activated = false;
	} else {
		jack_set_error_function(error_cb);
		jack_on_shutdown(m_client, jack_shutdown_cb, this);
		jack_set_port_registration_callback(m_client, jack_port_registration_cb, this);
		jack_set_graph_order_callback(m_client, jack_graph_order_cb, this);
		jack_set_buffer_size_callback(m_client, jack_buffer_size_cb, this);
		jack_set_xrun_callback(m_client, jack_xrun_cb, this);
	
		//m_is_dirty = true;
		m_buffer_size = jack_get_buffer_size(m_client);

		if (!jack_activate(m_client)) {
			m_is_activated = true;
			//signal_attached.emit();
			//m_app->status_message("[JACK] Attached");
		} else {
			//m_app->status_message("[JACK] ERROR: Failed to attach");
			m_is_activated = false;
		}
	}
}


void
JackDriver::detach() 
{
	if (m_client) {
		jack_deactivate(m_client);
		jack_client_close(m_client);
		m_client = NULL;
		m_is_activated = false;
		//signal_detached.emit();
	}
}


void
JackDriver::jack_port_registration_cb(jack_port_id_t /*port_id*/, int /*registered*/, void* /*jack_driver*/) 
{
	/* whatever. */
}


int
JackDriver::jack_graph_order_cb(void* jack_driver) 
{
	JackDriver* me = reinterpret_cast<JackDriver*>(jack_driver);
	
	assert(me);
	me->on_graph_order_changed();

	return 0;
}


int
JackDriver::jack_buffer_size_cb(jack_nframes_t buffer_size, void* jack_driver) 
{
	JackDriver* me = reinterpret_cast<JackDriver*>(jack_driver);
	
	assert(me);
	me->reset_xruns();
	me->reset_delay();
	me->on_buffer_size_changed(buffer_size);
	me->m_buffer_size = buffer_size;

	return 0;
}


int
JackDriver::jack_xrun_cb(void* jack_driver)
{
	JackDriver* me = reinterpret_cast<JackDriver*>(jack_driver);
	
	assert(me);

	me->m_xruns++;
	me->m_xrun_delay = jack_get_xrun_delayed_usecs(me->m_client);
	me->reset_delay();

	me->on_xrun();
	
	return 0;
}


void
JackDriver::jack_shutdown_cb(void* jack_driver)
{
	JackDriver* me = reinterpret_cast<JackDriver*>(jack_driver);
	assert(me);

	me->reset_delay();
	me->reset_xruns();

	me->on_shutdown();

	me->m_client = NULL;
}


void
JackDriver::error_cb(const char* msg)
{
	cerr << "JACK ERROR: " << msg << endl;
}


jack_nframes_t
JackDriver::buffer_size()
{
	if (m_is_activated)
		return m_buffer_size;
	else
		return jack_get_buffer_size(m_client);
}


void
JackDriver::reset_xruns()
{
	m_xruns = 0;
	m_xrun_delay = 0;
}


bool
JackDriver::set_buffer_size(jack_nframes_t size)
{
	if (buffer_size() == size) {
		return true;
	}

	if (!m_client) {
		m_buffer_size = size;
		return true;
	}
	
	if (jack_set_buffer_size(m_client, size)) {
		//m_app->status_message("[JACK] ERROR: Unable to set buffer size");
		return false;
	} else {
		return true;
	}
}

