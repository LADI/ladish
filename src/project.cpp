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

struct project_impl
{
	string name;
	time_t modification_time;
	string comment;
};

project::project(
	const string& name,
	time_t modification_time,
	const string& comment)
{
	_impl_ptr = new project_impl;
	_impl_ptr->name = name;
	_impl_ptr->modification_time = modification_time;
	_impl_ptr->comment = comment;
}

project::~project()
{
	delete _impl_ptr;
}

void
project::get_name(
	string& name)
{
	name = _impl_ptr->name;
}

void
project::set_name(
	const string& name)
{
	_impl_ptr->name = name;
}

void
project::get_modification_time(
	time_t& modification_time)
{
}

void
get_comment(
	string& comment)
{
}
