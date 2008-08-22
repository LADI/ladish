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

#include "LashClient.hpp"
#include "Patchage.hpp"

using namespace std;

struct LashClientImpl {
	LashProxy* proxy;
	Project* project;
	string id;
	string name;
};

LashClient::LashClient(
    LashProxy* proxy,
    Project* project,
    const string& id,
    const string& name)
{
	_impl = new LashClientImpl;
	_impl->proxy = proxy;
	_impl->project = project;
	_impl->id = id;
	_impl->name = name;

	//g_app->info_msg("client created");
}

LashClient::~LashClient()
{
	delete _impl;
	//g_app->info_msg("client destroyed");
}

Project*
LashClient::get_project()
{
	return _impl->project;
}

const string&
LashClient::get_id() const
{
	return _impl->id;
}

const string&
LashClient::get_name() const
{
	return _impl->name;
}

void
LashClient::do_rename(const string& name)
{
	if (_impl->name != name) {
		//_impl->proxy->client_rename(_impl->id, name);
	}
}

void
LashClient::on_name_changed(const string& name)
{
	_impl->name = name;
	_signal_renamed.emit();
}
