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
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <map>
#include <raul/SharedPtr.hpp>
#include "machina/Node.hpp"
#include "machina/Machine.hpp"
#include "machina/Action.hpp"
#include "machina/Edge.hpp"
#include "machina/LearnRequest.hpp"
#include "MachinaGUI.hpp"
#include "MachinaCanvas.hpp"
#include "NodeView.hpp"
#include "EdgeView.hpp"

using namespace FlowCanvas;


MachinaCanvas::MachinaCanvas(MachinaGUI* app, int width, int height)
: Canvas(width, height),
  _app(app)
{
	grab_focus();
}


void
MachinaCanvas::node_clicked(WeakPtr<NodeView> item, GdkEventButton* event)
{
	SharedPtr<NodeView> node = PtrCast<NodeView>(item.lock());
	if (!node)
		return;
	
	if (event->state & GDK_CONTROL_MASK)
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
	
	SharedPtr<Machina::Machine> machine = _app->machine();
	if (!machine)
		return false;

	if (event->type == GDK_BUTTON_RELEASE
			&& event->button.button == 3
			&& !(event->button.state & (GDK_CONTROL_MASK))) {

		const double x = event->button.x;
		const double y = event->button.y;

		string name = string("Note")+(char)(last++ +'0');

		SharedPtr<Machina::Node> node(new Machina::Node(1.0, false));
		SharedPtr<NodeView> view(new NodeView(_app->window(), shared_from_this(), node,
					name, x, y));

		view->signal_clicked.connect(sigc::bind<0>(sigc::mem_fun(this,
						&MachinaCanvas::node_clicked), WeakPtr<NodeView>(view)));
		add_item(view);
		view->resize();
		view->raise_to_top();

		machine->add_node(node);

		return true;
	
	} else {
		return Canvas::canvas_event(event);
	}
}


void
MachinaCanvas::connect_node(boost::shared_ptr<NodeView> src,
                            boost::shared_ptr<NodeView> head)
{
	SharedPtr<Machina::Edge> edge(new Machina::Edge(src->node(), head->node()));
	src->node()->add_outgoing_edge(edge);
	
	boost::shared_ptr<Connection> c(new EdgeView(shared_from_this(),
			src, head, edge));
	src->add_connection(c);
	head->add_connection(c);
	add_connection(c);
}


void
MachinaCanvas::disconnect_node(boost::shared_ptr<NodeView> src,
                               boost::shared_ptr<NodeView> head)
{
	src->node()->remove_outgoing_edges_to(head->node());
	remove_connection(src, head);
}


SharedPtr<NodeView>
MachinaCanvas::create_node_view(SharedPtr<Machina::Node> node)
{
	SharedPtr<NodeView> view(new NodeView(_app->window(), shared_from_this(), node,
				"", 10, 10));

	if ( ! node->enter_action() && ! node->exit_action() )
		view->set_base_color(0x101010FF);

	view->signal_clicked.connect(sigc::bind<0>(sigc::mem_fun(this,
					&MachinaCanvas::node_clicked), WeakPtr<NodeView>(view)));

	add_item(view);

	return view;
}


void
MachinaCanvas::build(SharedPtr<Machina::Machine> machine)
{
	destroy();
	_last_clicked.reset();

	if (!machine)
		return;

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

			SharedPtr<NodeView> head_view = views[(*e)->head()];
			if (!head_view) {
				cerr << "WARNING: Edge to node with no view" << endl;
				continue;
			}
				
			boost::shared_ptr<Connection> c(new EdgeView(shared_from_this(),
					view, head_view, (*e)));
			view->add_connection(c);
			head_view->add_connection(c);
			add_connection(c);
		}
	
		while (Gtk::Main::events_pending())
			Gtk::Main::iteration(false);
	}
	
	arrange();
}


