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

#include <raul/Maid.hpp>
#include <raul/Deletable.hpp>

namespace Raul {


Maid::Maid(size_t size)
	: _objects(size)
{
}


Maid::~Maid()
{
	cleanup();
}

/** Manage a SharedPtr.
 * NOT realtime safe.
 *
 * @ptr is guaranteed to be deleted in the context that calls cleanup()
 */
void
Maid::manage(SharedPtr<Raul::Deletable> ptr)
{
	if (ptr)
		_managed.push_back(new Raul::List<SharedPtr<Raul::Deletable> >::Node(ptr));
}


/** Free all the objects in the queue (passed by push()).
 */
void
Maid::cleanup()
{
	Raul::Deletable* obj = NULL;

	while (!_objects.empty()) {
		obj = _objects.front();
		_objects.pop();
		delete obj;
	}

	for (Managed::iterator i = _managed.begin(); i != _managed.end() ; ) {
		Managed::iterator next = i;
		++next;

		if ((*i).unique())
			_managed.erase(i);

		i = next;
	}

}


} // namespace Raul
