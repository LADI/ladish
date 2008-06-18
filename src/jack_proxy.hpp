/* This file is part of Patchage.
 * Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
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

#ifndef PATCHAGE_JACKDBUSDRIVER_HPP
#define PATCHAGE_JACKDBUSDRIVER_HPP

#include <iostream>
#include <boost/shared_ptr.hpp>
#include <jack/jack.h>
#include <jack/statistics.h>
#include <glibmm/thread.h>
#include <raul/AtomicPtr.hpp>
#include <dbus/dbus.h>

#include "Patchage.hpp"
#include "PatchageModule.hpp"

class PatchageEvent;
class PatchageFlowCanvas;
class PatchagePort;

class jack_proxy
{
public:
	jack_proxy(Patchage* app);
	~jack_proxy();

	bool is_started();
	bool is_realtime();
	
	void refresh();

	bool
	connect(
		boost::shared_ptr<PatchagePort> src,
		boost::shared_ptr<PatchagePort> dst);

	bool
	disconnect(
		boost::shared_ptr<PatchagePort> src,
		boost::shared_ptr<PatchagePort> dst);
	void start_transport();
	void stop_transport();
	void rewind_transport();

	void reset_xruns();

	jack_nframes_t buffer_size();
	bool set_buffer_size(jack_nframes_t size);

	float sample_rate();

	size_t xruns();

	float get_dsp_load();

	float get_max_dsp_load();
	void reset_max_dsp_load();

	void
	start_server();

	void
	stop_server();

	sigc::signal<void> signal_started;
	sigc::signal<void> signal_stopped;

private:
	void error_msg(const std::string& msg) const;
	void info_msg(const std::string& msg) const;

	boost::shared_ptr<PatchageModule>
	find_or_create_module(
		ModuleType type,
		const std::string& name);

	void
	add_port(
		boost::shared_ptr<PatchageModule>& module,
		PortType type,
		const std::string& name,
		bool is_input);

	void
	add_port(
		dbus_uint64_t client_id,
		const char *client_name,
		dbus_uint64_t port_id,
		const char *port_name,
		dbus_uint32_t port_flags,
		dbus_uint32_t port_type);

	void
	remove_port(
		dbus_uint64_t client_id,
		const char *client_name,
		dbus_uint64_t port_id,
		const char *port_name);

	void
	connect_ports(
		dbus_uint64_t connection_id,
		dbus_uint64_t client1_id,
		const char *client1_name,
		dbus_uint64_t port1_id,
		const char *port1_name,
		dbus_uint64_t client2_id,
		const char *client2_name,
		dbus_uint64_t port2_id,
		const char *port2_name);

	void
	disconnect_ports(
		dbus_uint64_t connection_id,
		dbus_uint64_t client1_id,
		const char *client1_name,
		dbus_uint64_t port1_id,
		const char *port1_name,
		dbus_uint64_t client2_id,
		const char *client2_name,
		dbus_uint64_t port2_id,
		const char *port2_name);

	bool
	call(
		bool response_expected,
		const char* iface,
		const char* method,
		DBusMessage ** reply_ptr_ptr,
		int in_type,
		...);

	void
	update_attached();

	void
	destroy_all_ports();

	void refresh_internal(bool force);

	static
	DBusHandlerResult
	dbus_message_hook(
		DBusConnection *connection,
		DBusMessage *message,
		void *me);

	void
	on_jack_appeared();

	void
	on_jack_disappeared();

private:
	Patchage* _app;

	bool _server_responding;
	bool _server_started;

	dbus_uint64_t _graph_version;

	float _max_dsp_load;
};

#endif // PATCHAGE_JACKDBUSDRIVER_HPP
