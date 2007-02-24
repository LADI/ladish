/* This file is part of Raul.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Raul is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Raul is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef RAUL_JACKDRIVER_H
#define RAUL_JACKDRIVER_H

#include <iostream>
#include <string>
#include <jack/jack.h>
#include <jack/statistics.h>

using std::string;

namespace Raul {


/** Handles all externally driven functionality, registering ports etc.
 *
 * Jack callbacks and connect methods and things like that live here.
 * Right now just for jack ports, but that will change...
 */
class JackDriver
{
public:
	JackDriver();
	virtual ~JackDriver();
	
	void attach(const string& client_name, string server_name="");
	void detach();

	void activate();
	void deactivate();

	bool is_attached() const { return (_client != NULL); }
	bool is_realtime() const { return _client && jack_is_realtime(_client); }
	
	void start_transport() { jack_transport_start(_client); }
	void stop_transport()  { jack_transport_stop(_client); }
	
	void rewind_transport() {
		jack_position_t zero;
		zero.frame = 0;
		zero.valid = (jack_position_bits_t)0;
		jack_transport_reposition(_client, &zero);
	}
	
	jack_nframes_t buffer_size();
	bool           set_buffer_size(jack_nframes_t size);

	inline float sample_rate() { return jack_get_sample_rate(_client); }

	inline size_t xruns() { return _xruns; }
	void reset_xruns();

	inline float max_delay() { return jack_get_max_delayed_usecs(_client); }
	inline void reset_delay() { jack_reset_max_delayed_usecs(_client); }

	jack_client_t* jack_client() { return _client; }


protected:
	/** Process callback.  Derived classes should do all audio processing here. */
	virtual void on_process(jack_nframes_t /*nframes*/) {}

	/** Graph order change callback. */
	virtual void on_graph_order_changed() {}

	/** Buffer size changed callback.
	 * At the time this is called, buffer_size() will still return the old
	 * size.  Immediately afterwards, it will be set to the new value.
	 */
	virtual void on_buffer_size_changed(jack_nframes_t /*size*/) {}

	virtual void on_xrun()     {}
	virtual void on_shutdown() {}
	virtual void on_error()    {}

protected:
	jack_client_t* _client;

private:

	static void error_cb(const char* msg);

	void destroy_all_ports();

	void update_time();

	static void jack_port_registration_cb(jack_port_id_t port_id, int registered, void* me);
	static int  jack_graph_order_cb(void* me);
	static int  jack_xrun_cb(void* me);
	static int  jack_buffer_size_cb(jack_nframes_t buffer_size, void* me);
	static int  jack_process_cb(jack_nframes_t nframes, void* me);

	static void jack_shutdown_cb(void* me);

	bool            _is_activated;
	jack_position_t _last_pos;
	jack_nframes_t  _buffer_size;
	size_t          _xruns;
	float           _xrun_delay;
};


} // namespace Raul

#endif // RAUL_JACKDRIVER_H
