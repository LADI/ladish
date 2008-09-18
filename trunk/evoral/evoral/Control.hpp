/* This file is part of Evoral.
 * Copyright (C) 2008 Dave Robillard <http://drobilla.net>
 * Copyright (C) 2000-2008 Paul Davis
 * 
 * Evoral is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Evoral is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef EVORAL_CONTROL_HPP
#define EVORAL_CONTROL_HPP

#include <set>
#include <map>
#include <boost/shared_ptr.hpp>
#include <glibmm/thread.h>
#include <evoral/types.hpp>
#include <evoral/Parameter.hpp>

namespace Evoral {

class ControlList;
class Transport;

class Control
{
public:
	Control(const Transport& transport, boost::shared_ptr<ControlList>);

	void  set_value(float val);
	float get_value()  const;
	float user_value() const;

	void set_list(boost::shared_ptr<ControlList>);

	boost::shared_ptr<ControlList>       list()       { return _list; }
	boost::shared_ptr<const ControlList> list() const { return _list; }

	Parameter parameter() const;

protected:
	const Transport&               _transport;
	boost::shared_ptr<ControlList> _list;
	float                          _user_value;
};

} // namespace Evoral

#endif // EVORAL_CONTROL_HPP
