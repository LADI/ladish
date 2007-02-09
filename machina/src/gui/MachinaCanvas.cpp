/* This file is part of Machina.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Machina is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Machina is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

//#include "config.h"
#include <raul/SharedPtr.h>
#include <flowcanvas/Ellipse.h>
#include "MachinaCanvas.hpp"
#include "MachinaGUI.hpp"

MachinaCanvas::MachinaCanvas(MachinaGUI* app, int width, int height)
: FlowCanvas(width, height),
  _app(app)
{

}


void
MachinaCanvas::connect(boost::shared_ptr<Connectable>, //item1,
                       boost::shared_ptr<Connectable>) //item2)
{
#if 0
	boost::shared_ptr<MachinaPort> p1 = boost::dynamic_pointer_cast<MachinaPort>(port1);
	boost::shared_ptr<MachinaPort> p2 = boost::dynamic_pointer_cast<MachinaPort>(port2);
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
#endif
}


void
MachinaCanvas::disconnect(boost::shared_ptr<Connectable>,// item1,
                          boost::shared_ptr<Connectable>)// item2)
{
#if 0
	boost::shared_ptr<MachinaPort> input
		= boost::dynamic_pointer_cast<MachinaPort>(port1);
	boost::shared_ptr<MachinaPort> output
		= boost::dynamic_pointer_cast<MachinaPort>(port2);

	if (!input || !output)
		return;

	if (input->is_output() && output->is_input()) {
		// Damn, guessed wrong
		boost::shared_ptr<MachinaPort> swap = input;
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
#endif
}


void
MachinaCanvas::status_message(const string& msg)
{
	_app->status_message(string("[Canvas] ").append(msg));
}


void
MachinaCanvas::item_selected(SharedPtr<Item> i)
{
	cerr << "SELECTED: " << i->name() << endl;
	
	SharedPtr<Connectable> item = PtrCast<Connectable>(i);
	if (!item)
		return;
	
	SharedPtr<Connectable> last = _last_selected.lock();

	if (last) {
		boost::shared_ptr<Connection> c(new Connection(shared_from_this(),
			last, item, 0x9999AAFF, true));
		last->add_connection(c);
		item->add_connection(c);
		add_connection(c);
		i->raise_to_top();
		_last_selected.reset();
	} else {
		_last_selected = item;
	}
}


void
MachinaCanvas::item_clicked(SharedPtr<Item> i, GdkEventButton* event)
{
	cerr << "CLICKED " << event->button << ": " <<  i->name() << endl;

	//return false;
}


bool
MachinaCanvas::canvas_event(GdkEvent* event)
{
	static int last = 0;
	
	assert(event);
	
	if (event->type == GDK_BUTTON_PRESS) {
	
		const double x = event->button.x;
		const double y = event->button.y;

		SharedPtr<Item> item;

		if (event->button.button == 1) {
			item = SharedPtr<Item>(new Ellipse(shared_from_this(),
				string("Note")+(char)(last++ +'0'), x, y, 30, 30, true));
		} else if (event->button.button == 2) {
			item = SharedPtr<Item>(new Module(shared_from_this(),
				string("Note")+(char)(last++ +'0'), x, y, true));
		}

		if (item) {
			item->signal_selected.connect(sigc::bind(sigc::mem_fun(this,
				&MachinaCanvas::item_selected), item));
			item->signal_clicked.connect(sigc::bind<0>(sigc::mem_fun(this,
				&MachinaCanvas::item_clicked), item));
			add_item(item);
			item->resize();
			item->raise_to_top();
		}
	}

	return FlowCanvas::canvas_event(event);
}
