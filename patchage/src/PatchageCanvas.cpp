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

#include <raul/SharedPtr.hpp>
#include CONFIG_H_PATH
#include "PatchageCanvas.hpp"
#include "Patchage.hpp"
#include "JackDriver.hpp"
#include "PatchageModule.hpp"
#include "PatchagePort.hpp"
#ifdef HAVE_ALSA
#include "AlsaDriver.hpp"
#endif

PatchageCanvas::PatchageCanvas(Patchage* app, int width, int height)
: FlowCanvas::Canvas(width, height),
  _app(app)
{
}


boost::shared_ptr<Item>
PatchageCanvas::get_item(const string& name)
{
	ItemList::iterator m = _items.begin();

	for ( ; m != _items.end(); ++m)
		if ((*m)->name() == name)
			break;

	return (m == _items.end()) ? boost::shared_ptr<Item>() : *m;
}


boost::shared_ptr<PatchageModule>
PatchageCanvas::find_module(const string& name, ModuleType type)
{
	for (ItemList::iterator m = _items.begin(); m != _items.end(); ++m) {
		boost::shared_ptr<PatchageModule> pm = boost::dynamic_pointer_cast<PatchageModule>(*m);
		if (pm && pm->name() == name && (pm->type() == type || pm->type() == InputOutput)) {
			return pm;
		}
	}

	return boost::shared_ptr<PatchageModule>();
}


boost::shared_ptr<PatchagePort>
PatchageCanvas::find_port(const PatchageEvent::PortRef& ref)
{
	jack_port_t* jack_port = NULL;
	string       module_name;
	string       port_name;
	
	SharedPtr<PatchageModule> module;
	boost::shared_ptr<PatchagePort> pp;

	// TODO: filthy.  keep a port map and make this O(log(n))
	switch (ref.type) {
	case PatchageEvent::PortRef::JACK_ID:
		jack_port = jack_port_by_id(_app->jack_driver()->client(), ref.id.jack_id);
		if (!jack_port)
			return boost::shared_ptr<PatchagePort>();
	
		_app->jack_driver()->port_names(ref, module_name, port_name);
	
		module = find_module(module_name,
				(jack_port_flags(jack_port) & JackPortIsInput) ? Input : Output);
	
		if (module)
			return PtrCast<PatchagePort>(module->get_port(port_name));
		else
			return boost::shared_ptr<PatchagePort>();
	
		break;
	
#ifdef HAVE_ALSA
	case PatchageEvent::PortRef::ALSA_ADDR:
		for (ItemList::iterator m = _items.begin(); m != _items.end(); ++m) {
			SharedPtr<PatchageModule> module = PtrCast<PatchageModule>(*m);
			if (!module)
				continue;
	
			for (PortVector::const_iterator p = module->ports().begin(); p != module->ports().end(); ++p) {
				pp = boost::dynamic_pointer_cast<PatchagePort>(*p);
				if (!pp)
					continue;
	
				if (pp->type() == ALSA_MIDI) {
					/*cerr << "ALSA PORT: " << (int)pp->alsa_addr()->client << ":"
					  << (int)pp->alsa_addr()->port << endl;*/
	
					if (pp->alsa_addr()
							&& pp->alsa_addr()->client == ref.id.alsa_addr.client
							&& pp->alsa_addr()->port   == ref.id.alsa_addr.port) {
						if (!ref.is_input && module->type() == Input) {
							//cerr << "WRONG DIRECTION, SKIPPED PORT" << endl;
						} else {
							return pp;
						}
					}
				}
			}
		}
	default:
		break;
	}
#endif // HAVE_ALSA

	return boost::shared_ptr<PatchagePort>();
}


void
PatchageCanvas::connect(boost::shared_ptr<Connectable> port1, boost::shared_ptr<Connectable> port2)
{
	boost::shared_ptr<PatchagePort> p1 = boost::dynamic_pointer_cast<PatchagePort>(port1);
	boost::shared_ptr<PatchagePort> p2 = boost::dynamic_pointer_cast<PatchagePort>(port2);
	if (!p1 || !p2)
		return;

	if (p1->type() == JACK_AUDIO && p2->type() == JACK_AUDIO
			|| (p1->type() == JACK_MIDI && p2->type() == JACK_MIDI))
		_app->jack_driver()->connect(p1, p2);
#ifdef HAVE_ALSA
	else if (p1->type() == ALSA_MIDI && p2->type() == ALSA_MIDI)
		_app->alsa_driver()->connect(p1, p2);
#endif
	else
		status_message("WARNING: Cannot make connection, incompatible port types.");
}


void
PatchageCanvas::disconnect(boost::shared_ptr<Connectable> port1, boost::shared_ptr<Connectable> port2)
{
	boost::shared_ptr<PatchagePort> input
		= boost::dynamic_pointer_cast<PatchagePort>(port1);
	boost::shared_ptr<PatchagePort> output
		= boost::dynamic_pointer_cast<PatchagePort>(port2);

	if (!input || !output)
		return;

	if (input->is_output() && output->is_input()) {
		// Damn, guessed wrong
		boost::shared_ptr<PatchagePort> swap = input;
		input = output;
		output = swap;
	}

	if (!input || !output || input->is_output() || output->is_input()) {
		status_message("ERROR: Attempt to disconnect mismatched/unknown ports");
		return;
	}
	
	if (input->type() == JACK_AUDIO && output->type() == JACK_AUDIO
			|| input->type() == JACK_MIDI && output->type() == JACK_MIDI)
		_app->jack_driver()->disconnect(output, input);
#ifdef HAVE_ALSA
	else if (input->type() == ALSA_MIDI && output->type() == ALSA_MIDI)
		_app->alsa_driver()->disconnect(output, input);
#endif
	else
		status_message("ERROR: Attempt to disconnect ports with mismatched types");
}


void
PatchageCanvas::status_message(const string& msg)
{
	_app->status_message(string("[Canvas] ").append(msg));
}


boost::shared_ptr<Port>
PatchageCanvas::get_port(const string& node_name, const string& port_name)
{
	for (ItemList::iterator i = _items.begin(); i != _items.end(); ++i) {
		const boost::shared_ptr<Item> item = *i;
		const boost::shared_ptr<Module> module
			= boost::dynamic_pointer_cast<Module>(item);
		if (!module)
			continue;
		const boost::shared_ptr<Port> port = module->get_port(port_name);
		if (module->name() == node_name && port)
			return port;
	}
	
	return boost::shared_ptr<Port>();
}

