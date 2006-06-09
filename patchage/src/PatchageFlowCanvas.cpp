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

#include "PatchageFlowCanvas.h"
#include "Patchage.h"
#include "JackDriver.h"
#include "AlsaDriver.h"
#include "PatchageModule.h"
#include "PatchagePort.h"


PatchageFlowCanvas::PatchageFlowCanvas(Patchage* app, int width, int height)
: FlowCanvas(width, height),
  m_app(app)
{
}


PatchageModule*
PatchageFlowCanvas::find_module(const string& name, ModuleType type)
{
	PatchageModule* pm = NULL;
	
	for (ModuleMap::iterator m = m_modules.begin(); m != m_modules.end(); ++m) {
		pm = (PatchageModule*)(*m).second;
		if (pm->name() == name && pm->type() == type) {
			return pm;
		}
	}

	return NULL;
}


PatchagePort*
PatchageFlowCanvas::find_port(const snd_seq_addr_t* alsa_addr, bool is_input)
{
	PatchagePort* pp = NULL;
	for (ModuleMap::iterator m = m_modules.begin(); m != m_modules.end(); ++m) {
		for (PortList::iterator p = (*m).second->ports().begin(); p != (*m).second->ports().end(); ++p) {
			pp = (PatchagePort*)(*p);
			if (pp->type() == ALSA_MIDI && pp->alsa_addr()
					&& pp->alsa_addr()->client == alsa_addr->client
					&& pp->alsa_addr()->port == alsa_addr->port)
				if (is_input == pp->is_input())
					return pp;
		}
	}

	return NULL;
}


void
PatchageFlowCanvas::connect(const Port* port1, const Port* port2)
{
	PatchagePort* p1 = (PatchagePort*)port1;
	PatchagePort* p2 = (PatchagePort*)port2;
	
	if (p1->type() == JACK_AUDIO && p2->type() == JACK_AUDIO
			|| (p1->type() == JACK_MIDI && p2->type() == JACK_MIDI))
		/*m_app->jack_driver()->connect(p1->module()->name(), p1->name(),
	                                  p2->module()->name(), p2->name());*/
		m_app->jack_driver()->connect(p1, p2);
	else if (p1->type() == ALSA_MIDI && p2->type() == ALSA_MIDI)
		m_app->alsa_driver()->connect(p1, p2);
	else
		m_app->status_message("Cannot make connection, incompatible port types.");
}


void
PatchageFlowCanvas::disconnect(const Port* port1, const Port* port2)
{
	PatchagePort* input = NULL;
	PatchagePort* output = NULL;
	
	if (port1->is_input() && !port2->is_input()) {
		input = (PatchagePort*)port1;
		output = (PatchagePort*)port2;
	} else if (port2->is_input() && !port1->is_input()) {
		input = (PatchagePort*)port2;
		output = (PatchagePort*)port1;
	} else {
		m_app->status_message("Attempt to disconnect two input (or output) ports?? Please report bug.");
		return;
	}
	
	if (input->type() == JACK_AUDIO && output->type() == JACK_AUDIO
			|| input->type() == JACK_MIDI && output->type() == JACK_MIDI)
		m_app->jack_driver()->disconnect(output, input);
	else if (input->type() == ALSA_MIDI && output->type() == ALSA_MIDI)
		m_app->alsa_driver()->disconnect(output, input);
	else
		m_app->status_message("Attempt to disconnect Jack audio port from Alsa Midi port?? Please report bug.");
}


void
PatchageFlowCanvas::status_message(const string& msg)
{
	m_app->status_message(msg);
}

