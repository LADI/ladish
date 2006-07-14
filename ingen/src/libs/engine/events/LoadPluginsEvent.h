/* This file is part of Ingen.  Copyright (C) 2006 Dave Robillard.
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
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef LOADPLUGINSEVENT_H
#define LOADPLUGINSEVENT_H

#include "QueuedEvent.h"

namespace Om {


/** Loads all plugins into the internal plugin database (in NodeFactory).
 *
 * \ingroup engine
 */
class LoadPluginsEvent : public QueuedEvent
{
public:
	LoadPluginsEvent(CountedPtr<Responder> responder, samplecount timestamp);
	
	void post_process();
};


} // namespace Om

#endif // LOADPLUGINSEVENT_H
