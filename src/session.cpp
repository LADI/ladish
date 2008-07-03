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
	const std::string& project_name)
{
	//shared_ptr<project> project_ptr(new project(project_name));

	//_impl_ptr->projects.push_back(project_ptr);

	_signal_project_added.emit(project_name);
}

void
session::project_close(
	const std::string& project_name)
{
	_signal_project_closed.emit(project_name);
}
