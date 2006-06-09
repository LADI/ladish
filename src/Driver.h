/* This file is part of Patchage.  Copyright (C) 2005 Dave Robillard.
 * 
 * Om is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Om is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef DRIVER_H
#define DRIVER_H

class PatchagePort;

/** Trival driver base class */
class Driver {
public:
	virtual ~Driver() {}
	
	virtual void attach(bool launch_daemon) = 0;
	virtual void detach()                   = 0;
	virtual bool is_attached() const        = 0;

	virtual void refresh() = 0;

	virtual bool connect(const PatchagePort* src_port,
	                     const PatchagePort* dst_port)
	{ return false; }
	
	virtual bool disconnect(const PatchagePort* src_port,
	                     const PatchagePort* dst_port)
	{ return false; }
	
	/** Returns whether or not a refresh is required. */
	bool is_dirty() const { return m_is_dirty; }

	/** Clear 'dirty' status after a refresh. */
	void undirty() { m_is_dirty = false; }

protected:
	Driver() : m_is_dirty(false) {}
	
	bool m_is_dirty;
};


#endif // DRIVER_H

