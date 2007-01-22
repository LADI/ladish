/* This file is part of Raul.  Copyright (C) 2007 Dave Robillard.
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

#ifndef RAUL_ATOMIC_INT_H
#define RAUL_ATOMIC_INT_H

#include <glib.h>

namespace Raul {


class AtomicInt {
public:
	
	inline AtomicInt(int val)
		{ g_atomic_int_set(&_val, val); }

	inline AtomicInt(const AtomicInt& copy)
		{ g_atomic_int_set(&_val, copy.get()); }
	
	inline int get() const
		{ return g_atomic_int_get(&_val); }

	inline void operator=(int val)
		{ g_atomic_int_set(&_val, val); }

	inline void operator+=(int val)
		{ g_atomic_int_add(&_val, val); }
	
	inline void operator-=(int val)
		{ g_atomic_int_add(&_val, -val); }
	
	inline bool operator==(int val) const
		{ return get() == val; }

	inline int operator+(int val) const
		{ return get() + val; }

	inline AtomicInt& operator++() // prefix
		{ g_atomic_int_inc(&_val); return *this; }
	
	inline AtomicInt& operator--() // prefix
		{ g_atomic_int_add(&_val, -1); return *this; }
	
	/** Set value to newval iff current value is oldval */
	inline bool compare_and_exchange(int oldval, int newval)
		{ return g_atomic_int_compare_and_exchange(&_val, oldval, newval); }

	/** Add val to value.
	 * @return value immediately before addition took place.
	 */
	inline int exchange_and_add(int val)
		{ return g_atomic_int_exchange_and_add(&_val, val); }

	/** Decrement value.
	 * @return true if value is now 0, otherwise false.
	 */
	inline bool decrement_and_test()
		{ return g_atomic_int_dec_and_test(&_val); }

private:
	volatile int _val;
};


} // namespace Raul

#endif // RAUL_ATOMIC_INT_H
