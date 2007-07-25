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

#include <string>
#include <set>
#include <iostream>
#include <cassert>
#include <raul/SharedPtr.hpp>
#include "PatchageCanvas.hpp"
#include "AlsaDriver.hpp"
#include "Patchage.hpp"
#include "PatchageModule.hpp"
#include "PatchagePort.hpp"

using namespace std;
using namespace FlowCanvas;

AlsaDriver::AlsaDriver(Patchage* app)
	: Driver(128)
	, _app(app)
	, _seq(NULL)
{
}


AlsaDriver::~AlsaDriver() 
{
	detach();
}


/** Attach to ALSA.
 * @a launch_daemon is ignored, as ALSA has no daemon to launch/connect to.
 */
void
AlsaDriver::attach(bool /*launch_daemon*/)
{
	int ret = snd_seq_open(&_seq, "default",
	                       SND_SEQ_OPEN_DUPLEX,
	                       SND_SEQ_NONBLOCK);
	if (ret) {
		_app->status_message("[ALSA] Unable to attach");
		_seq = NULL;
	} else {
		_app->status_message("[ALSA] Attached");

		snd_seq_set_client_name(_seq, "Patchage");
	
		ret = pthread_create(&_refresh_thread, NULL, &AlsaDriver::refresh_main, this);
		if (ret)
			cerr << "Couldn't start refresh thread" << endl;
		
		signal_attached.emit();
	}
}


void
AlsaDriver::detach() 
{
	if (_seq) {
		pthread_cancel(_refresh_thread);
		pthread_join(_refresh_thread, NULL);
		snd_seq_close(_seq);
		_seq = NULL;
		signal_detached.emit();
		_app->status_message("[ALSA] Detached");
	}
}


/** Refresh all Alsa Midi ports and connections.
 */
void
AlsaDriver::refresh()
{
	if (!is_attached())
		return;

	assert(_seq);
	
	refresh_ports();
	refresh_connections();
}


boost::shared_ptr<PatchagePort>
AlsaDriver::create_port(boost::shared_ptr<PatchageModule> parent,
		const string& name, bool is_input, snd_seq_addr_t addr)
{
	boost::shared_ptr<PatchagePort> ret(
		new PatchagePort(parent, ALSA_MIDI, name, is_input,
			_app->state_manager()->get_port_color(ALSA_MIDI)));
	ret->alsa_addr(addr);
	return ret;
}


/** Refresh all Alsa Midi ports. 
 */
void
AlsaDriver::refresh_ports()
{
	assert(is_attached());
	assert(_seq);
	
	snd_seq_client_info_t* cinfo;
	snd_seq_client_info_alloca(&cinfo);
	snd_seq_client_info_set_client(cinfo, -1);

	snd_seq_port_info_t * pinfo;
	snd_seq_port_info_alloca(&pinfo);
	
	string client_name;
	string port_name;
	bool is_input       = false;
	bool is_duplex      = false;
	bool is_application = true;

	set<SharedPtr<PatchageModule> > resized_modules;
	
	while (snd_seq_query_next_client (_seq, cinfo) >= 0) {
		snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
		snd_seq_port_info_set_port(pinfo, -1);

		client_name = snd_seq_client_info_get_name(cinfo);

		while (snd_seq_query_next_port(_seq, pinfo) >= 0) {
			int caps = snd_seq_port_info_get_capability(pinfo);
			int type = snd_seq_port_info_get_type(pinfo);

			// Skip ports we shouldn't show
			if (caps & SND_SEQ_PORT_CAP_NO_EXPORT)
				continue;
			else if ( !( (caps & SND_SEQ_PORT_CAP_READ)
						|| (caps & SND_SEQ_PORT_CAP_WRITE)
						|| (caps & SND_SEQ_PORT_CAP_DUPLEX)))
				continue;
			else if ((snd_seq_client_info_get_type(cinfo) != SND_SEQ_USER_CLIENT)
					&& ((type == SND_SEQ_PORT_SYSTEM_TIMER
						|| type == SND_SEQ_PORT_SYSTEM_ANNOUNCE)))
				continue;
			
			const snd_seq_addr_t addr = *snd_seq_port_info_get_addr(pinfo);
			
			is_duplex = false;

			// FIXME: Should be CAP_SUBS_READ etc?
			if ((caps & SND_SEQ_PORT_CAP_READ) && (caps & SND_SEQ_PORT_CAP_WRITE))
				is_duplex = true;
			else if (caps & SND_SEQ_PORT_CAP_READ)
				is_input = false;
			else if (caps & SND_SEQ_PORT_CAP_WRITE)
				is_input = true;

			is_application = (type & SND_SEQ_PORT_TYPE_APPLICATION);
			port_name = snd_seq_port_info_get_name(pinfo);
			boost::shared_ptr<PatchageModule> m;

			bool split = _app->state_manager()->get_module_split(client_name, !is_application)
				|| is_duplex;
			
			//cout << client_name << " : " << port_name << " is_application = " << is_application
			//	<< " is_duplex = " << is_duplex << ", split = " << split << endl;
			
			// Application input/output ports go on the same module
			if (!split) {
				m = _app->canvas()->find_module(client_name, InputOutput);
				if (!m) {
					m = boost::shared_ptr<PatchageModule>(new PatchageModule(_app, client_name, InputOutput));
					m->load_location();
					m->store_location();
				}
				
				if (!m->get_port(port_name)) {
					if (!is_duplex) {
						m->add_port(create_port(m, port_name, is_input, addr));
					} else {
						m->add_port(create_port(m, port_name, true, addr));
						m->add_port(create_port(m, port_name, false, addr));
					}
					resized_modules.insert(m);
				}

			} else { // non-application input/output ports (hw interface, etc) go on separate modules
				ModuleType type = InputOutput;
				
				// The 'application' hint isn't always set by clients, so this bit
				// is pretty nasty...

				if (!is_duplex) {  // just one port to add

					type = ((is_input) ? Input : Output);
					
					// See if an InputOutput module exists (maybe with Jack ports on it)
					m = _app->canvas()->find_module(client_name, InputOutput);
				
					if (!m)
						m = _app->canvas()->find_module(client_name, type);

					if (!m) {
						m = boost::shared_ptr<PatchageModule>(
							new PatchageModule(_app, client_name, type));
						m->load_location();
						m->store_location();
					}
					
					if (!m->get_port(port_name)) {
						m->add_port(create_port(m, port_name, is_input, addr));
						resized_modules.insert(m);
					}

				} else {  // two ports to add
					type = Input;
					
					// See if an InputOutput module exists (maybe with Jack ports on it)
					m = _app->canvas()->find_module(client_name, InputOutput);
					
					if (!m)
						m = _app->canvas()->find_module(client_name, type);

					if (!m) {
						m = boost::shared_ptr<PatchageModule>(
							new PatchageModule(_app, client_name, type));
						m->load_location();
						m->store_location();
						_app->canvas()->add_item(m);
					}

					assert(m);

					if (!m->get_port(port_name)) {
						m->add_port(create_port(m, port_name, true, addr));
						resized_modules.insert(m);
					}

					type = Output;
					
					// See if an InputOutput module exists (maybe with Jack ports on it)
					m = _app->canvas()->find_module(client_name, InputOutput);
					
					if (!m)
						m = _app->canvas()->find_module(client_name, type);

					if (!m) {
						m = boost::shared_ptr<PatchageModule>(
							new PatchageModule(_app, client_name, type));
						m->load_location();
						m->store_location();
						_app->canvas()->add_item(m);
					}

					if (!m->get_port(port_name)) {
						m->add_port(create_port(m, port_name, false, addr));
						resized_modules.insert(m);
					}
				}
			}
		}
	}

	for (set<SharedPtr<PatchageModule> >::const_iterator i = resized_modules.begin();
			i != resized_modules.end(); ++i)
		(*i)->resize();
}


/** Refresh all Alsa Midi connections.
 */
void
AlsaDriver::refresh_connections()
{
	assert(is_attached());
	assert(_seq);
	
	boost::shared_ptr<PatchageModule> m;
	boost::shared_ptr<PatchagePort>   p;
	
	for (ItemList::iterator i = _app->canvas()->items().begin();
			i != _app->canvas()->items().end(); ++i) {
		m = boost::dynamic_pointer_cast<PatchageModule>(*i);
		if (m) {
			for (PortVector::const_iterator j = m->ports().begin(); j != m->ports().end(); ++j) {
				p = boost::dynamic_pointer_cast<PatchagePort>(*j);
				if (p->type() == ALSA_MIDI)
					add_connections(p);
			}
		}
	}
}


/** Add all connections for the given port.
 */
void
AlsaDriver::add_connections(boost::shared_ptr<PatchagePort> port)
{
	assert(is_attached());
	assert(_seq);
	
	const snd_seq_addr_t* addr = port->alsa_addr();
	boost::shared_ptr<PatchagePort> connected_port;
	
	// Fix a problem with duplex->duplex connections (would show up twice)
	// No sense doing them all twice anyway..
	if (port->is_input())
		return;
	
	snd_seq_query_subscribe_t* subsinfo;
	snd_seq_query_subscribe_alloca(&subsinfo);
	snd_seq_query_subscribe_set_root(subsinfo, addr);
	snd_seq_query_subscribe_set_index(subsinfo, 0);
	
	while (!snd_seq_query_port_subscribers(_seq, subsinfo)) {
		const snd_seq_addr_t* connected_addr = snd_seq_query_subscribe_get_addr(subsinfo);
		if (!connected_addr)
			continue;
		
		connected_port = _app->canvas()->find_port(*connected_addr, true);

		if (connected_port) {
			boost::shared_ptr<Connection> existing = _app->canvas()->get_connection(port, connected_port);
			if (existing) {
				existing->set_flagged(false);
			} else {
				_app->canvas()->add_connection(port, connected_port, port->color() + 0x22222200);
			}
		}

		snd_seq_query_subscribe_set_index(subsinfo, snd_seq_query_subscribe_get_index(subsinfo) + 1);
	}

}


/** Connects two Alsa Midi ports.
 * 
 * \return Whether connection succeeded.
 */
bool
AlsaDriver::connect(boost::shared_ptr<PatchagePort> src_port, boost::shared_ptr<PatchagePort> dst_port) 
{
	const snd_seq_addr_t* src = src_port->alsa_addr();
	const snd_seq_addr_t* dst = dst_port->alsa_addr();

	bool result = true;
	
	if (src && dst) {
		snd_seq_port_subscribe_t* subs;
		snd_seq_port_subscribe_malloc(&subs);
		snd_seq_port_subscribe_set_sender(subs, src);
		snd_seq_port_subscribe_set_dest(subs, dst);
		snd_seq_port_subscribe_set_exclusive(subs, 0);
		snd_seq_port_subscribe_set_time_update(subs, 0);
		snd_seq_port_subscribe_set_time_real(subs, 0);

		// Already connected (shouldn't happen)
		if (!snd_seq_get_port_subscription(_seq, subs)) {
			cerr << "Error: Attempt to subscribe Alsa ports that are already subscribed." << endl;
			result = false;
		}

		int ret = snd_seq_subscribe_port(_seq, subs);
		if (ret < 0) {
			cerr << "Alsa subscription failed: " << snd_strerror(ret) << endl;
			result = false;
		}
	}
	
	if (result)
		_app->status_message(string("[ALSA] Connected ")
			+ src_port->full_name() + " -> " + dst_port->full_name());
	else
		_app->status_message(string("[ALSA] Unable to connect ")
			+ src_port->full_name() + " -> " + dst_port->full_name());

	return (!result);
}


/** Disconnects two Alsa Midi ports.
 * 
 * \return Whether disconnection succeeded.
 */
bool
AlsaDriver::disconnect(boost::shared_ptr<PatchagePort> src_port, boost::shared_ptr<PatchagePort> dst_port) 
{
	const snd_seq_addr_t* src = src_port->alsa_addr();
	const snd_seq_addr_t* dst = dst_port->alsa_addr();
	
	bool result = true;

	snd_seq_port_subscribe_t* subs;
	snd_seq_port_subscribe_malloc(&subs);
	snd_seq_port_subscribe_set_sender(subs, src);
	snd_seq_port_subscribe_set_dest(subs, dst);
	snd_seq_port_subscribe_set_exclusive(subs, 0);
	snd_seq_port_subscribe_set_time_update(subs, 0);
	snd_seq_port_subscribe_set_time_real(subs, 0);

	// Not connected (shouldn't happen)
	if (snd_seq_get_port_subscription(_seq, subs) != 0) {
		cerr << "Error: Attempt to unsubscribe Alsa ports that are not subscribed." << endl;
		result = false;
	}
	
	int ret = snd_seq_unsubscribe_port(_seq, subs);
	if (ret < 0) {
		cerr << "Alsa unsubscription failed: " << snd_strerror(ret) << endl;
		result = false;
	}
	
	if (result)
		_app->status_message(string("[ALSA] Disconnected ")
			+ src_port->full_name() + " -> " + dst_port->full_name());
	else
		_app->status_message(string("[ALSA] Unable to disconnect ")
			+ src_port->full_name() + " -> " + dst_port->full_name());

	return (!result);
}


bool
AlsaDriver::create_refresh_port()
{
	// Mostly lifted from alsa-patch-bay, (C) 2002 Robert Ham, released under GPL

	int ret;
	snd_seq_port_info_t* port_info;

	snd_seq_port_info_alloca(&port_info);
	snd_seq_port_info_set_name(port_info, "System Announcement Reciever");
	snd_seq_port_info_set_capability(port_info,
		SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE|SND_SEQ_PORT_CAP_NO_EXPORT);
	
	snd_seq_port_info_set_type(port_info, SND_SEQ_PORT_TYPE_APPLICATION);

	ret = snd_seq_create_port(_seq, port_info);
	if (ret) {
		cerr << "Error creating alsa port: " << snd_strerror(ret) << endl;
		return false;
	}

	// Subscribe the port to the system announcer
	ret = snd_seq_connect_from(_seq,
		snd_seq_port_info_get_port(port_info),
		SND_SEQ_CLIENT_SYSTEM,
		SND_SEQ_PORT_SYSTEM_ANNOUNCE);
	
	if (ret) {
		cerr << "Could not connect to system announcer port: " << snd_strerror(ret) << endl;
		return false;
	}

	return true;
}


void*
AlsaDriver::refresh_main(void* me)
{
	AlsaDriver* ad = (AlsaDriver*)me;
	ad->_refresh_main();
	return NULL;
}


void
AlsaDriver::_refresh_main()
{
	// "Heavily influenced" from alsa-patch-bay
	// (C) 2002 Robert Ham, released under GPL

	int             ret;
	int             nfds    = snd_seq_poll_descriptors_count(_seq, POLLIN);
	struct pollfd*  pfds    = new struct pollfd[nfds];
	unsigned short* revents = new unsigned short[nfds];

	if (!create_refresh_port()) {
		cerr << "Could not create Alsa listen port.  Auto refreshing will not work." << endl;
		return;
	}

	snd_seq_poll_descriptors(_seq, pfds, nfds, POLLIN);

	while (true) {
		ret = poll(pfds, nfds, -1);
		if (ret == -1) {
			if (errno == EINTR)
				continue;

			cerr << "Error polling Alsa sequencer: " << strerror(errno) << endl;
			continue;
		}

		ret = snd_seq_poll_descriptors_revents(_seq, pfds, nfds, revents);
		if (ret) {
			cerr << "Error getting Alsa sequencer poll events: "
				<< snd_strerror(ret) << endl;
			continue;
		}

		for (int i = 0; i < nfds; ++i) {
			if (revents[i] > 0) {
				snd_seq_event_t* ev;
				snd_seq_event_input(_seq, &ev);

				if (ev == NULL)
					continue;

				switch (ev->type) {
				case SND_SEQ_EVENT_PORT_SUBSCRIBED:
					/*cout << "Subscription: ";
					print_addr(ev->data.connect.sender);
					cout << " -> ";
					print_addr(ev->data.connect.dest);
					cout << endl;*/

					_events.push(PatchageEvent(PatchageEvent::CONNECTION,
								ev->data.connect.sender, ev->data.connect.dest));
					break;
				case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
					_events.push(PatchageEvent(PatchageEvent::DISCONNECTION,
								ev->data.connect.sender, ev->data.connect.dest));
					break;
				case SND_SEQ_EVENT_RESET:
				case SND_SEQ_EVENT_CLIENT_START:
				case SND_SEQ_EVENT_CLIENT_EXIT:
				case SND_SEQ_EVENT_CLIENT_CHANGE:
				case SND_SEQ_EVENT_PORT_START:
				case SND_SEQ_EVENT_PORT_EXIT:
				case SND_SEQ_EVENT_PORT_CHANGE:
					break;

					break;
				default:
					break;
				}
			}
		}
	}
}


void
AlsaDriver::print_addr(snd_seq_addr_t addr)
{
	cout << (int)addr.client << ":" << (int)addr.port << endl;
}
