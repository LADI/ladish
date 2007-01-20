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

#ifndef RAUL_JACKDRIVER_H
#define RAUL_JACKDRIVER_H

#include <iostream>
#include <string>
#include <jack/jack.h>
#include <jack/statistics.h>

using std::string;


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
	
	void attach(const string& client_name);
	void detach();

	bool is_attached() const { return (m_client != NULL); }
	bool is_realtime() const { return m_client && jack_is_realtime(m_client); }
	
	void start_transport() { jack_transport_start(m_client); }
	void stop_transport()  { jack_transport_stop(m_client); }
	
	void rewind_transport() {
		jack_position_t zero;
		zero.frame = 0;
		zero.valid = (jack_position_bits_t)0;
		jack_transport_reposition(m_client, &zero);
	}
	
	jack_nframes_t buffer_size();
	bool           set_buffer_size(jack_nframes_t size);

	inline float sample_rate() { return jack_get_sample_rate(m_client); }

	inline size_t xruns() { return m_xruns; }
	void reset_xruns();

	inline float max_delay() { return jack_get_max_delayed_usecs(m_client); }
	inline void reset_delay() { jack_reset_max_delayed_usecs(m_client); }

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

private:

	static void error_cb(const char* msg);

	void destroy_all_ports();

	void update_time();

	static void jack_port_registration_cb(jack_port_id_t port_id, int registered, void* me);
	static int  jack_graph_order_cb(void* me);
	static int  jack_buffer_size_cb(jack_nframes_t buffer_size, void* me);
	static int  jack_xrun_cb(void* me);
	static void jack_shutdown_cb(void* me);

	jack_client_t* m_client;

	bool            m_is_activated;
	jack_position_t m_last_pos;
	jack_nframes_t  m_buffer_size;
	size_t          m_xruns;
	float           m_xrun_delay;
};


#endif // RAUL_JACKDRIVER_H
