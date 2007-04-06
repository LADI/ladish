/* This file is part of Patchage.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Patchage is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Patchage is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef DRIVER_H
#define DRIVER_H

#include <boost/shared_ptr.hpp>
#include <sigc++/sigc++.h>

class PatchagePort;

/** Trival driver base class */
class Driver {
public:
	virtual ~Driver() {}
	
	virtual void attach(bool launch_daemon) = 0;
	virtual void detach()                   = 0;
	virtual bool is_attached() const        = 0;

	virtual void refresh() = 0;

	virtual bool connect(boost::shared_ptr<PatchagePort> src_port,
	                     boost::shared_ptr<PatchagePort> dst_port) = 0;
	
	virtual bool disconnect(boost::shared_ptr<PatchagePort> src_port,
	                        boost::shared_ptr<PatchagePort> dst_port) = 0;
	
	/** Returns whether or not a refresh is required (pending). */
	inline bool is_dirty() const { return _is_dirty; }

	/** Clear 'dirty' status after a refresh. */
	inline void undirty() { _is_dirty = false; }

	sigc::signal<void> signal_attached;
	sigc::signal<void> signal_detached;

protected:
	Driver() : _is_dirty(false) {}
	
	bool _is_dirty;
};


#endif // DRIVER_H

