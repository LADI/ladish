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
#include "lash_proxy.hpp"

struct project_impl
{
	lash_proxy * lash_ptr;
	string name;
	string comment;
	bool modified_status;
};

project::project(
	lash_proxy * lash_ptr,
	const string& name)
{
	lash_loaded_project_properties properties;

	lash_ptr->get_loaded_project_properties(name, properties);

	_impl_ptr = new project_impl;
	_impl_ptr->lash_ptr = lash_ptr;
	_impl_ptr->name = name;

	_impl_ptr->comment = properties.comment;
	_impl_ptr->modified_status = properties.modified_status;
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
project::on_name_changed(
	const string& name)
{
	_impl_ptr->name = name;
	_signal_renamed.emit();
}

void
project::get_comment(
	string& comment)
{
}

bool
project::get_modified_status()
{
	return _impl_ptr->modified_status;
}

bool
project::on_modified_status_changed(
	bool modified_status)
{
	_impl_ptr->modified_status = modified_status;
	_signal_modified_status_changed.emit();
}

void
project::do_rename(
	const string& name)
{
	_impl_ptr->lash_ptr->project_rename(_impl_ptr->name, name);
}
