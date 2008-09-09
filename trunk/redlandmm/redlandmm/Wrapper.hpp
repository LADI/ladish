/* This file is part of redlandmm.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * redlandmm is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * redlandmm is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef REDLANDMM_WRAPPER_HPP
#define REDLANDMM_WRAPPER_HPP

namespace Redland {


/** C++ wrapper for a redland object
 */
template <typename T>
class Wrapper {
public:
	Wrapper(T* c_obj = NULL) : _c_obj(c_obj) {}
	
	T*       c_obj()       { return _c_obj; }
	const T* c_obj() const { return _c_obj; }

protected:
	T* _c_obj;
};


} // namespace Redland

#endif // REDLANDMM_WRAPPER_HPP


