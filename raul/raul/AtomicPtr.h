/* This file is part of Raul.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Raul is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Raul is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef RAUL_ATOMIC_PTR_H
#define RAUL_ATOMIC_PTR_H

#include <glib.h>

namespace Raul {


template<typename T>
class AtomicPtr {
public:

	inline AtomicPtr(const AtomicPtr& copy)
		{ g_atomic_pointer_set(&_val, copy.get()); }

	inline T* get() const
		{ return (T*)g_atomic_pointer_get(&_val); }

	inline void operator=(T* val)
		{ g_atomic_pointer_set(&_val, val); }

	/** Set value to newval iff current value is oldval */
	inline bool compare_and_exchange(int oldval, int newval)
		{ return g_atomic_pointer_compare_and_exchange(&_val, oldval, newval); }

private:
	volatile T* _val;
};


} // namespace Raul

#endif // RAUL_ATOMIC_PTR_H
