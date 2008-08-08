/* LVZ - A C++ interface for writing LV2 plugins.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
 *  
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __lvz_gui_h
#define __lvz_gui_h

#include "audioeffectx.h"

static const uint32_t kBlackCColor = 0x00000000;


class CRect {
public:
	CRect(long x=0, long y=0, long w=0, long h=0)
		: left(x), top(y), right(x+w), bottom(y+h)
	{}
	void offset(long x, long y) {}
	long left, top, right, bottom;
};

class CControl {
public:
	CControl(CRect& size) {}
	void setDirty(bool dirty) {}
	void setValue(float value) {}
};

class CPoint {
public:
	CPoint(long a_x, long a_y) : x(a_x), y(a_y) {}
	long x, y;
};

class CFrame {
public:
	CFrame(CRect& size, void* ptr, void* editor) {}
	void addView(CControl* control) {}
};

class CDrawContext {
public:
	void setFillColor(uint32_t color) {}
	void fillRect(CRect& rect);
};

class CBitmap {
public:
	CBitmap(long something) {}

	long getWidth() { return 0; }
	long getHeight() { return 0; }

	void draw(CDrawContext* context, CRect& rect) {}
};

class AEffGUIEditor {
public:
	AEffGUIEditor(AudioEffect* eff) : effect(eff), rect(0, 0, 0, 0) {}
	void open(void* ptr) {}

	void idle() {}

protected:
	AudioEffect* effect;
	CRect rect;
	CFrame* frame;
};

#endif // __lvz_gui_h

