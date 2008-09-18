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

#include <limits>
#include <evoral/ControlSet.hpp>
#include <evoral/ControlList.hpp>
#include <evoral/Control.hpp>
#include <evoral/Event.hpp>

using namespace std;

namespace Evoral {


nframes_t ControlSet::_automation_interval = 0;

ControlSet::ControlSet(const Transport& transport)
	: _transport(transport)
	, _last_automation_snapshot(0)
{}


void
ControlSet::add_control(boost::shared_ptr<Control> ac)
{
	Parameter param = ac->parameter();

	_controls[param] = ac;
	
	_can_automate_list.insert(param);

	// Sync everything (derived classes) up to initial values
	auto_state_changed(param);
}

void
ControlSet::what_has_automation (set<Parameter>& s) const
{
	Glib::Mutex::Lock lm (_automation_lock);
	Controls::const_iterator li;
	
	// FIXME: correct semantics?
	for (li = _controls.begin(); li != _controls.end(); ++li) {
		s.insert  ((*li).first);
	}
}

void
ControlSet::what_has_visible_automation (set<Parameter>& s) const
{
	Glib::Mutex::Lock lm (_automation_lock);
	set<Parameter>::const_iterator li;
	
	for (li = _visible_controls.begin(); li != _visible_controls.end(); ++li) {
		s.insert  (*li);
	}
}

/** If \a create_if_missing is true, a control list will be created and returned
 * if one does not already exists.  Otherwise NULL will be returned if a control list
 * for \a parameter does not exist.
 */
boost::shared_ptr<Control>
ControlSet::control (Parameter parameter, bool create_if_missing)
{
	Controls::iterator i = _controls.find(parameter);

	if (i != _controls.end()) {
		return i->second;

	} else if (create_if_missing) {
		boost::shared_ptr<ControlList> al (new ControlList (parameter));
		boost::shared_ptr<Control> ac(control_factory(al));
		add_control(ac);
		return ac;

	} else {
		//warning << "ControlList " << parameter.to_string() << " not found for " << _name << endmsg;
		return boost::shared_ptr<Control>();
	}
}

boost::shared_ptr<const Control>
ControlSet::control (Parameter parameter) const
{
	Controls::const_iterator i = _controls.find(parameter);

	if (i != _controls.end()) {
		return i->second;
	} else {
		//warning << "ControlList " << parameter.to_string() << " not found for " << _name << endmsg;
		return boost::shared_ptr<Control>();
	}
}

void
ControlSet::can_automate (Parameter what)
{
	_can_automate_list.insert (what);
}

void
ControlSet::mark_automation_visible (Parameter what, bool yn)
{
	if (yn) {
		_visible_controls.insert (what);
	} else {
		set<Parameter>::iterator i;

		if ((i = _visible_controls.find (what)) != _visible_controls.end()) {
			_visible_controls.erase (i);
		}
	}
}

bool
ControlSet::find_next_event (nframes_t now, nframes_t end, ControlEvent& next_event) const
{
	Controls::const_iterator li;	

	next_event.when = std::numeric_limits<nframes_t>::max();
	
  	for (li = _controls.begin(); li != _controls.end(); ++li) {
		ControlList::const_iterator i;
		boost::shared_ptr<const ControlList> alist (li->second->list());
		ControlEvent cp (now, 0.0f);
		
 		for (i = lower_bound (alist->begin(), alist->end(), &cp, ControlList::time_comparator);
				i != alist->end() && (*i)->when < end; ++i) {
 			if ((*i)->when > now) {
 				break; 
 			}
 		}
 		
 		if (i != alist->end() && (*i)->when < end) {
 			if ((*i)->when < next_event.when) {
 				next_event.when = (*i)->when;
 			}
 		}
 	}

 	return next_event.when != std::numeric_limits<nframes_t>::max();
}

void
ControlSet::clear_automation ()
{
	Glib::Mutex::Lock lm (_automation_lock);

	for (Controls::iterator li = _controls.begin(); li != _controls.end(); ++li)
		li->second->list()->clear();
}
	
void
ControlSet::set_parameter_automation_state (Parameter param, AutoState s)
{
	Glib::Mutex::Lock lm (_automation_lock);
	
	boost::shared_ptr<Control> c = control (param, true);

	if (s != c->list()->automation_state()) {
		c->list()->set_automation_state (s);
		//_session.set_dirty ();
	}
}

AutoState
ControlSet::get_parameter_automation_state (Parameter param, bool lock)
{
	AutoState result = Off;

	if (lock)
		_automation_lock.lock();

	boost::shared_ptr<Control> c = control(param);

	if (c)
		result = c->list()->automation_state();
	
	if (lock)
		_automation_lock.unlock();

	return result;
}

void
ControlSet::set_parameter_automation_style (Parameter param, AutoStyle s)
{
	Glib::Mutex::Lock lm (_automation_lock);
	
	boost::shared_ptr<Control> c = control(param, true);

	if (s != c->list()->automation_style()) {
		c->list()->set_automation_style (s);
		//_session.set_dirty ();
	}
}

AutoStyle
ControlSet::get_parameter_automation_style (Parameter param)
{
	Glib::Mutex::Lock lm (_automation_lock);

	boost::shared_ptr<Control> c = control(param);

	if (c) {
		return c->list()->automation_style();
	} else {
		return Absolute; // whatever
	}
}

void
ControlSet::protect_automation ()
{
	set<Parameter> automated_params;

	what_has_automation (automated_params);

	for (set<Parameter>::iterator i = automated_params.begin(); i != automated_params.end(); ++i) {

		boost::shared_ptr<Control> c = control(*i);

		switch (c->list()->automation_state()) {
		case Write:
			c->list()->set_automation_state (Off);
			break;
		case Touch:
			c->list()->set_automation_state (Play);
			break;
		default:
			break;
		}
	}
}

void
ControlSet::automation_snapshot (nframes_t now, bool force)
{
	if (force || _last_automation_snapshot > now || (now - _last_automation_snapshot) > _automation_interval) {

		for (Controls::iterator i = _controls.begin(); i != _controls.end(); ++i) {
			if (i->second->list()->automation_write()) {
				i->second->list()->rt_add (now, i->second->user_value());
			}
		}
		
		_last_automation_snapshot = now;
	}
}

void
ControlSet::transport_stopped (nframes_t now)
{
	for (Controls::iterator li = _controls.begin(); li != _controls.end(); ++li) {
		
		boost::shared_ptr<Control> c = li->second;
		
		c->list()->reposition_for_rt_add (now);

		if (c->list()->automation_state() != Off) {
			c->set_value(c->list()->eval(now));
		}
	}
}


/* FIXME: Make this function a user parameter so the application can create
 * special Control-derived objects for given types.
 */
boost::shared_ptr<Control>
ControlSet::control_factory(boost::shared_ptr<ControlList> list)
{
	return boost::shared_ptr<Control>(new Control(_transport, list));
}


} // namespace Evoral
