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

namespace Raul {


JackDriver::JackDriver()
: _client(NULL)
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
JackDriver::attach(const string& client_name) 
{
	// Already connected
	if (_client)
		return;
	
	jack_set_error_function(error_cb);

	_client = jack_client_open(client_name.c_str(), JackNullOption, NULL);
	if (_client == NULL) {
		//_app->status_message("[JACK] Unable to create client");
		_is_activated = false;
	} else {
		jack_set_error_function(error_cb);
		jack_on_shutdown(_client, jack_shutdown_cb, this);
		jack_set_port_registration_callback(_client, jack_port_registration_cb, this);
		jack_set_graph_order_callback(_client, jack_graph_order_cb, this);
		jack_set_buffer_size_callback(_client, jack_buffer_size_cb, this);
		jack_set_xrun_callback(_client, jack_xrun_cb, this);
	
		//_is_dirty = true;
		_buffer_size = jack_get_buffer_size(_client);

		if (!jack_activate(_client)) {
			_is_activated = true;
			//signal_attached.emit();
			//_app->status_message("[JACK] Attached");
		} else {
			//_app->status_message("[JACK] ERROR: Failed to attach");
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
		_client = NULL;
		_is_activated = false;
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
	me->_buffer_size = buffer_size;

	return 0;
}


int
JackDriver::jack_xrun_cb(void* jack_driver)
{
	JackDriver* me = reinterpret_cast<JackDriver*>(jack_driver);
	
	assert(me);

	me->_xruns++;
	me->_xrun_delay = jack_get_xrun_delayed_usecs(me->_client);
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

	me->_client = NULL;
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
		//_app->status_message("[JACK] ERROR: Unable to set buffer size");
		return false;
	} else {
		return true;
	}
}


} // namespace Raul

