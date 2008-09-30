/****************************************************************************
    
    lv2plugin.cpp - support file for writing LV2 plugins in C++
    
    Copyright (C) 2006-2007 Lars Luthman <lars.luthman@gmail.com>
        Modified by Dave Robillard, 2008
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307  USA

****************************************************************************/

#include "lv2plugin.hpp"


namespace LV2 {

	DescList::~DescList() {
		for (unsigned i = 0; i < size(); ++i)
			delete [] operator[](i).URI;
	}

	DescList& get_lv2_descriptors() {
		static DescList descriptors;
		return descriptors;
	}

}


extern "C" {
  
	const LV2_Descriptor* lv2_descriptor(uint32_t index) {
		if (index < LV2::get_lv2_descriptors().size())
			return &LV2::get_lv2_descriptors()[index];
		return NULL;
	}
  
}


