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

#ifndef SESSION_HPP__C870E949_EF2A_43E8_8FE8_55AE5A714172__INCLUDED
#define SESSION_HPP__C870E949_EF2A_43E8_8FE8_55AE5A714172__INCLUDED

struct session_impl;
class project;
class lash_client;

class session
{
public:
	session();
	~session();

	void
	clear();

	void
	project_add(
		shared_ptr<project> project_ptr);

	void
	project_close(
		const string& project_name);

	shared_ptr<project> find_project_by_name(const string& name);

	void
	client_add(
		shared_ptr<lash_client> client_ptr);

	void
	client_remove(
		const string& id);

	shared_ptr<lash_client> find_client_by_id(const string& id);

	sigc::signal1<void, shared_ptr<project> > _signal_project_added;
	sigc::signal1<void, shared_ptr<project> > _signal_project_closed;

private:
	session_impl * _impl_ptr;
};

#endif // #ifndef SESSION_HPP__C870E949_EF2A_43E8_8FE8_55AE5A714172__INCLUDED
