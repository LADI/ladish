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

#ifndef RAUL_PATH_TABLE_HPP
#define RAUL_PATH_TABLE_HPP

#include "raul/Path.hpp"
#include "raul/Table.hpp"

namespace Raul {

template <typename T>
class PathTable : public Table<Path, T> {
public:
	/** Find all descendants of a Path key in the Table.
	 * It is guaranteed that (parent, parent+1, parent+2, ..., ret-1) are all
	 * descendants of parent.  The return value is never a descendent of
	 * parent, and may be end().
	 */
	typename Table<Path, T>::iterator find_descendants_end(
			typename Table<Path, T>::iterator parent)
	{
		return find_range_end(parent, &Path::descendant_comparator);
	}
	
	typename Table<Path, T>::const_iterator find_descendants_end(
			typename Table<Path, T>::const_iterator parent) const 
	{
		return find_range_end(parent, &Path::descendant_comparator);
	}
};


} // namespace Raul

#endif // RAUL_PATH_TABLE_HPP

