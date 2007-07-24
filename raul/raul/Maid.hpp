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

#ifndef RAUL_MAID_HPP
#define RAUL_MAID_HPP

#include <boost/utility.hpp>
#include <raul/SharedPtr.hpp>
#include <raul/SRSWQueue.hpp>
#include <raul/Deletable.hpp>
#include <raul/List.hpp>

namespace Raul {


/** Explicitly driven garbage collector.
 *
 * This is used by realtime threads to allow hard realtime deletion of objects
 * (push() is realtime safe).
 *
 * You can also manage a SharedPtr, so cleanup() will delete it when all
 * references are dropped (except the one held by the Maid itself).
 * This allows using a SharedPtr freely in hard realtime threads without having
 * to worry about deletion accidentally occuring in the realtime thread.
 *
 * cleanup() should be called periodically to free memory, often enough to
 * prevent the queue from overflowing.  This is probably best done by the main
 * thread to avoid the overhead of having a thread just to delete things.
 *
 * \ingroup raul
 */
class Maid : boost::noncopyable
{
public:
	Maid(size_t size);
	~Maid();

	/** Push a raw pointer to be deleted when cleanup() is called next.
	 * Realtime safe.
	 */
	inline void push(Raul::Deletable* obj) {
		if (obj)
			_objects.push(obj);
	}

	void manage(SharedPtr<Raul::Deletable> ptr);
	
	void cleanup();
	
private:
	typedef Raul::SRSWQueue<Raul::Deletable*>       Objects;
	typedef Raul::List<SharedPtr<Raul::Deletable> > Managed;
	
	Objects _objects;
	Managed _managed;
};


} // namespace Raul

#endif // RAUL_MAID_HPP

