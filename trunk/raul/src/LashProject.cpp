/* This file is part of Raul.
 * Copyright (C) 2007 Dave Robillard <http://drobilla.net>
 * 
 * Raul is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * Raul is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <iostream>
#include "raul/LashProject.hpp"

using namespace std;

namespace Raul {

LashProject::LashProject(SharedPtr<LashClient> client, const string& name)
	: _client(client)
	, _name(name)
{
}


void
LashProject::save()
{
	SharedPtr<LashClient> client = _client.lock();
	if (client) {
		cout << "[LASH] Saving project " << _name << endl;
		lash_event_t* event = lash_event_new_with_type(LASH_Save);
		lash_event_set_project(event, _name.c_str());
		lash_send_event(client->lash_client(), event);
	}
}


void
LashProject::close()
{
	SharedPtr<LashClient> client = _client.lock();
	if (client) {
		cout << "[LASH] Closing project " << _name << endl;
		lash_event_t* event = lash_event_new_with_type(LASH_Project_Remove);
		lash_event_set_project(event, _name.c_str());
		lash_send_event(client->lash_client(), event);
	}
}


void
LashProject::set_directory(const string& filename)
{
	SharedPtr<LashClient> client = _client.lock();
	if (client) {
		cout << "[LASH] Project " << _name << " directory = " << filename << endl;
		lash_event_t* event = lash_event_new_with_type(LASH_Project_Dir);
		lash_event_set_project(event, _name.c_str());
		lash_event_set_string(event, filename.c_str());
		lash_send_event(client->lash_client(), event);
	}
}


void
LashProject::set_name(const string& name)
{
	SharedPtr<LashClient> client = _client.lock();
	if (client) {
		cout << "[LASH] Project " << _name << " name = " << name << endl;
		lash_event_t* event = lash_event_new_with_type(LASH_Project_Name);
		lash_event_set_project(event, _name.c_str());
		lash_event_set_string(event, name.c_str());
		lash_send_event(client->lash_client(), event);
	}
	
	_name = name;
}


} // namespace Raul
