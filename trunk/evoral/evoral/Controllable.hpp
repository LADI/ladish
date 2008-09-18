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

#ifndef EVORAL_CONTROLLABLE_HPP
#define EVORAL_CONTROLLABLE_HPP

#include <set>
#include <map>
#include <boost/shared_ptr.hpp>
#include <glibmm/thread.h>
#include <evoral/types.hpp>
#include <evoral/Parameter.hpp>

namespace Evoral {

class Control;
class ControlList;
class ControlEvent;
class Transport;

class Controllable {
public:
	Controllable(const Transport& transport);
	virtual ~Controllable() {}

	virtual boost::shared_ptr<Control> control(Parameter id, bool create_if_missing=false);
	virtual boost::shared_ptr<const Control> control(Parameter id) const;
	
	boost::shared_ptr<Control> control_factory(boost::shared_ptr<ControlList> list);
	
	typedef std::map< Parameter, boost::shared_ptr<Control> > Controls;
	Controls&       controls()       { return _controls; }
	const Controls& controls() const { return _controls; }

	virtual void add_control(boost::shared_ptr<Control>);

	virtual void automation_snapshot(nframes_t now, bool force);
	inline bool should_snapshot(nframes_t now) {
		return (_last_automation_snapshot > now
				|| (now - _last_automation_snapshot) > _automation_interval);
	}

	virtual void transport_stopped(nframes_t now);

	virtual bool find_next_event(nframes_t start, nframes_t end, ControlEvent& ev) const;
	
	virtual float default_parameter_value(Parameter param) { return 1.0f; }

	virtual void clear_automation();

	AutoState    get_parameter_automation_state(Parameter param, bool lock=true);
	virtual void set_parameter_automation_state(Parameter param, AutoState);
	
	AutoStyle get_parameter_automation_style(Parameter param);
	void      set_parameter_automation_style(Parameter param, AutoStyle);

	void protect_automation();

	void what_has_automation(std::set<Parameter>&) const;
	void what_has_visible_automation(std::set<Parameter>&) const;
	const std::set<Parameter>& what_can_be_automated() const { return _can_automate_list; }

	void mark_automation_visible(Parameter, bool);
	
	Glib::Mutex& automation_lock() const { return _automation_lock; }

	static void set_automation_interval(nframes_t frames) { _automation_interval = frames; }
	static nframes_t automation_interval() { return _automation_interval; }

protected:
	void can_automate(Parameter);

	virtual void auto_state_changed(Parameter which) {}

	mutable Glib::Mutex _automation_lock;
	
	const Transport&    _transport;
	Controls            _controls;
	std::set<Parameter> _visible_controls;
	std::set<Parameter> _can_automate_list;
	nframes_t           _last_automation_snapshot;

	static nframes_t _automation_interval;
};

} // namespace Evoral

#endif // EVORAL_CONTROLLABLE_HPP
