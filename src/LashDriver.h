/* This file is part of Patchage.  Copyright (C) 2005 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

	bool is_attached() const { return lash_enabled(m_client); }

	void refresh() {}

	void process_events();

private:
	Patchage*      m_app;
	lash_client_t* m_client;
	lash_args_t*   m_args;

	void handle_event(lash_event_t* conf);
	void handle_config(lash_config_t* conf);
};


#endif // LASHDRIVER_H
