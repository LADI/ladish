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
#include <map>
#include <raul/SharedPtr.h>
#include "machina/Node.hpp"
#include "machina/Machine.hpp"
#include "machina/Action.hpp"
#include "machina/Edge.hpp"
#include "machina/LearnRequest.hpp"
#include "MachinaGUI.hpp"
#include "MachinaCanvas.hpp"
#include "NodeView.hpp"
#include "EdgeView.hpp"

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
MachinaCanvas::node_clicked(WeakPtr<NodeView> item, GdkEventButton* event)
{
	SharedPtr<NodeView> node = PtrCast<NodeView>(item.lock());
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
				if (get_connection(last, node))
					disconnect_node(last, node);
				else
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
	
	if (event->type == GDK_BUTTON_RELEASE
			&& event->button.state & GDK_CONTROL_MASK) {
	
		const double x = event->button.x;
		const double y = event->button.y;

		if (event->button.button == 1) {
			string name = string("Note")+(char)(last++ +'0');

			SharedPtr<Machina::Node> node(new Machina::Node(1.0, false));
			//node->add_enter_action(SharedPtr<Machina::Action>(new Machina::PrintAction(name)));
			SharedPtr<NodeView> view(new NodeView(node, shared_from_this(),
				name, x, y));

			view->signal_clicked.connect(sigc::bind<0>(sigc::mem_fun(this,
				&MachinaCanvas::node_clicked), WeakPtr<NodeView>(view)));
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
	SharedPtr<Machina::Edge> edge(new Machina::Edge(src->node(), dst->node()));
	src->node()->add_outgoing_edge(edge);
	
	boost::shared_ptr<Connection> c(new EdgeView(shared_from_this(),
			src, dst, edge));
	src->add_connection(c);
	dst->add_connection(c);
	add_connection(c);
}


void
MachinaCanvas::disconnect_node(boost::shared_ptr<NodeView> src,
                               boost::shared_ptr<NodeView> dst)
{
	src->node()->remove_outgoing_edges_to(dst->node());
	remove_connection(src, dst);

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


SharedPtr<NodeView>
MachinaCanvas::create_node_view(SharedPtr<Machina::Node> node)
{
	SharedPtr<NodeView> view(new NodeView(node, shared_from_this(),
				"", 10, 10));

	view->signal_clicked.connect(sigc::bind<0>(sigc::mem_fun(this,
					&MachinaCanvas::node_clicked), WeakPtr<NodeView>(view)));

	add_item(view);
	//view->resize();

	return view;
}


void
MachinaCanvas::build(SharedPtr<Machina::Machine> machine)
{
	destroy();

	std::map<SharedPtr<Machina::Node>, SharedPtr<NodeView> > views;

	for (Machina::Machine::Nodes::const_iterator i = machine->nodes().begin();
			i != machine->nodes().end(); ++i) {
	
		const SharedPtr<NodeView> view = create_node_view(*i);
		views.insert(std::make_pair((*i), view));
	}

	for (ItemList::iterator i = _items.begin(); i != _items.end(); ++i) {
		const SharedPtr<NodeView> view = PtrCast<NodeView>(*i);
		if (!view)
			continue;

		for (Machina::Node::Edges::const_iterator e = view->node()->outgoing_edges().begin();
				e != view->node()->outgoing_edges().end(); ++e) {

			SharedPtr<NodeView> dst_view = views[(*e)->dst()];
			assert(dst_view);
				
			boost::shared_ptr<Connection> c(new EdgeView(shared_from_this(),
					view, dst_view, (*e)));
			view->add_connection(c);
			dst_view->add_connection(c);
			add_connection(c);
		}
	
		while (Gtk::Main::events_pending())
			Gtk::Main::iteration(false);
	}
	
	arrange();
	/*	
	while (Gtk::Main::events_pending())
		Gtk::Main::iteration(false);

	for (list<boost::shared_ptr<Connection> >::iterator c = _connections.begin(); c != _connections.end(); ++c) {
		const SharedPtr<EdgeView> view = PtrCast<EdgeView>(*c);
		if (view)
			view->update_label(); // very, very slow
	
		while (Gtk::Main::events_pending())
			Gtk::Main::iteration(false);

	}
	*/
}


