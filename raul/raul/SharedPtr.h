/* A Reference Counting Smart Pointer.
 * Copyright (C) 2006 Dave Robillard.
 * 
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This file is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef RAUL_SHARED_PTR_H
#define RAUL_SHARED_PTR_H

#include <cassert>
#include <cstddef>

#ifdef BOOST_SP_ENABLE_DEBUG_HOOKS
#include <iostream>
#include <list>
#include <algorithm>

static std::list<void*> shared_ptr_counters;

// Use debug hooks to ensure 2 shared_ptrs never point to the same thing
namespace boost {
	
	inline void sp_scalar_constructor_hook(void* object, unsigned long cnt, void* ptr) {
		assert(std::find(shared_ptr_counters.begin(), shared_ptr_counters.end(),
				(void*)object) == shared_ptr_counters.end());
		shared_ptr_counters.push_back(object);
		//std::cerr << "Creating SharedPtr to "
		//	<< object << ", count = " << cnt << std::endl;
	}
	
	inline void sp_scalar_destructor_hook(void* object, unsigned long cnt, void* ptr) {
		shared_ptr_counters.remove(object);
		//std::cerr << "Destroying SharedPtr to "
		//	<< object << ", count = " << cnt << std::endl;
	}

}
#endif // BOOST_SP_ENABLE_DEBUG_HOOKS


#include <boost/shared_ptr.hpp>

#ifdef BOOST_AC_USE_PTHREADS
#error "Boost is using mutex locking for pointer reference counting."
#error "This is VERY slow.  Please report your platform."
#endif

#define SharedPtr boost::shared_ptr
#define PtrCast   boost::dynamic_pointer_cast

#endif // RAUL_SHARED_PTR_H

