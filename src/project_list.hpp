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

#ifndef PROJECT_LIST_HPP__D786489B_E400_4E92_85C7_2BAE606DE56D__INCLUDED
#define PROJECT_LIST_HPP__D786489B_E400_4E92_85C7_2BAE606DE56D__INCLUDED

struct project_list_impl;

class project_list
{
public:
	project_list(Glib::RefPtr<Gnome::Glade::Xml> xml);
	~project_list();

	void set_lash_availability(bool lash_active);

	void project_added(const string& project_name);
	void project_closed(const string& project_name);

private:
	project_list_impl * _impl_ptr;
};

#endif // #ifndef PROJECT_LIST_HPP__D786489B_E400_4E92_85C7_2BAE606DE56D__INCLUDED
