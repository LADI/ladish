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

#include <iomanip>
#include <sstream>
#include "flowcanvas/Canvas.hpp"
#include "machina/Edge.hpp"
#include "EdgeView.hpp"
#include "NodeView.hpp"


/* probability colour stuff */

#define RGB_TO_UINT(r,g,b) ((((guint)(r))<<16)|(((guint)(g))<<8)|((guint)(b)))
#define RGB_TO_RGBA(x,a) (((x) << 8) | ((((guint)a) & 0xff)))
#define RGBA_TO_UINT(r,g,b,a) RGB_TO_RGBA(RGB_TO_UINT(r,g,b), a)

#define UINT_RGBA_R(x) (((uint32_t)(x))>>24)
#define UINT_RGBA_G(x) ((((uint32_t)(x))>>16)&0xff)
#define UINT_RGBA_B(x) ((((uint32_t)(x))>>8)&0xff)
#define UINT_RGBA_A(x) (((uint32_t)(x))&0xff)

#define MONO_INTERPOLATE(v1, v2, t) ((int)rint((v2)*(t)+(v1)*(1-(t))))

#define UINT_INTERPOLATE(c1, c2, t) \
  RGBA_TO_UINT( MONO_INTERPOLATE(UINT_RGBA_R(c1), UINT_RGBA_R(c2), t), \
		MONO_INTERPOLATE(UINT_RGBA_G(c1), UINT_RGBA_G(c2), t), \
		MONO_INTERPOLATE(UINT_RGBA_B(c1), UINT_RGBA_B(c2), t), \
		MONO_INTERPOLATE(UINT_RGBA_A(c1), UINT_RGBA_A(c2), t) )

inline static uint32_t edge_color(float prob)
{
	static uint32_t min = 0xFF4444C0;
	static uint32_t mid = 0xFFFF44C0;
	static uint32_t max = 0x44FF44C0;

	if (prob <= 0.5)
		return UINT_INTERPOLATE(min, mid, prob*2.0);
	else
		return UINT_INTERPOLATE(mid, max, (prob-0.5)*2.0);
}

/* end probability colour stuff */


using namespace FlowCanvas;

EdgeView::EdgeView(SharedPtr<Canvas>        canvas,
                   SharedPtr<NodeView>      src,
                   SharedPtr<NodeView>      dst,
                   SharedPtr<Machina::Edge> edge)
	: FlowCanvas::Connection(canvas, src, dst, 0x9FA0A0F4, true)
	, _edge(edge)
{
	set_color(edge_color(_edge->probability()));
	set_handle_style(HANDLE_CIRCLE);
	show_handle(true);
}

	
double
EdgeView::length_hint() const
{
	return _edge->tail().lock()->duration().ticks() * 10;
}


void
EdgeView::show_label(bool show)
{
	/* too slow
	if (show) {
		char label[4];
		snprintf(label, 4, "%3f", _edge->probability());
		set_label(label);
	} else {
		set_label("");
	}*/
	show_handle(show);
	set_color(edge_color(_edge->probability()));
}


void
EdgeView::update()
{
	if (_handle/* && _handle->text*/) {
		/*char label[4];
		snprintf(label, 4, "%3f", _edge->probability());
		set_label(label);*/
		show_handle(true);
	}
	set_color(edge_color(_edge->probability()));
}


bool
EdgeView::on_event(GdkEvent* ev)
{
	if (ev->type == GDK_BUTTON_PRESS) {
		if (ev->button.state & GDK_CONTROL_MASK) {
			if (ev->button.button == 1) {
				_edge->set_probability(_edge->probability() - 0.1);
				update();
				return true;
			} else if (ev->button.button == 3) {
				_edge->set_probability(_edge->probability() + 0.1);
				update();
				return true;
			}
		}
	}

	return false;
}


