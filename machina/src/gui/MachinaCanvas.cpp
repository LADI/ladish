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
#include "machina/Node.hpp"
#include "machina/Machine.hpp"
#include "machina/Action.hpp"
#include "machina/Edge.hpp"
#include "machina/LearnRequest.hpp"
#include "NodeView.hpp"
#include "MachinaCanvas.hpp"
#include "MachinaGUI.hpp"

using namespace LibFlowCanvas;


MachinaCanvas::MachinaCanvas(MachinaGUI* app, int width, int height)
: FlowCanvas(width, height),
  _app(app)
{

}


void
MachinaCanvas::status_message(const string& msg)
{
	_app->status_message(string("[Canvas] ").append(msg));
}


void
MachinaCanvas::node_clicked(SharedPtr<NodeView> item, GdkEventButton* event)
{
	cerr << "CLICKED: " << item->name() << endl;

	SharedPtr<NodeView> node = PtrCast<NodeView>(item);
	if (!node)
		return;
	
	// Middle click, learn
	if (event->button == 2) {
		_app->machine()->learn(Machina::LearnRequest::create(_app->maid(), node->node()));
		return;
	} else if (event->button == 3) {

		SharedPtr<NodeView> last = _last_clicked.lock();

		if (last) {
			if (node != last)
				connect_node(last, node);

			last->set_default_base_color();
			_last_clicked.reset();

		} else {
			_last_clicked = node;
			node->set_base_color(0xFF0000FF);
		}
	}
}


bool
MachinaCanvas::canvas_event(GdkEvent* event)
{
	static int last = 0;
	
	assert(event);
	
	if (event->type == GDK_BUTTON_RELEASE) {
	
		const double x = event->button.x;
		const double y = event->button.y;

		if (event->button.button == 1) {
			string name = string("Note")+(char)(last++ +'0');

			SharedPtr<Machina::Node> node(new Machina::Node(1024*10, false));
			//node->add_enter_action(SharedPtr<Machina::Action>(new Machina::PrintAction(name)));
			SharedPtr<NodeView> view(new NodeView(node, shared_from_this(),
				name, x, y));

			//view->signal_clicked.connect(sigc::bind(sigc::mem_fun(this,
			//	&MachinaCanvas::node_clicked), view));
			view->signal_clicked.connect(sigc::bind<0>(sigc::mem_fun(this,
				&MachinaCanvas::node_clicked), view));
			add_item(view);
			view->resize();
			view->raise_to_top();
			
			_app->machine()->add_node(node);
		}
	}

	return FlowCanvas::canvas_event(event);
}


void
MachinaCanvas::connect_node(boost::shared_ptr<NodeView> src,
                            boost::shared_ptr<NodeView> dst)
{
	boost::shared_ptr<Connection> c(new Connection(shared_from_this(),
				src, dst, 0x9999AAFF, true));
	src->add_connection(c);
	dst->add_connection(c);
	c->set_label("1.0");
	add_connection(c);

	src->node()->add_outgoing_edge(SharedPtr<Machina::Edge>(
		new Machina::Edge(src->node(), dst->node())));
}


void
MachinaCanvas::disconnect_node(boost::shared_ptr<NodeView>,// item1,
                               boost::shared_ptr<NodeView>)// item2)
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


