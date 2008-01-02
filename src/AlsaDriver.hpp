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

#ifndef ALSADRIVER_H
#define ALSADRIVER_H

#include <iostream>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <string>
#include "Driver.hpp"
class Patchage;
class PatchagePort;


/** Handles all externally driven functionality, registering ports etc.
 */
class AlsaDriver : public Driver
{
public:
	AlsaDriver(Patchage* app);
	~AlsaDriver();
	
	void attach(bool launch_daemon = false);
	void detach();

	bool is_attached() const { return (_seq != NULL); }

	void refresh();

	bool connect(boost::shared_ptr<PatchagePort> src_port,
	             boost::shared_ptr<PatchagePort> dst_port);

	bool disconnect(boost::shared_ptr<PatchagePort> src_port,
	                boost::shared_ptr<PatchagePort> dst_port);

	void print_addr(snd_seq_addr_t addr);
	
private:
	void refresh_ports();
	void refresh_connections();
	
	void add_connections(boost::shared_ptr<PatchagePort> port);
	
	bool          create_refresh_port();
	static void* refresh_main(void* me);
	void         _refresh_main();
	
	boost::shared_ptr<PatchagePort> create_port(
		boost::shared_ptr<PatchageModule> parent,
		const std::string&                name,
		bool                              is_input,
		snd_seq_addr_t                    addr);
	                                            
	Patchage*  _app;
	snd_seq_t* _seq;
	pthread_t  _refresh_thread;
};

#endif // ALSADRIVER_H
