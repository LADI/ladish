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

#ifndef PROJECT_HPP__C1D5778B_7D4B_4DD7_9B27_657D79B53083__INCLUDED
#define PROJECT_HPP__C1D5778B_7D4B_4DD7_9B27_657D79B53083__INCLUDED

struct project_impl;
class lash_proxy;
class lash_proxy_impl;

class project
{
public:
	project(
		lash_proxy * lash_ptr,
		const string& name);

	~project();

	void
	get_name(
		string& name);

	void
	get_comment(
		string& comment);

	bool
	get_modified_status();

	void
	do_rename(
		const string& name);

	signal<void> _signal_renamed;
	signal<void> _signal_modified_status_changed;

private:
	friend class lash_proxy_impl;

	void
	on_name_changed(
		const string& name);

	bool
	on_modified_status_changed(
		bool modified_status);

	project_impl * _impl_ptr;
};

#endif // #ifndef PROJECT_HPP__C1D5778B_7D4B_4DD7_9B27_657D79B53083__INCLUDED
