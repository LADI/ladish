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


boost::shared_ptr<PatchageModule>
PatchageFlowCanvas::find_module(const string& name, ModuleType type)
{
	for (ModuleMap::iterator m = m_modules.begin(); m != m_modules.end(); ++m) {
		boost::shared_ptr<PatchageModule> pm = boost::dynamic_pointer_cast<PatchageModule>((*m).second);
		if (pm && pm->name() == name && pm->type() == type) {
			return pm;
		}
	}

	return boost::shared_ptr<PatchageModule>();
}


boost::shared_ptr<PatchagePort>
PatchageFlowCanvas::find_port(const snd_seq_addr_t* alsa_addr)
{
	boost::shared_ptr<PatchagePort> pp;
	for (ModuleMap::iterator m = m_modules.begin(); m != m_modules.end(); ++m) {
		for (PortVector::const_iterator p = (*m).second->ports().begin(); p != (*m).second->ports().end(); ++p) {
			pp = boost::dynamic_pointer_cast<PatchagePort>(*p);
			if (pp && pp->type() == ALSA_MIDI && pp->alsa_addr()
					&& pp->alsa_addr()->client == alsa_addr->client
					&& pp->alsa_addr()->port == alsa_addr->port)
					return pp;
		}
	}

	return boost::shared_ptr<PatchagePort>();
}


void
PatchageFlowCanvas::connect(boost::shared_ptr<Port> port1, boost::shared_ptr<Port> port2)
{
	boost::shared_ptr<PatchagePort> p1 = boost::dynamic_pointer_cast<PatchagePort>(port1);
	boost::shared_ptr<PatchagePort> p2 = boost::dynamic_pointer_cast<PatchagePort>(port2);
	if (!p1 || !p2)
		return;

	if (p1->type() == JACK_AUDIO && p2->type() == JACK_AUDIO
			|| (p1->type() == JACK_MIDI && p2->type() == JACK_MIDI))
		m_app->jack_driver()->connect(p1, p2);
	else if (p1->type() == ALSA_MIDI && p2->type() == ALSA_MIDI)
		m_app->alsa_driver()->connect(p1, p2);
	else
		status_message("WARNING: Cannot make connection, incompatible port types.");
}


void
PatchageFlowCanvas::disconnect(boost::shared_ptr<Port> port1, boost::shared_ptr<Port> port2)
{
	boost::shared_ptr<PatchagePort> input;
	boost::shared_ptr<PatchagePort> output;
	
	if (port1->is_input() && !port2->is_input()) {
		input = boost::dynamic_pointer_cast<PatchagePort>(port1);
		output = boost::dynamic_pointer_cast<PatchagePort>(port2);
	} else if (port2->is_input() && !port1->is_input()) {
		input = boost::dynamic_pointer_cast<PatchagePort>(port2);
		output = boost::dynamic_pointer_cast<PatchagePort>(port1);
	}

	if (!input || !output) {
		status_message("ERROR: Attempt to disconnect mismatched/unknown ports");
		return;
	}
	
	if (input->type() == JACK_AUDIO && output->type() == JACK_AUDIO
			|| input->type() == JACK_MIDI && output->type() == JACK_MIDI)
		m_app->jack_driver()->disconnect(output, input);
	else if (input->type() == ALSA_MIDI && output->type() == ALSA_MIDI)
		m_app->alsa_driver()->disconnect(output, input);
	else
		status_message("ERROR: Attempt to disconnect ports with mismatched types");
}


void
PatchageFlowCanvas::status_message(const string& msg)
{
	m_app->status_message(string("[Canvas] ").append(msg));
}

