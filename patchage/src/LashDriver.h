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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef LASHDRIVER_H
#define LASHDRIVER_H

#include <lash/lash.h>
#include "Driver.h"

class Patchage;

class LashDriver : public Driver
{
public:
	LashDriver(Patchage* app, int argc, char** argv);
	~LashDriver();
	
	void attach(bool launch_daemon);
	void detach();

	bool is_attached() const { return lash_enabled(_client); }
	
	bool connect(boost::shared_ptr<PatchagePort>, boost::shared_ptr<PatchagePort>)
	{ return false; }

	bool disconnect(boost::shared_ptr<PatchagePort>, boost::shared_ptr<PatchagePort>)
	{ return false; }

	void refresh() {}

	void process_events();

private:
	Patchage*      _app;
	lash_client_t* _client;
	lash_args_t*   _args;

	void handle_event(lash_event_t* conf);
	void handle_config(lash_config_t* conf);
};


#endif // LASHDRIVER_H
