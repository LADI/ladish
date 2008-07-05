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

#ifndef LASH_CLIENT_HPP__BC03AD3F_8D6F_4D3A_95ED_ECDA8C929BCF__INCLUDED
#define LASH_CLIENT_HPP__BC03AD3F_8D6F_4D3A_95ED_ECDA8C929BCF__INCLUDED

struct lash_client_impl;
class lash_proxy;
class lash_proxy_impl;
class project;

class lash_client
{
public:
	lash_client(
		lash_proxy * lash_ptr,
		project * project_ptr,
		const string& id,
		const string& name);

	~lash_client();

	void
	get_name(
		string& name);

	void
	do_rename(
		const string& name);

	signal<void> _signal_renamed;

private:
	friend class lash_proxy_impl;

	void
	on_name_changed(
		const string& name);

	lash_client_impl * _impl_ptr;
};

#endif // #ifndef LASH_CLIENT_HPP__BC03AD3F_8D6F_4D3A_95ED_ECDA8C929BCF__INCLUDED
