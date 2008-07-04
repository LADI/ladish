// -*- Mode: C++ ; indent-tabs-mode: t -*-
/* This file is part of Patchage.
 * Copyright (C) 2008 Nedko Arnaudov <nedko@arnaudov.name>
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

#include "common.hpp"
#include "project.hpp"
#include "session.hpp"
#include <iostream>

struct session_impl
{
public:
	list<shared_ptr<project> > projects;
};

session::session()
{
	_impl_ptr = new session_impl;
}

session::~session()
{
	delete _impl_ptr;
}

void
session::project_add(
	shared_ptr<project> project_ptr)
{
	_impl_ptr->projects.push_back(project_ptr);

	_signal_project_added.emit(project_ptr);
}

shared_ptr<project>
session::find_project_by_name(
	const string& name)
{
	shared_ptr<project> project_ptr;
	string temp_name;

	for (list<shared_ptr<project> >::iterator iter = _impl_ptr->projects.begin(); iter != _impl_ptr->projects.end(); iter++)
	{
		project_ptr = *iter;
		project_ptr->get_name(temp_name);

		if (temp_name == name)
		{
			return project_ptr;
		}
	}

	return shared_ptr<project>();
}

void
session::project_close(
	const string& project_name)
{
	shared_ptr<project> project_ptr;
	string temp_name;

	for (list<shared_ptr<project> >::iterator iter = _impl_ptr->projects.begin(); iter != _impl_ptr->projects.end(); iter++)
	{
		project_ptr = *iter;
		project_ptr->get_name(temp_name);

		if (temp_name == project_name)
		{
			_impl_ptr->projects.erase(iter);
			_signal_project_closed.emit(project_ptr);
			return;
		}
	}
}
