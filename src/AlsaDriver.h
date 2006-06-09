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

#ifndef ALSADRIVER_H
#define ALSADRIVER_H

#include <iostream>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <queue>
#include <string>
#include "Driver.h"
class Patchage;
class PatchagePort;
class PatchageFlowCanvas;

using std::queue; using std::string;


/** Handles all externally driven functionality, registering ports etc.
 */
class AlsaDriver : public Driver
{
public:
	AlsaDriver(Patchage* app);
	~AlsaDriver();
	
	void attach(bool launch_daemon = false);
	void detach();

	bool is_attached() const { return (m_seq != NULL); }

	void refresh();

	bool connect(const PatchagePort* const src_port, const PatchagePort* const dst_port);
	bool disconnect(const PatchagePort* const src_port, const PatchagePort* const dst_port);
	
	PatchageFlowCanvas* canvas() { return m_canvas; }

private:
	void refresh_ports();
	void refresh_connections();
	
	void add_connections(PatchagePort* port);
	
	bool create_refresh_port();
	static void* refresh_main(void* me);
	void m_refresh_main();
	
	Patchage*             m_app;
	PatchageFlowCanvas*   m_canvas;

	snd_seq_t* m_seq;

	pthread_t m_refresh_thread;
};


#endif // ALSADRIVER_H
