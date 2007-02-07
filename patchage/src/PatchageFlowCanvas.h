/* This file is part of Patchage.  Copyright (C) 2004 Dave Robillard.
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

#ifndef PATCHAGEPATCHBAYAREA_H
#define PATCHAGEPATCHBAYAREA_H

#include <string>
#ifdef HAVE_ALSA
#include <alsa/asoundlib.h>
#endif
#include <flowcanvas/FlowCanvas.h>
#include "StateManager.h"



class Patchage;
class PatchageModule;
class PatchagePort;

using std::string;
using namespace LibFlowCanvas;

class PatchageFlowCanvas : public FlowCanvas
{
public:
	PatchageFlowCanvas(Patchage* _app, int width, int height);
	
	boost::shared_ptr<PatchageModule> find_module(const string& name, ModuleType type);
#ifdef HAVE_ALSA
	boost::shared_ptr<PatchagePort>   find_port(const snd_seq_addr_t* alsa_addr);
#endif
	void connect(boost::shared_ptr<Port> port1, boost::shared_ptr<Port> port2);
	void disconnect(boost::shared_ptr<Port> port1, boost::shared_ptr<Port> port2);

	void status_message(const string& msg);

private:
	Patchage* _app;
};


#endif // PATCHAGEPATCHBAYAREA_H
