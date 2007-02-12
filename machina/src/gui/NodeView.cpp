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

#include "NodeView.hpp"
#include <iostream>

NodeView::NodeView(SharedPtr<Machina::Node>             node,
                   SharedPtr<LibFlowCanvas::FlowCanvas> canvas,
                   const std::string&                   name,
                   double                               x,
                   double                               y)
	: LibFlowCanvas::Ellipse(canvas, name, x, y, 20, 20, false)
	, _node(node)
{
	//signal_double_clicked.connect(sigc::mem_fun(this, &NodeView::on_double_click));
}


void
NodeView::on_double_click(GdkEventButton*)
{
	bool is_initial = _node->is_initial();
	std::cerr << "INITIAL " << is_initial;
	_node->set_initial( ! is_initial );
}

void
NodeView::update_state()
{
	static const uint32_t active_color = 0x80808AFF;

	if (_node->is_active()) {
		if (_color != active_color) {
			_old_color = _color;
			set_base_color(active_color);
		}
	} else if (_color == active_color) {
		set_base_color(_old_color);
	}
}
